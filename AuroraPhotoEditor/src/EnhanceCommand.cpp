#include "EnhanceCommand.h"
#include "MLProfiler.h"
#include <opencv2/opencv.hpp>
#include <QDebug>

EnhanceCommand::EnhanceCommand() = default;
EnhanceCommand::~EnhanceCommand() = default;

QString EnhanceCommand::name() const {
    return "Enhance";
}

QImage EnhanceCommand::execute(const QImage& input, MLProfiler* profiler) const {
    if (input.isNull()) return input;

    if (profiler) profiler->startPhase("FormatConversion_QImageToMat");

    // Convert QImage to cv::Mat (RGB). image is a fresh detached copy, so
    // bits() hands back writable memory without a deep copy and avoids a
    // const_cast; cvtColor only reads it anyway.
    QImage image = input.convertToFormat(QImage::Format_RGB888);
    cv::Mat rgbMat(image.height(), image.width(), CV_8UC3,
                   image.bits(), image.bytesPerLine());

    if (profiler) profiler->startPhase("Adaptive_Contrast");
    
    // 1. Adaptive Soft Contrast (Gentle CLAHE)
    cv::Mat labMat;
    cv::cvtColor(rgbMat, labMat, cv::COLOR_RGB2Lab);
    
    std::vector<cv::Mat> labChannels;
    cv::split(labMat, labChannels);
    
    // Lowered clip limit to 1.2 for a gentle shadow/highlight recovery
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(1.2, cv::Size(8, 8));
    clahe->apply(labChannels[0], labChannels[0]);
    
    cv::merge(labChannels, labMat);
    
    cv::Mat enhancedRgb;
    cv::cvtColor(labMat, enhancedRgb, cv::COLOR_Lab2RGB);
    
    if (profiler) profiler->startPhase("Smart_Vibrancy_Boost");

    // 2. Smart Vibrancy Boost
    cv::Mat hsvMat;
    cv::cvtColor(enhancedRgb, hsvMat, cv::COLOR_RGB2HSV);
    std::vector<cv::Mat> hsvChannels;
    cv::split(hsvMat, hsvChannels);
    
    // Create a non-linear Look-Up Table (LUT) for saturation
    // It boosts muted colors (low saturation) more than already vibrant colors
    uchar lut[256];
    float vibranceFactor = 0.35f; // 35% max boost
    for (int i = 0; i < 256; i++) {
        // Curve: S_new = S_old + (255 - S_old) * (S_old/255) * factor
        float normalizedS = i / 255.0f;
        float boost = (1.0f - normalizedS) * normalizedS * 255.0f * vibranceFactor;
        int s = static_cast<int>(i + boost);
        lut[i] = cv::saturate_cast<uchar>(s);
    }
    cv::LUT(hsvChannels[1], cv::Mat(1, 256, CV_8UC1, lut), hsvChannels[1]);
    
    cv::merge(hsvChannels, hsvMat);
    cv::cvtColor(hsvMat, enhancedRgb, cv::COLOR_HSV2RGB);
    
    if (profiler) profiler->startPhase("Unsharp_Mask");

    // 3. Crispness (Unsharp Mask)
    cv::Mat blurred;
    cv::GaussianBlur(enhancedRgb, blurred, cv::Size(0, 0), 3.0);
    cv::Mat sharpened;
    // sharpened = original + (original - blurred) * amount
    // Here we use addWeighted: 1.5 * original - 0.5 * blurred
    cv::addWeighted(enhancedRgb, 1.3, blurred, -0.3, 0, sharpened);

    if (profiler) profiler->startPhase("FormatConversion_MatToQImage");

    // Convert back to QImage
    QImage resultImage((const uchar*)sharpened.data,
                       sharpened.cols, sharpened.rows,
                       sharpened.step,
                       QImage::Format_RGB888);
    
    // Deep copy because sharpened memory will be destroyed when it goes out of scope
    QImage finalImage = resultImage.copy();

    if (profiler) profiler->endPhase();

    return finalImage;
}
