#include "MLInferenceEngine.h"
#include "MLProfiler.h"
#include <iostream>

MLInferenceEngine::MLInferenceEngine() {
    inputDims = {1, inputChannels, inputHeight, inputWidth};
    outputDims = {1, outputChannels, inputHeight, inputWidth};

    size_t inputTensorSize = inputChannels * inputHeight * inputWidth;
    size_t outputTensorSize = outputChannels * inputHeight * inputWidth;
    inputTensorValues.assign(inputTensorSize, 0.0f);
    outputTensorValues.assign(outputTensorSize, 0.0f);

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

        Ort::AllocatorWithDefaultOptions allocator;
        
        auto input_name_alloc = session->GetInputNameAllocated(0, allocator);
        inputName = input_name_alloc.get();
        
        auto output_name_alloc = session->GetOutputNameAllocated(0, allocator);
        outputName = output_name_alloc.get();

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
    
    QImage resized = original.scaled(inputWidth, inputHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    resized = resized.convertToFormat(QImage::Format_RGB888);

    // Заполнение входного тензора NCHW (нормализация 0..1)
    int imgSize = inputHeight * inputWidth;
    for (int y = 0; y < inputHeight; ++y) {
        const uchar* line = resized.scanLine(y);
        for (int x = 0; x < inputWidth; ++x) {
            int idx = y * inputWidth + x;
            inputTensorValues[idx] = line[x * 3] / 255.0f;             // R
            inputTensorValues[imgSize + idx] = line[x * 3 + 1] / 255.0f; // G
            inputTensorValues[imgSize * 2 + idx] = line[x * 3 + 2] / 255.0f; // B
        }
    }

    if (profiler) profiler->endPhase();

    // 2. Инференс
    if (profiler) profiler->startPhase("Inference");

    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo, inputTensorValues.data(), inputTensorValues.size(),
        inputDims.data(), inputDims.size());
        
    Ort::Value outputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo, outputTensorValues.data(), outputTensorValues.size(),
        outputDims.data(), outputDims.size());

    const char* inputNames[] = {inputName.c_str()};
    const char* outputNames[] = {outputName.c_str()};

    try {
        session->Run(Ort::RunOptions{nullptr}, 
                     inputNames, &inputTensor, 1, 
                     outputNames, &outputTensor, 1);
    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX Runtime Exception during Run: " << e.what() << std::endl;
        if (profiler) profiler->endPhase();
        return QImage();
    }

    if (profiler) profiler->endPhase();

    // 3. Постпроцессинг
    if (profiler) profiler->startPhase("Postprocessing");

    QImage mask(inputWidth, inputHeight, QImage::Format_Grayscale8);
    for (int y = 0; y < inputHeight; ++y) {
        uchar* line = mask.scanLine(y);
        for (int x = 0; x < inputWidth; ++x) {
            int idx = y * inputWidth + x;
            float val = outputTensorValues[idx];
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
    QImage resized = original.scaled(tensorSize, Qt::IgnoreAspectRatio, Qt::FastTransformation);
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
    if (profiler) {
        profiler->endPhase();
    }
    return upscaled;
}
