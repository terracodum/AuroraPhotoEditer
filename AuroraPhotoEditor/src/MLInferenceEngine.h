#pragma once

#include <memory>
#include <string>

#include "tensorflow/lite/model.h"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

class MLInferenceEngine {
public:
    MLInferenceEngine();
    ~MLInferenceEngine();
    
    bool loadModel(const std::string& modelPath);
    
    cv::Mat processImage(const cv::Mat& inputImage);

private:
    std::unique_ptr<tflite::FlatBufferModel> m_model;
    std::unique_ptr<tflite::Interpreter> m_interpreter;
};
