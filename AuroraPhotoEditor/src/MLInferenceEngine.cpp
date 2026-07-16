#include "MLInferenceEngine.h"
#include "MLProfiler.h"
#include "MLTensorProcessor.h"
#include <QPainter>
#include <cmath>
#include <iostream>

MLInferenceEngine::MLInferenceEngine() {
  try {
    // Initialize ONNX Runtime environment
    env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING,
                                     "MLInferenceEngine");

    memoryInfo.emplace(
        Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

    std::cout << "ONNX Runtime environment initialized successfully."
              << std::endl;
  } catch (const Ort::Exception &e) {
    std::cerr << "Failed to initialize ONNX Runtime environment: " << e.what()
              << std::endl;
  }
}

MLInferenceEngine::~MLInferenceEngine() {
  // Unique pointers and Optionals will automatically clean up the resources.
}

bool MLInferenceEngine::loadModel(const std::string &modelPath) {
  if (!env) {
    std::cerr << "ONNX Runtime environment is not initialized!" << std::endl;
    return false;
  }

  try {
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(4);
    sessionOptions.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    // Create the session
    session =
        std::make_unique<Ort::Session>(*env, modelPath.c_str(), sessionOptions);
    std::cout << "Successfully loaded ONNX model: " << modelPath << std::endl;

    Ort::AllocatorWithDefaultOptions allocator;

    // Log all outputs and find the one with the largest resolution (or same as
    // input)
    size_t num_outputs = session->GetOutputCount();
    size_t best_output_idx = 0;
    int64_t max_output_pixels = 0;

    std::cout << "Model has " << num_outputs << " outputs:" << std::endl;
    for (size_t i = 0; i < num_outputs; ++i) {
      auto out_name = session->GetOutputNameAllocated(i, allocator);
      auto type_info = session->GetOutputTypeInfo(i);
      auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
      std::vector<int64_t> shape = tensor_info.GetShape();

      std::cout << "  Output " << i << ": name=" << out_name.get()
                << " shape=[";
      int64_t pixels = 1;
      for (size_t j = 0; j < shape.size(); ++j) {
        std::cout << shape[j] << (j < shape.size() - 1 ? "," : "");
        if (shape[j] > 0)
          pixels *= shape[j];
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
      if (input_node_dims[0] <= 0)
        input_node_dims[0] = 1;
      inputChannels = input_node_dims[1];
      inputHeight = input_node_dims[2];
      inputWidth = input_node_dims[3];
      if (inputHeight <= 0) inputHeight = 512;
      if (inputWidth <= 0) inputWidth = 512;
    }
    inputDims = {1, static_cast<int64_t>(inputChannels),
                 static_cast<int64_t>(inputHeight),
                 static_cast<int64_t>(inputWidth)};

    auto output_name_alloc =
        session->GetOutputNameAllocated(best_output_idx, allocator);
    outputName = output_name_alloc.get();

    std::cout << "Selected output " << best_output_idx << ": " << outputName
              << std::endl;

    Ort::TypeInfo out_type_info = session->GetOutputTypeInfo(best_output_idx);
    auto out_tensor_info = out_type_info.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> output_node_dims = out_tensor_info.GetShape();

    for (size_t i = 0; i < output_node_dims.size(); ++i) {
      if (output_node_dims[i] <= 0) {
        if (i == 0)
          output_node_dims[i] = 1;
        else if (i == 1)
          output_node_dims[i] = 3; // Default to 3 channels if dynamic
        else if (i == 2)
          output_node_dims[i] = inputHeight;
        else if (i == 3)
          output_node_dims[i] = inputWidth;
      }
    }
    outputDims = output_node_dims;
    if (outputDims.size() >= 4) {
      outputChannels = outputDims[1];
    } else {
      outputChannels = 1;
    }

    size_t inputTensorSize = 1;
    for (auto d : inputDims)
      inputTensorSize *= d;

    size_t outputTensorSize = 1;
    for (auto d : outputDims)
      outputTensorSize *= d;

    std::cout << "Loaded model. Input dims: " << inputWidth << "x"
              << inputHeight << " Output elements: " << outputTensorSize
              << std::endl;

    return true;
  } catch (const Ort::Exception &e) {
    std::cerr << "Exception loading model " << modelPath << ": " << e.what()
              << std::endl;
    return false;
  }
}

QImage MLInferenceEngine::runInference(const QImage &original,
                                       MLProfiler *profiler) {
  if (!session || !memoryInfo) {
    std::cerr << "Session not initialized!" << std::endl;
    return QImage();
  }

  // 1. Препроцессинг
  if (profiler)
    profiler->startPhase("Preprocessing");

  std::vector<float> inputTensorValues = MLTensorProcessor::processInput(
      original, inputWidth, inputHeight, normMode);
  if (inputTensorValues.empty()) {
    if (profiler)
      profiler->endPhase();
    return QImage();
  }

  if (profiler)
    profiler->endPhase();

  // 2. Инференс
  if (profiler)
    profiler->startPhase("Inference");

  Ort::Value inputTensorLocal = Ort::Value::CreateTensor<float>(
      *memoryInfo, inputTensorValues.data(), inputTensorValues.size(),
      inputDims.data(), inputDims.size());

  const char *inputNames[] = {inputName.c_str()};
  const char *outputNames[] = {outputName.c_str()};
  std::vector<Ort::Value> outputTensors;

  try {
    outputTensors = session->Run(Ort::RunOptions{nullptr}, inputNames, &inputTensorLocal, 1,
                 outputNames, 1);
  } catch (const Ort::Exception &e) {
    std::cerr << "ONNX Runtime Exception during Run: " << e.what() << std::endl;
    if (profiler)
      profiler->endPhase();
    return QImage();
  }

  if (profiler)
    profiler->endPhase();

  // 3. Постпроцессинг
  if (profiler)
    profiler->startPhase("Postprocessing");

  float* out_data = outputTensors[0].GetTensorMutableData<float>();
  auto out_info = outputTensors[0].GetTensorTypeAndShapeInfo();
  auto actualOutputDims = out_info.GetShape();
  size_t outputElements = out_info.GetElementCount();
  std::vector<float> outputTensorValues(out_data, out_data + outputElements);

  int actualChannels = 1;
  int outH = inputHeight;
  int outW = inputWidth;

  if (actualOutputDims.size() >= 4) {
      actualChannels = actualOutputDims[1];
      outH = actualOutputDims[actualOutputDims.size() - 2];
      outW = actualOutputDims.back();
  } else if (actualOutputDims.size() == 3) {
      actualChannels = actualOutputDims[0];
      outH = actualOutputDims[1];
      outW = actualOutputDims[2];
  }

  QImage result;
  if (actualChannels == 3) {
    result =
        MLTensorProcessor::processOutputRGB(outputTensorValues, outW, outH);
  } else {
    result = MLTensorProcessor::processOutput(outputTensorValues, outW, outH);
  }

  if (profiler)
    profiler->endPhase();

  return result;
}

QImage MLInferenceEngine::prepareModelInput(const QImage &original,
                                            const QSize &tensorSize,
                                            MLProfiler *profiler) {
  if (profiler) {
    profiler->startPhase("Preprocessing_Old");
  }
  QImage resized = original.scaled(tensorSize, Qt::IgnoreAspectRatio,
                                   Qt::SmoothTransformation);
  if (profiler) {
    profiler->endPhase();
  }
  return resized;
}

QImage MLInferenceEngine::upscaleResult(const QImage &modelOutput,
                                        const QSize &originalSize,
                                        MLProfiler *profiler) {
  if (profiler) {
    profiler->startPhase("Postprocessing_Upscale");
  }
  QImage upscaled = modelOutput.scaled(originalSize, Qt::IgnoreAspectRatio,
                                       Qt::SmoothTransformation);
  if (upscaled.format() != QImage::Format_Grayscale8) {
    upscaled = upscaled.convertToFormat(QImage::Format_Grayscale8);
  }
  if (profiler) {
    profiler->endPhase();
  }
  return upscaled;
}

int MLInferenceEngine::getModelOutputChannels(const std::string &modelPath) {
  try {
    Ort::Env tempEnv(ORT_LOGGING_LEVEL_FATAL, "ChannelCheck");
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_DISABLE_ALL);
    Ort::Session tempSession(tempEnv, modelPath.c_str(), sessionOptions);

    Ort::AllocatorWithDefaultOptions allocator;
    size_t num_outputs = tempSession.GetOutputCount();

    size_t best_output_idx = 0;
    int64_t max_output_pixels = 0;

    for (size_t i = 0; i < num_outputs; ++i) {
      auto type_info = tempSession.GetOutputTypeInfo(i);
      auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
      std::vector<int64_t> shape = tensor_info.GetShape();

      int64_t pixels = 1;
      for (size_t j = 0; j < shape.size(); ++j) {
        if (shape[j] > 0)
          pixels *= shape[j];
      }
      if (pixels > max_output_pixels) {
        max_output_pixels = pixels;
        best_output_idx = i;
      }
    }

    auto type_info = tempSession.GetOutputTypeInfo(best_output_idx);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> shape = tensor_info.GetShape();

    if (shape.size() >= 4) {
      // A symbolic (unresolved) channel dim isn't necessarily RGB — U2Net/
      // RMBG-style segmentation exports declare *every* output dim
      // (including channels) symbolically too, and defaulting them to 3
      // here was misclassifying them as style-transfer models in
      // PipelineManager::getAvailableStyles(). Report "unknown" instead so
      // the == 3 check there safely excludes them.
      return shape[1] > 0 ? shape[1] : -1;
    }
    return 1;
  } catch (...) {
    return -1;
  }
}
