#pragma once

#include <string>
#include <memory>
#include <vector>
#include <array>
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
    
    // Выполняет инференс на заданном изображении (возвращает маску 256x256)
    QImage runInference(const QImage& original, MLProfiler* profiler = nullptr);

    static QImage prepareModelInput(const QImage& original, const QSize& tensorSize, MLProfiler* profiler = nullptr);
    static QImage upscaleResult(const QImage& modelOutput, const QSize& originalSize, MLProfiler* profiler = nullptr);
private:
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    
    // Константы размера тензора
    static constexpr int inputChannels = 3;
    static constexpr int inputHeight = 256;
    static constexpr int inputWidth = 256;
    static constexpr int outputChannels = 1;

    // Переиспользуемые буферы для входных и выходных данных
    std::vector<float> inputTensorValues;
    std::vector<float> outputTensorValues;
    
    // Формы тензоров [batch, channels, height, width]
    std::array<int64_t, 4> inputDims;
    std::array<int64_t, 4> outputDims;

    // Имена входного и выходного узлов (для сессии)
    std::string inputName;
    std::string outputName;
};
