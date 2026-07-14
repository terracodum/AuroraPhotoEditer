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

    if (profiler) profiler->startPhase("LAB_Conversion_and_CLAHE");

    // Convert RGB to LAB
    cv::Mat labMat;
    cv::cvtColor(rgbMat, labMat, cv::COLOR_RGB2Lab);

    // Split channels
    std::vector<cv::Mat> labChannels;
    cv::split(labMat, labChannels);

    // Apply CLAHE to L channel
    // Clip limit 2.0 and grid size 8x8 are good defaults for shadow/highlight recovery
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    clahe->apply(labChannels[0], labChannels[0]);

    // Merge channels back
    cv::merge(labChannels, labMat);

    // Convert LAB back to RGB
    cv::Mat claheRgbMat;
    cv::cvtColor(labMat, claheRgbMat, cv::COLOR_Lab2RGB);

    if (profiler) profiler->startPhase("WhiteBalance_Correction");

    // White balance correction (Gray World Assumption)
    cv::Scalar means = cv::mean(claheRgbMat);
    double meanR = means[0];
    double meanG = means[1];
    double meanB = means[2];

    double overallMean = (meanR + meanG + meanB) / 3.0;

    double scaleR = (meanR > 0) ? (overallMean / meanR) : 1.0;
    double scaleG = (meanG > 0) ? (overallMean / meanG) : 1.0;
    double scaleB = (meanB > 0) ? (overallMean / meanB) : 1.0;

    std::vector<cv::Mat> rgbChannels;
    cv::split(claheRgbMat, rgbChannels);

    // Apply scaling
    rgbChannels[0].convertTo(rgbChannels[0], CV_8U, scaleR);
    rgbChannels[1].convertTo(rgbChannels[1], CV_8U, scaleG);
    rgbChannels[2].convertTo(rgbChannels[2], CV_8U, scaleB);

    cv::Mat resultRgbMat;
    cv::merge(rgbChannels, resultRgbMat);

    if (profiler) profiler->startPhase("FormatConversion_MatToQImage");

    // Convert back to QImage
    QImage resultImage((const uchar*)resultRgbMat.data,
                       resultRgbMat.cols, resultRgbMat.rows,
                       resultRgbMat.step,
                       QImage::Format_RGB888);
    
    // Deep copy because resultRgbMat memory will be destroyed
    QImage finalImage = resultImage.copy();

    if (profiler) profiler->endPhase();

    return finalImage;
}
