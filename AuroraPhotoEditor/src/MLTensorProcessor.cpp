#include "MLTensorProcessor.h"
#include <cmath>
#include <algorithm>

namespace MLTensorProcessor {

std::vector<float> processInput(const QImage& original, int targetWidth, int targetHeight, NormalizationMode mode) {
    if (targetWidth <= 0 || targetHeight <= 0) return {};
    
    QImage resized = original.scaled(targetWidth, targetHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (resized.format() != QImage::Format_RGB888) {
        resized = resized.convertToFormat(QImage::Format_RGB888);
    }

    float mean[] = {0.485f, 0.456f, 0.406f};
    float std[] = {0.229f, 0.224f, 0.225f};
    
    if (mode == NormalizationMode::RMBG || targetWidth >= 1024) { 
        mean[0] = 0.5f; mean[1] = 0.5f; mean[2] = 0.5f;
        std[0] = 1.0f; std[1] = 1.0f; std[2] = 1.0f;
    }

    int imgSize = targetHeight * targetWidth;
    std::vector<float> inputTensorValues(3 * imgSize, 0.0f);

    for (int y = 0; y < targetHeight; ++y) {
        // Safe line access
        const uchar* line = resized.constScanLine(y);
        for (int x = 0; x < targetWidth; ++x) {
            int idx = y * targetWidth + x;
            
            int r_idx = idx;
            int g_idx = imgSize + idx;
            int b_idx = imgSize * 2 + idx;
            
            int pixelOffset = x * 3;
            
            if (mode == NormalizationMode::None) {
                inputTensorValues[r_idx] = static_cast<float>(line[pixelOffset]);
                inputTensorValues[g_idx] = static_cast<float>(line[pixelOffset + 1]);
                inputTensorValues[b_idx] = static_cast<float>(line[pixelOffset + 2]);
            } else {
                inputTensorValues[r_idx] = (line[pixelOffset] / 255.0f - mean[0]) / std[0];
                inputTensorValues[g_idx] = (line[pixelOffset + 1] / 255.0f - mean[1]) / std[1];
                inputTensorValues[b_idx] = (line[pixelOffset + 2] / 255.0f - mean[2]) / std[2];
            }
        }
    }

    return inputTensorValues;
}

QImage processOutput(const std::vector<float>& outputTensorValues, int outW, int outH) {
    if (outputTensorValues.empty() || outW <= 0 || outH <= 0) {
        return QImage();
    }
    
    float minVal = outputTensorValues[0];
    float maxVal = outputTensorValues[0];
    for (float val : outputTensorValues) {
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }
    
    bool is255 = (minVal >= 0.0f && maxVal > 2.0f && maxVal <= 255.1f);
    bool applySigmoid = (minVal < -1.0f || maxVal > 2.0f) && !is255;

    // Safety guard
    int safePixels = std::min(outW * outH, static_cast<int>(outputTensorValues.size()));

    QImage mask(outW, outH, QImage::Format_Grayscale8);
    mask.fill(0); 
    
    for (int y = 0; y < outH; ++y) {
        uchar* line = mask.scanLine(y);
        for (int x = 0; x < outW; ++x) {
            int idx = y * outW + x;
            if (idx >= safePixels) break; 
            
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

    return mask;
}

QImage processOutputRGB(const std::vector<float>& outputTensorValues, int outW, int outH) {
    if (outputTensorValues.empty() || outW <= 0 || outH <= 0) {
        return QImage();
    }
    
    int imgSize = outW * outH;
    int safePixels = std::min(imgSize, static_cast<int>(outputTensorValues.size() / 3));

    QImage result(outW, outH, QImage::Format_RGB888);
    result.fill(Qt::black);

    for (int y = 0; y < outH; ++y) {
        uchar* line = result.scanLine(y);
        for (int x = 0; x < outW; ++x) {
            int idx = y * outW + x;
            if (idx >= safePixels) break;
            
            int r_idx = idx;
            int g_idx = imgSize + idx;
            int b_idx = imgSize * 2 + idx;
            
            float r = outputTensorValues[r_idx];
            float g = outputTensorValues[g_idx];
            float b = outputTensorValues[b_idx];
            
            if (r < 0.0f) r = 0.0f;
            if (r > 255.0f) r = 255.0f;
            if (g < 0.0f) g = 0.0f;
            if (g > 255.0f) g = 255.0f;
            if (b < 0.0f) b = 0.0f;
            if (b > 255.0f) b = 255.0f;
            
            int pixelOffset = x * 3;
            line[pixelOffset] = static_cast<uchar>(r);
            line[pixelOffset + 1] = static_cast<uchar>(g);
            line[pixelOffset + 2] = static_cast<uchar>(b);
        }
    }

    return result;
}

} // namespace MLTensorProcessor
