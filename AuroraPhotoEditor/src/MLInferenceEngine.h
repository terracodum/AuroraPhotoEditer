#pragma once

#include <string>
#include <memory>
#include <vector>
#include <array>
#include <optional>
#include <onnxruntime_cxx_api.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <QImage>
#include <QSize>

#include "MLTensorProcessor.h"

class MLProfiler;

class MLInferenceEngine {
public:
    MLInferenceEngine();
    ~MLInferenceEngine();

    bool loadModel(const std::string& modelPath);
    
    void setNormalizationMode(MLTensorProcessor::NormalizationMode mode) { normMode = mode; }
    
    void setDynamicInputSize(const QSize& size) {
        inputWidth = size.width();
        inputHeight = size.height();
        inputDims = {1, static_cast<int64_t>(inputChannels), static_cast<int64_t>(inputHeight), static_cast<int64_t>(inputWidth)};
        if (outputDims.size() >= 4) {
            outputDims[2] = inputHeight;
            outputDims[3] = inputWidth;
        }
    }
    
    // Выполняет инференс на заданном изображении (возвращает маску 256x256 или RGB изображение)
    QImage runInference(const QImage& original, MLProfiler* profiler = nullptr);

    static QImage prepareModelInput(const QImage& original, const QSize& tensorSize, MLProfiler* profiler = nullptr);
    static QImage upscaleResult(const QImage& modelOutput, const QSize& originalSize, MLProfiler* profiler = nullptr);
    
    // Checks the ONNX graph for the number of output channels
    static int getModelOutputChannels(const std::string& modelPath);
private:
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    
    std::optional<Ort::MemoryInfo> memoryInfo;

    MLTensorProcessor::NormalizationMode normMode = MLTensorProcessor::NormalizationMode::Standard;
    
    size_t inputChannels = 3;
    int64_t inputHeight = 256;
    int inputWidth = 256;
    int outputChannels = 1;
    
    // Формы тензоров [batch, channels, height, width]
    std::vector<int64_t> inputDims;
    std::vector<int64_t> outputDims;

    // Имена входного и выходного узлов (для сессии)
    std::string inputName;
    std::string outputName;
};
