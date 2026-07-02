#include "MLInferenceEngine.h"
#include <iostream>

MLInferenceEngine::MLInferenceEngine() {
}

MLInferenceEngine::~MLInferenceEngine() {
}

bool MLInferenceEngine::loadModel(const std::string& modelPath) {
    m_model = tflite::FlatBufferModel::BuildFromFile(modelPath.c_str());
    if (!m_model) {
        std::cerr << "Failed to load model at: " << modelPath << std::endl;
        return false;
    }

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder builder(*m_model, resolver);
    builder(&m_interpreter);

    if (!m_interpreter) {
        std::cerr << "Failed to construct interpreter" << std::endl;
        return false;
    }

    if (m_interpreter->AllocateTensors() != kTfLiteOk) {
        std::cerr << "Failed to allocate tensors" << std::endl;
        return false;
    }

    return true;
}

cv::Mat MLInferenceEngine::processImage(const cv::Mat& inputImage) {
    if (!m_interpreter) {
        std::cerr << "Model is not loaded!" << std::endl;
        return cv::Mat();
    }
    
    if (inputImage.empty()) {
        std::cerr << "Input image is empty!" << std::endl;
        return cv::Mat();
    }

    return inputImage.clone();
}
