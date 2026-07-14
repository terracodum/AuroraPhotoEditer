#pragma once

#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <QImage>
#include <QSize>

class MLProfiler;

class MLInferenceEngine {
public:
    MLInferenceEngine();
    ~MLInferenceEngine();

    bool loadModel(const std::string& modelPath);
    
    static QImage prepareModelInput(const QImage& original, const QSize& tensorSize, MLProfiler* profiler = nullptr);
    static QImage upscaleResult(const QImage& modelOutput, const QSize& originalSize, MLProfiler* profiler = nullptr);
private:
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
};
