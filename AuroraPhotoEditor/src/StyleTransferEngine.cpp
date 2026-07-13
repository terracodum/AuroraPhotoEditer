#include "StyleTransferEngine.h"
#include "MLInferenceEngine.h"
#include "ImageScaler.h"

#include <QImage>
#include <QDebug>
#include <QtConcurrent>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QElapsedTimer>
#include <vector>
#include <cmath>
#include <algorithm>

QImage StyleTransferEngine::applyStyleTransfer(const QImage &source, const QString &modelPath, std::atomic<bool> *cancelFlag)
{
    auto checkCancelled = [cancelFlag]() {
        if (cancelFlag && cancelFlag->load(std::memory_order_acquire)) {
            throw std::runtime_error("cancelled");
        }
    };

    QImage srcImage = source;
    if (srcImage.format() != QImage::Format_RGB888) {
        srcImage = srcImage.convertToFormat(QImage::Format_RGB888);
    }

    const int origW = srcImage.width();
    const int origH = srcImage.height();

    Ort::Session* session = MLInferenceEngine::instance()->getSession(modelPath);
    if (!session) {
        throw std::runtime_error("Failed to load style model: " + modelPath.toStdString());
    }

    auto typeInfo = session->GetInputTypeInfo(0);
    auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
    auto expectedShape = tensorInfo.GetShape();

    int targetW = -1;
    int targetH = -1;
    bool isDynamic = false;

    if (expectedShape.size() == 4) {
        targetH = expectedShape[2] > 0 ? expectedShape[2] : -1;
        targetW = expectedShape[3] > 0 ? expectedShape[3] : -1;
    }

    QImage scaledImage;
    if (targetH > 0 && targetW > 0) {
        // Fixed shape model
        scaledImage = srcImage.scaled(targetW, targetH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else {
        // Dynamic shape model
        isDynamic = true;
        scaledImage = ImageScaler::prepareModelInput(srcImage, 720);
        targetW = scaledImage.width();
        targetH = scaledImage.height();
    }

    if (scaledImage.format() != QImage::Format_RGB888) {
        scaledImage = scaledImage.convertToFormat(QImage::Format_RGB888);
    }

    checkCancelled();

    std::vector<float> inputTensorValues(1 * 3 * targetH * targetW);
    for (int y = 0; y < targetH; ++y) {
        const uchar *line = scaledImage.constScanLine(y);
        for (int x = 0; x < targetW; ++x) {
            inputTensorValues[0 * targetH * targetW + y * targetW + x] = line[x * 3 + 0];
            inputTensorValues[1 * targetH * targetW + y * targetW + x] = line[x * 3 + 1];
            inputTensorValues[2 * targetH * targetW + y * targetW + x] = line[x * 3 + 2];
        }
    }

    checkCancelled();

    // session is already loaded and verified above

    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> inputShape = {1, 3, targetH, targetW};
    auto inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo, inputTensorValues.data(), inputTensorValues.size(),
        inputShape.data(), inputShape.size());

    Ort::AllocatorWithDefaultOptions allocator;
    auto inputNamePtr = session->GetInputNameAllocated(0, allocator);
    auto outputNamePtr = session->GetOutputNameAllocated(0, allocator);

    const char *inNames[] = {inputNamePtr.get()};
    const char *outNames[] = {outputNamePtr.get()};

    auto outputTensors = session->Run(Ort::RunOptions{nullptr}, inNames, &inputTensor, 1, outNames, 1);
    if (outputTensors.empty()) {
        throw std::runtime_error("Empty output from style model");
    }

    checkCancelled();

    auto outputInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
    auto outputShape = outputInfo.GetShape();

    int outH = targetH;
    int outW = targetW;
    if (outputShape.size() == 4) {
        outH = static_cast<int>(outputShape[2]);
        outW = static_cast<int>(outputShape[3]);
    }

    const float *outputData = outputTensors[0].GetTensorData<float>();
    QImage resultSmall(outW, outH, QImage::Format_RGB888);

    for (int y = 0; y < outH; ++y) {
        uchar *line = resultSmall.scanLine(y);
        for (int x = 0; x < outW; ++x) {
            line[x * 3 + 0] = static_cast<uchar>(std::max(0.0f, std::min(255.0f, outputData[0 * outH * outW + y * outW + x])));
            line[x * 3 + 1] = static_cast<uchar>(std::max(0.0f, std::min(255.0f, outputData[1 * outH * outW + y * outW + x])));
            line[x * 3 + 2] = static_cast<uchar>(std::max(0.0f, std::min(255.0f, outputData[2 * outH * outW + y * outW + x])));
        }
    }

    checkCancelled();

    QImage upscaled = ImageScaler::upscaleResult(resultSmall, QSize(origW, origH));
    QImage origARGB = source;
    if (origARGB.format() != QImage::Format_ARGB32) {
        origARGB = origARGB.convertToFormat(QImage::Format_ARGB32);
    }
    return ImageScaler::compositeWithDetails(upscaled, origARGB, 16);
}

StyleTransferEngine::StyleTransferEngine(QObject *parent)
    : QObject(parent), m_isProcessing(false)
{
}

StyleTransferEngine::~StyleTransferEngine()
{
}

void StyleTransferEngine::setIsProcessing(bool processing)
{
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        emit isProcessingChanged();
    }
}

void StyleTransferEngine::setModelPath(const QString &path)
{
    QString nativePath = path;
    if (nativePath.startsWith("file://")) {
        nativePath = nativePath.mid(7);
    }

    // Pre-load the session into the cache via MLInferenceEngine
    Ort::Session* session = MLInferenceEngine::instance()->getSession(nativePath);
    if (session) {
        m_modelPath = nativePath;
        qDebug() << "StyleTransfer: Model loaded via MLInferenceEngine:" << nativePath;
    } else {
        emit errorOccurred(QString("Failed to load style model: %1").arg(nativePath));
    }
}

void StyleTransferEngine::process(const QString &imagePath)
{
    if (m_modelPath.isEmpty()) {
        emit errorOccurred("Style model is not loaded");
        return;
    }

    if (m_isProcessing) {
        return;
    }

    setIsProcessing(true);

    QtConcurrent::run([this, imagePath]() {
        this->processInternal(imagePath);
    });
}

void StyleTransferEngine::processInternal(QString imagePath)
{
    QElapsedTimer totalTimer;
    totalTimer.start();

    if (imagePath.startsWith("file://")) {
        imagePath = imagePath.mid(7);
    }

    QImage originalImage;
    if (!originalImage.load(imagePath)) {
        QMetaObject::invokeMethod(this, "onStyleError",
            Qt::QueuedConnection,
            Q_ARG(QString, "Failed to load image"));
        return;
    }

    // Convert to RGB888
    if (originalImage.format() != QImage::Format_RGB888) {
        originalImage = originalImage.convertToFormat(QImage::Format_RGB888);
    }

    const int origW = originalImage.width();
    const int origH = originalImage.height();

    // Downscale for inference
    QElapsedTimer phaseTimer;
    phaseTimer.start();

    QImage scaledImage = ImageScaler::prepareModelInput(originalImage, 720);
    if (scaledImage.format() != QImage::Format_RGB888) {
        scaledImage = scaledImage.convertToFormat(QImage::Format_RGB888);
    }

    int targetW = scaledImage.width();
    int targetH = scaledImage.height();

    qDebug() << "StyleTransfer [Preprocess]:" << phaseTimer.elapsed() << "ms";

    // Prepare input tensor: [1, 3, H, W]
    phaseTimer.restart();
    std::vector<float> inputTensorValues(1 * 3 * targetH * targetW);

    for (int y = 0; y < targetH; ++y) {
        const uchar *line = scaledImage.constScanLine(y);
        for (int x = 0; x < targetW; ++x) {
            float r = static_cast<float>(line[x * 3 + 0]);
            float g = static_cast<float>(line[x * 3 + 1]);
            float b = static_cast<float>(line[x * 3 + 2]);

            inputTensorValues[0 * targetH * targetW + y * targetW + x] = r;
            inputTensorValues[1 * targetH * targetW + y * targetW + x] = g;
            inputTensorValues[2 * targetH * targetW + y * targetW + x] = b;
        }
    }
    qDebug() << "StyleTransfer [TensorPrep]:" << phaseTimer.elapsed() << "ms";

    // Run inference via MLInferenceEngine
    phaseTimer.restart();

    Ort::Session* session = MLInferenceEngine::instance()->getSession(m_modelPath);
    if (!session) {
        QMetaObject::invokeMethod(this, "onStyleError",
            Qt::QueuedConnection,
            Q_ARG(QString, "Model session not available"));
        return;
    }

    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> inputShape = {1, 3, targetH, targetW};

    auto inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo, inputTensorValues.data(), inputTensorValues.size(),
        inputShape.data(), inputShape.size());

    Ort::AllocatorWithDefaultOptions allocator;
    auto inputNamePtr = session->GetInputNameAllocated(0, allocator);
    auto outputNamePtr = session->GetOutputNameAllocated(0, allocator);

    const char *dynamicInputNames[] = {inputNamePtr.get()};
    const char *dynamicOutputNames[] = {outputNamePtr.get()};

    try {
        auto outputTensors = session->Run(
            Ort::RunOptions{nullptr},
            dynamicInputNames, &inputTensor, 1,
            dynamicOutputNames, 1);

        if (outputTensors.empty()) {
            throw std::runtime_error("Empty output from style model");
        }

        qDebug() << "StyleTransfer [ORT_Inference]:" << phaseTimer.elapsed() << "ms";

        // Decode output
        phaseTimer.restart();

        auto outputInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
        auto outputShape = outputInfo.GetShape();

        int outH = targetH;
        int outW = targetW;
        if (outputShape.size() == 4) {
            outH = static_cast<int>(outputShape[2]);
            outW = static_cast<int>(outputShape[3]);
        }

        const float *outputData = outputTensors[0].GetTensorData<float>();
        QImage resultImage(outW, outH, QImage::Format_RGB888);

        for (int y = 0; y < outH; ++y) {
            uchar *line = resultImage.scanLine(y);
            for (int x = 0; x < outW; ++x) {
                float r = outputData[0 * outH * outW + y * outW + x];
                float g = outputData[1 * outH * outW + y * outW + x];
                float b = outputData[2 * outH * outW + y * outW + x];

                line[x * 3 + 0] = static_cast<uchar>(std::max(0.0f, std::min(255.0f, r)));
                line[x * 3 + 1] = static_cast<uchar>(std::max(0.0f, std::min(255.0f, g)));
                line[x * 3 + 2] = static_cast<uchar>(std::max(0.0f, std::min(255.0f, b)));
            }
        }

        // Upscale and composite with high-frequency details from original
        QImage upscaled = ImageScaler::upscaleResult(resultImage, QSize(origW, origH));

        QImage origARGB = originalImage;
        if (origARGB.format() != QImage::Format_ARGB32) {
            origARGB = origARGB.convertToFormat(QImage::Format_ARGB32);
        }
        QImage composited = ImageScaler::compositeWithDetails(upscaled, origARGB, 16);

        qDebug() << "StyleTransfer [Postprocess]:" << phaseTimer.elapsed() << "ms";

        // Save result
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir().mkpath(tempDir);
        QString resultPath = tempDir + "/style_" +
            QUuid::createUuid().toString().remove('{').remove('}') + ".png";

        if (composited.save(resultPath)) {
            QMetaObject::invokeMethod(this, "onStyleSuccess",
                Qt::QueuedConnection,
                Q_ARG(QString, resultPath));
        } else {
            throw std::runtime_error("Failed to save stylized image");
        }

        qDebug() << "StyleTransfer [Total]:" << totalTimer.elapsed() << "ms";

    } catch (const std::exception &e) {
        QString errorMsg = QString::fromStdString(e.what());
        QMetaObject::invokeMethod(this, "onStyleError",
            Qt::QueuedConnection,
            Q_ARG(QString, errorMsg));
    }
}
