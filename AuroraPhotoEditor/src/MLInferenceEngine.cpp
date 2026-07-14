#include "MLInferenceEngine.h"
#include "MLProfiler.h"
#include <iostream>
#include <cmath>
#include <QPainter>

MLInferenceEngine::MLInferenceEngine() {
    try {
        // Initialize ONNX Runtime environment
        env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "MLInferenceEngine");
        
        memoryInfo.emplace(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
        
        std::cout << "ONNX Runtime environment initialized successfully." << std::endl;
    } catch (const Ort::Exception& e) {
        std::cerr << "Failed to initialize ONNX Runtime environment: " << e.what() << std::endl;
    }
}

MLInferenceEngine::~MLInferenceEngine() {
    // Unique pointers and Optionals will automatically clean up the resources.
}

bool MLInferenceEngine::loadModel(const std::string& modelPath) {
    if (!env) {
        std::cerr << "ONNX Runtime environment is not initialized!" << std::endl;
        return false;
    }

    try {
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(4);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

        // Create the session
        session = std::make_unique<Ort::Session>(*env, modelPath.c_str(), sessionOptions);
        std::cout << "Successfully loaded ONNX model: " << modelPath << std::endl;

        Ort::AllocatorWithDefaultOptions allocator;
        
        // Log all outputs and find the one with the largest resolution (or same as input)
        size_t num_outputs = session->GetOutputCount();
        size_t best_output_idx = 0;
        int64_t max_output_pixels = 0;

        std::cout << "Model has " << num_outputs << " outputs:" << std::endl;
        for (size_t i = 0; i < num_outputs; ++i) {
            auto out_name = session->GetOutputNameAllocated(i, allocator);
            auto type_info = session->GetOutputTypeInfo(i);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            std::vector<int64_t> shape = tensor_info.GetShape();
            
            std::cout << "  Output " << i << ": name=" << out_name.get() << " shape=[";
            int64_t pixels = 1;
            for (size_t j = 0; j < shape.size(); ++j) {
                std::cout << shape[j] << (j < shape.size() - 1 ? "," : "");
                if (shape[j] > 0) pixels *= shape[j];
            }
            std::cout << "]" << std::endl;

            if (pixels > max_output_pixels) {
                max_output_pixels = pixels;
                best_output_idx = i;
            }
        }

        auto input_name_alloc = session->GetInputNameAllocated(0, allocator);
        inputName = input_name_alloc.get();

        Ort::TypeInfo type_info = session->GetInputTypeInfo(0);
        auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
        std::vector<int64_t> input_node_dims = tensor_info.GetShape();

        if (input_node_dims.size() >= 4) {
            if (input_node_dims[0] < 0) input_node_dims[0] = 1;
            inputChannels = input_node_dims[1];
            inputHeight = input_node_dims[2];
            inputWidth = input_node_dims[3];
            if (inputHeight < 0) inputHeight = 256;
            if (inputWidth < 0) inputWidth = 256;
        }
        inputDims = {1, static_cast<int64_t>(inputChannels), static_cast<int64_t>(inputHeight), static_cast<int64_t>(inputWidth)};
        
        auto output_name_alloc = session->GetOutputNameAllocated(best_output_idx, allocator);
        outputName = output_name_alloc.get();

        std::cout << "Selected output " << best_output_idx << ": " << outputName << std::endl;

        Ort::TypeInfo out_type_info = session->GetOutputTypeInfo(best_output_idx);
        auto out_tensor_info = out_type_info.GetTensorTypeAndShapeInfo();
        std::vector<int64_t> output_node_dims = out_tensor_info.GetShape();

        for (size_t i = 0; i < output_node_dims.size(); ++i) {
            if (output_node_dims[i] < 0) {
                if (i == 0) output_node_dims[i] = 1;
                else if (i == 1) output_node_dims[i] = 1; // Assuming channel is 1
                else if (i == 2) output_node_dims[i] = inputHeight;
                else if (i == 3) output_node_dims[i] = inputWidth;
            }
        }
        outputDims = output_node_dims;

        size_t inputTensorSize = 1;
        for (auto d : inputDims) inputTensorSize *= d;
        
        size_t outputTensorSize = 1;
        for (auto d : outputDims) outputTensorSize *= d;

        std::cout << "Loaded model. Input dims: " << inputWidth << "x" << inputHeight 
                  << " Output elements: " << outputTensorSize << std::endl;

        inputTensorValues.assign(inputTensorSize, 0.0f);
        outputTensorValues.assign(outputTensorSize, 0.0f);
        
        inputTensor.emplace(Ort::Value::CreateTensor<float>(
            *memoryInfo, inputTensorValues.data(), inputTensorValues.size(),
            inputDims.data(), inputDims.size()));
            
        outputTensor.emplace(Ort::Value::CreateTensor<float>(
            *memoryInfo, outputTensorValues.data(), outputTensorValues.size(),
            outputDims.data(), outputDims.size()));

        return true;
    } catch (const Ort::Exception& e) {
        std::cerr << "Exception loading model " << modelPath << ": " << e.what() << std::endl;
        return false;
    }
}

QImage MLInferenceEngine::runInference(const QImage& original, MLProfiler* profiler) {
    if (!session) {
        std::cerr << "Session not initialized!" << std::endl;
        return QImage();
    }

    // 1. Препроцессинг
    if (profiler) profiler->startPhase("Preprocessing");
    
    QImage resized = original.scaled(inputWidth, inputHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (resized.format() != QImage::Format_RGB888) {
        resized = resized.convertToFormat(QImage::Format_RGB888);
    }

    // Заполнение входного тензора NCHW
    float mean[] = {0.485f, 0.456f, 0.406f};
    float std[] = {0.229f, 0.224f, 0.225f};
    
    if (inputWidth >= 1024) { // RMBG-1.4 usually uses 1024x1024
        mean[0] = 0.5f; mean[1] = 0.5f; mean[2] = 0.5f;
        std[0] = 1.0f; std[1] = 1.0f; std[2] = 1.0f;
    }

    int imgSize = inputHeight * inputWidth;
    for (int y = 0; y < inputHeight; ++y) {
        const uchar* line = resized.scanLine(y);
        for (int x = 0; x < inputWidth; ++x) {
            int idx = y * inputWidth + x;
            inputTensorValues[idx] = (line[x * 3] / 255.0f - mean[0]) / std[0];             // R
            inputTensorValues[imgSize + idx] = (line[x * 3 + 1] / 255.0f - mean[1]) / std[1]; // G
            inputTensorValues[imgSize * 2 + idx] = (line[x * 3 + 2] / 255.0f - mean[2]) / std[2]; // B
        }
    }

    if (profiler) profiler->endPhase();

    // 2. Инференс
    if (profiler) profiler->startPhase("Inference");

    const char* inputNames[] = {inputName.c_str()};
    const char* outputNames[] = {outputName.c_str()};

    try {
        session->Run(Ort::RunOptions{nullptr}, 
                     inputNames, &inputTensor.value(), 1, 
                     outputNames, &outputTensor.value(), 1);
    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX Runtime Exception during Run: " << e.what() << std::endl;
        if (profiler) profiler->endPhase();
        return QImage();
    }

    if (profiler) profiler->endPhase();

    // 3. Постпроцессинг
    if (profiler) profiler->startPhase("Postprocessing");

    float minVal = outputTensorValues[0];
    float maxVal = outputTensorValues[0];
    for (float val : outputTensorValues) {
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }
    
    bool is255 = (minVal >= 0.0f && maxVal > 2.0f && maxVal <= 255.1f);
    bool applySigmoid = (minVal < -1.0f || maxVal > 2.0f) && !is255;

    int outH = inputHeight;
    int outW = inputWidth;
    if (outputDims.size() >= 3) {
        outW = outputDims.back();
        outH = outputDims[outputDims.size() - 2];
    }

    QImage mask(outW, outH, QImage::Format_Grayscale8);
    for (int y = 0; y < outH; ++y) {
        uchar* line = mask.scanLine(y);
        for (int x = 0; x < outW; ++x) {
            int idx = y * outW + x;
            float val = outputTensorValues[idx];
            
            if (is255) {
                val /= 255.0f;
            } else if (applySigmoid) {
                val = 1.0f / (1.0f + std::exp(-val));
            }
            
            if (val < 0.0f) val = 0.0f;
            if (val > 1.0f) val = 1.0f;
            
            line[x] = static_cast<uchar>(val * 255.0f);
        }
    }

    if (profiler) profiler->endPhase();

    return mask;
}

QImage MLInferenceEngine::prepareModelInput(const QImage& original, const QSize& tensorSize, MLProfiler* profiler) {
    if (profiler) {
        profiler->startPhase("Preprocessing_Old");
    }
    QImage resized = original.scaled(tensorSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (profiler) {
        profiler->endPhase();
    }
    return resized;
}

QImage MLInferenceEngine::upscaleResult(const QImage& modelOutput, const QSize& originalSize, MLProfiler* profiler) {
    if (profiler) {
        profiler->startPhase("Postprocessing_Upscale");
    }
    QImage upscaled = modelOutput.scaled(originalSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (upscaled.format() != QImage::Format_Grayscale8) {
        upscaled = upscaled.convertToFormat(QImage::Format_Grayscale8);
    }
    if (profiler) {
        profiler->endPhase();
    }
    return upscaled;
}
