#include "BackgroundCommand.h"
#include "MLProfiler.h"
#include <auroraapp.h>
#include <QUrl>
#include <QDebug>
#include <opencv2/opencv.hpp>

BackgroundCommand::BackgroundCommand() = default;
BackgroundCommand::~BackgroundCommand() = default;

QString BackgroundCommand::name() const {
    return "Background";
}

QImage BackgroundCommand::execute(const QImage& input, MLProfiler* profiler) const {
    if (input.isNull()) return input;

    if (!isModelLoaded) {
        if (profiler) profiler->startPhase("LoadModel");
        
        QUrl modelUrl = Aurora::Application::pathTo(QStringLiteral("data/models/u2net.onnx"));
        QString modelPathStr = modelUrl.isLocalFile() ? modelUrl.toLocalFile() : modelUrl.toString();
        
        bool success = engine.loadModel(modelPathStr.toStdString());
        if (!success) {
            qWarning() << "Failed to load model from" << modelPathStr;
            if (profiler) profiler->endPhase();
            return input;
        }
        isModelLoaded = true;
        if (profiler) profiler->endPhase();
    }

    // Run Inference
    QImage mask = engine.runInference(input, profiler);
    if (mask.isNull()) {
        qWarning() << "Inference returned a null mask";
        return input;
    }

    // Upscale the mask
    mask = MLInferenceEngine::upscaleResult(mask, input.size(), profiler);

    // Apply alpha exactly like ImageProcessor (this avoids diagonal stretching along with the Grayscale8 fix)
    QImage resultImage = input.convertToFormat(QImage::Format_ARGB32);
    
    if (profiler) profiler->startPhase("ApplyMask");
    
    for (int y = 0; y < resultImage.height(); ++y) {
        QRgb* resultLine = reinterpret_cast<QRgb*>(resultImage.scanLine(y));
        const uchar* maskLine = mask.constScanLine(y);
        for (int x = 0; x < resultImage.width(); ++x) {
            int alpha = maskLine[x];
            // Clear the existing alpha and apply the new alpha from the mask
            resultLine[x] = (resultLine[x] & 0x00ffffff) | (alpha << 24);
        }
    }

    if (profiler) profiler->endPhase();

    return resultImage;
}
