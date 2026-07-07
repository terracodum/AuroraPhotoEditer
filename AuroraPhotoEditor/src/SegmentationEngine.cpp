#include "SegmentationEngine.h"
#include <QImage>
#include <QDebug>
#include <QtConcurrent>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <vector>
#include <cmath>

SegmentationEngine::SegmentationEngine(QObject *parent)
    : QObject(parent), m_isProcessing(false)
{
    m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "SegmentationEngine");
}

SegmentationEngine::~SegmentationEngine()
{
}

void SegmentationEngine::setIsProcessing(bool processing)
{
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        emit isProcessingChanged();
    }
}

void SegmentationEngine::setModelPath(const QString &path)
{
    QString nativePath = path;
    if (nativePath.startsWith("file://")) {
        nativePath = nativePath.mid(7);
    }
    
    try {
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(2);
        sessionOptions.SetInterOpNumThreads(2);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        m_session = std::make_unique<Ort::Session>(*m_env, nativePath.toStdString().c_str(), sessionOptions);
        m_modelPath = nativePath;
        qDebug() << "ONNX model loaded successfully:" << nativePath;
    } catch (const Ort::Exception& e) {
        qWarning() << "Failed to load ONNX model:" << e.what();
    }
}

void SegmentationEngine::process(const QString &imagePath)
{
    if (!m_session) {
        emit errorOccurred("Model is not loaded");
        return;
    }
    
    if (m_isProcessing) {
        return;
    }
    
    setIsProcessing(true);
    
    // Run in background
    QtConcurrent::run([this, imagePath]() {
        this->processInternal(imagePath);
    });
}

void SegmentationEngine::processInternal(QString imagePath)
{
    if (imagePath.startsWith("file://")) {
        imagePath = imagePath.mid(7);
    }

    QImage originalImage;
    if (!originalImage.load(imagePath)) {
        setIsProcessing(false);
        emit errorOccurred("Failed to load image");
        return;
    }

    // Convert to RGB888 if needed
    if (originalImage.format() != QImage::Format_RGB888) {
        originalImage = originalImage.convertToFormat(QImage::Format_RGB888);
    }

    int targetSize = 320;
    float mean[3] = {0.485f, 0.456f, 0.406f};
    float std_dev[3] = {0.229f, 0.224f, 0.225f};
    
    if (m_modelPath.contains("rmbg14")) {
        targetSize = 1024;
        mean[0] = 0.5f; mean[1] = 0.5f; mean[2] = 0.5f;
        std_dev[0] = 1.0f; std_dev[1] = 1.0f; std_dev[2] = 1.0f;
    }

    QImage scaledImage = originalImage.scaled(targetSize, targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    
    // Ensure format is RGB888 after scaling (SmoothTransformation often converts to ARGB32)
    if (scaledImage.format() != QImage::Format_RGB888) {
        scaledImage = scaledImage.convertToFormat(QImage::Format_RGB888);
    }

    // Prepare input tensor
    std::vector<float> inputTensorValues(1 * 3 * targetSize * targetSize);


    for (int y = 0; y < targetSize; ++y) {
        const uchar* line = scaledImage.constScanLine(y);
        for (int x = 0; x < targetSize; ++x) {
            // QImage Format_RGB888 is RGB
            float r = (line[x * 3 + 0] / 255.0f - mean[0]) / std_dev[0];
            float g = (line[x * 3 + 1] / 255.0f - mean[1]) / std_dev[1];
            float b = (line[x * 3 + 2] / 255.0f - mean[2]) / std_dev[2];
            
            // Channels first: R, G, B
            inputTensorValues[0 * targetSize * targetSize + y * targetSize + x] = r;
            inputTensorValues[1 * targetSize * targetSize + y * targetSize + x] = g;
            inputTensorValues[2 * targetSize * targetSize + y * targetSize + x] = b;
        }
    }

    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<int64_t> inputShape = {1, 3, targetSize, targetSize};
    
    auto inputTensor = Ort::Value::CreateTensor<float>(memoryInfo, inputTensorValues.data(), inputTensorValues.size(), inputShape.data(), inputShape.size());

    // input/output names resolved dynamically from session below
    
    // Instead of hardcoding, get from session
    Ort::AllocatorWithDefaultOptions allocator;
    auto inputNamePtr = m_session->GetInputNameAllocated(0, allocator);
    auto outputNamePtr = m_session->GetOutputNameAllocated(0, allocator);
    
    const char* dynamicInputNames[] = {inputNamePtr.get()};
    const char* dynamicOutputNames[] = {outputNamePtr.get()};

    try {
        auto outputTensors = m_session->Run(Ort::RunOptions{nullptr}, dynamicInputNames, &inputTensor, 1, dynamicOutputNames, 1);
        
        if (outputTensors.empty()) {
            throw std::runtime_error("Empty output from ONNX model");
        }

        const float* outputData = outputTensors[0].GetTensorData<float>();
        
        // Find min and max for debugging and auto-sigmoid
        float minVal = outputData[0];
        float maxVal = outputData[0];
        for (int i = 1; i < targetSize * targetSize; ++i) {
            if (outputData[i] < minVal) minVal = outputData[i];
            if (outputData[i] > maxVal) maxVal = outputData[i];
        }
        qDebug() << "ONNX output range:" << minVal << "to" << maxVal;
        
        bool needsSigmoid = (minVal < 0.0f || maxVal > 1.0f);
        if (needsSigmoid) {
            qDebug() << "Values outside [0, 1], applying Sigmoid.";
        }

        // Output is [1, 1, 320, 320] probability mask [0..1]
        QImage maskImage(targetSize, targetSize, QImage::Format_Grayscale8);
        for (int y = 0; y < targetSize; ++y) {
            uchar* line = maskImage.scanLine(y);
            for (int x = 0; x < targetSize; ++x) {
                float val = outputData[y * targetSize + x];
                if (needsSigmoid) {
                    val = 1.0f / (1.0f + std::exp(-val));
                } else {
                    if (val < 0.0f) val = 0.0f;
                    if (val > 1.0f) val = 1.0f;
                }
                line[x] = static_cast<uchar>(val * 255.0f);
            }
        }

        // Scale mask back to original image size
        QImage finalMask = maskImage.scaled(originalImage.width(), originalImage.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        
        // Save mask to temp file
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir().mkpath(tempDir);
        QString maskPath = tempDir + "/mask_" + QUuid::createUuid().toString().remove('{').remove('}') + ".png";
        
        if (finalMask.save(maskPath)) {
            // Qt 5.6: use QMetaObject::invokeMethod with method name string (no lambda form)
            QMetaObject::invokeMethod(this, "onSegmentationSuccess",
                Qt::QueuedConnection,
                Q_ARG(QString, maskPath));
        } else {
            throw std::runtime_error("Failed to save mask image");
        }
        
    } catch (const std::exception& e) {
        QString errorMsg = QString::fromStdString(e.what());
        QMetaObject::invokeMethod(this, "onSegmentationError",
            Qt::QueuedConnection,
            Q_ARG(QString, errorMsg));
    }
}
