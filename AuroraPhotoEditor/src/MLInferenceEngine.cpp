#include "MLInferenceEngine.h"
#include "MLProfiler.h"
#include <iostream>

MLInferenceEngine::MLInferenceEngine() {
    try {
        // Initialize ONNX Runtime environment
        env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "MLInferenceEngine");
        std::cout << "ONNX Runtime environment initialized successfully." << std::endl;
    } catch (const Ort::Exception& e) {
        std::cerr << "Failed to initialize ONNX Runtime environment: " << e.what() << std::endl;
    }
}

MLInferenceEngine::~MLInferenceEngine() {
    // Unique pointers will automatically clean up the resources.
}

bool MLInferenceEngine::loadModel(const std::string& modelPath) {
    if (!env) {
        std::cerr << "ONNX Runtime environment is not initialized!" << std::endl;
        return false;
    }

    try {
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(1);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        // Create the session
        session = std::make_unique<Ort::Session>(*env, modelPath.c_str(), sessionOptions);
        std::cout << "Successfully loaded ONNX model: " << modelPath << std::endl;
        return true;
    } catch (const Ort::Exception& e) {
        std::cerr << "Exception loading model " << modelPath << ": " << e.what() << std::endl;
        return false;
    }
}

QImage MLInferenceEngine::prepareModelInput(const QImage& original, const QSize& tensorSize, MLProfiler* profiler) {
    if (profiler) {
        profiler->startPhase("Preprocessing");
    }
    
    // Use Qt::FastTransformation for speed, this is usually acceptable for downsizing to ML tensors
    QImage resized = original.scaled(tensorSize, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    
    if (profiler) {
        profiler->endPhase();
    }
    return resized;
}

QImage MLInferenceEngine::upscaleResult(const QImage& modelOutput, const QSize& originalSize, MLProfiler* profiler) {
    if (profiler) {
        profiler->startPhase("Postprocessing");
    }
    
    // Use Qt::SmoothTransformation for high-quality upscaling of masks/layers back to original resolution
    QImage upscaled = modelOutput.scaled(originalSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    
    if (profiler) {
        profiler->endPhase();
    }
    return upscaled;
}
