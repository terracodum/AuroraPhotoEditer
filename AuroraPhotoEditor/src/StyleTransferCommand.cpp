#include "StyleTransferCommand.h"
#include "MLInferenceEngine.h"
#include "MLProfiler.h"
#include <auroraapp.h>
#include <QUrl>
#include <QDebug>
#include <QMutex>
#include <memory>

namespace {
    // Cache the engine/model across invocations: loading an ONNX model
    // (parse + graph optimization) is expensive, and reloading it on every
    // execute() was pure waste. Only reload when the requested model changes.
    std::unique_ptr<MLInferenceEngine> g_styleEngine;
    QString g_styleLoadedModel;
    QMutex g_styleMutex;
}

StyleTransferCommand::StyleTransferCommand(const QString& modelName)
    : m_modelName(modelName) {
}

StyleTransferCommand::~StyleTransferCommand() = default;

QImage StyleTransferCommand::execute(const QImage& input, MLProfiler* profiler) const {
    if (input.isNull()) return input;

    QMutexLocker locker(&g_styleMutex);

    if (!g_styleEngine) {
        g_styleEngine = std::make_unique<MLInferenceEngine>();
        g_styleEngine->setNormalizationMode(MLTensorProcessor::NormalizationMode::None);
    }

    if (g_styleLoadedModel != m_modelName) {
        QString relativePath = "data/models/" + m_modelName;
        QUrl modelUrl = Aurora::Application::pathTo(relativePath);
        QString modelPathStr = modelUrl.isLocalFile() ? modelUrl.toLocalFile() : modelUrl.toString();

        if (profiler) profiler->startPhase("LoadModel_" + m_modelName);
        bool success = g_styleEngine->loadModel(modelPathStr.toStdString());
        if (profiler) profiler->endPhase();

        if (!success) {
            qWarning() << "StyleTransferCommand: Failed to load model" << modelPathStr;
            g_styleLoadedModel.clear();
            return input;
        }
        g_styleLoadedModel = m_modelName;
    }

    // Fast neural style models have 2 downsample/upsample layers (stride 2),
    // which require the input dimensions to be strictly divisible by 4.
    int w = input.width();
    int h = input.height();

    // Cap the working resolution. Style-transfer CNNs blow up activation
    // memory at full resolution and OOM on 32-bit devices; scale the longest
    // side down to maxSide (aspect preserved). The result is upscaled back to
    // the original size below, so output resolution is unchanged.
    const int maxSide = 720;
    if (w > maxSide || h > maxSide) {
        if (w >= h) {
            h = static_cast<int>(static_cast<qint64>(h) * maxSide / w);
            w = maxSide;
        } else {
            w = static_cast<int>(static_cast<qint64>(w) * maxSide / h);
            h = maxSide;
        }
    }

    w = (w / 4) * 4;
    h = (h / 4) * 4;
    if (w == 0) w = 4;
    if (h == 0) h = 4;

    g_styleEngine->setDynamicInputSize(QSize(w, h));

    QImage result = g_styleEngine->runInference(input, profiler);

    if (result.isNull()) {
        qWarning() << "StyleTransferCommand: Inference failed";
        return input;
    }

    if (result.size() != input.size()) {
        if (profiler) profiler->startPhase("Postprocessing_Scale");
        result = result.scaled(input.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        if (profiler) profiler->endPhase();
    }

    return result;
}

QString StyleTransferCommand::name() const {
    return "Style Transfer (" + m_modelName + ")";
}
