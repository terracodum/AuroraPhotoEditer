#include "StyleTransferCommand.h"
#include "MLInferenceEngine.h"
#include "MLProfiler.h"
#include <auroraapp.h>
#include <QUrl>
#include <QDebug>

StyleTransferCommand::StyleTransferCommand(const QString& modelName)
    : m_modelName(modelName) {
}

StyleTransferCommand::~StyleTransferCommand() = default;

QImage StyleTransferCommand::execute(const QImage& input, MLProfiler* profiler) const {
    if (input.isNull()) return input;

    MLInferenceEngine engine;
    engine.setNormalizationMode(MLTensorProcessor::NormalizationMode::None);

    QString relativePath = "data/models/" + m_modelName;
    QUrl modelUrl = Aurora::Application::pathTo(relativePath);
    QString modelPathStr = modelUrl.isLocalFile() ? modelUrl.toLocalFile() : modelUrl.toString();

    if (profiler) profiler->startPhase("LoadModel_" + m_modelName);
    bool success = engine.loadModel(modelPathStr.toStdString());
    if (profiler) profiler->endPhase();

    if (!success) {
        qWarning() << "StyleTransferCommand: Failed to load model" << modelPathStr;
        return input;
    }

    // Fast neural style models have 2 downsample/upsample layers (stride 2),
    // which require the input dimensions to be strictly divisible by 4.
    int w = input.width();
    int h = input.height();
    w = (w / 4) * 4;
    h = (h / 4) * 4;
    if (w == 0) w = 4;
    if (h == 0) h = 4;

    // Attempt to process the image near its original resolution instead of downscaling.
    engine.setDynamicInputSize(QSize(w, h));

    QImage result = engine.runInference(input, profiler);

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
