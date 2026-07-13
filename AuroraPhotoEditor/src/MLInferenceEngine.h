#pragma once

#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

class MLInferenceEngine {
public:
    MLInferenceEngine();
    ~MLInferenceEngine();

    bool loadModel(const std::string& modelPath);
    
private:
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
};
