#include "MLInferenceEngine.h"
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
