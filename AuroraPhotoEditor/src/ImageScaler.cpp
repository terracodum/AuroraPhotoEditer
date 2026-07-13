#include "ImageScaler.h"
#include <cmath>
#include <algorithm>
#include <QDebug>
#include <QElapsedTimer>

QImage ImageScaler::prepareModelInput(const QImage &source, int targetSize)
{
    QElapsedTimer timer;
    timer.start();

    int origW = source.width();
    int origH = source.height();
    int targetW = origW;
    int targetH = origH;

    if (origW > targetSize || origH > targetSize) {
        if (origW >= origH) {
            targetW = targetSize;
            targetH = static_cast<int>(static_cast<float>(origH) / origW * targetSize);
        } else {
            targetH = targetSize;
            targetW = static_cast<int>(static_cast<float>(origW) / origH * targetSize);
        }
    }

    // Ensure even dimensions (some models require this)
    targetW = (targetW / 2) * 2;
    targetH = (targetH / 2) * 2;
    if (targetW < 2) targetW = 2;
    if (targetH < 2) targetH = 2;

    QImage result = source.scaled(targetW, targetH,
        Qt::IgnoreAspectRatio, Qt::FastTransformation);

    qDebug() << "ImageScaler::prepareModelInput:" << origW << "x" << origH
             << "->" << targetW << "x" << targetH
             << "in" << timer.elapsed() << "ms";
    return result;
}

QImage ImageScaler::prepareModelInputExact(const QImage &source, int width, int height)
{
    QElapsedTimer timer;
    timer.start();

    QImage result = source.scaled(width, height,
        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    qDebug() << "ImageScaler::prepareModelInputExact:"
             << source.width() << "x" << source.height()
             << "->" << width << "x" << height
             << "in" << timer.elapsed() << "ms";
    return result;
}

QImage ImageScaler::upscaleResult(const QImage &result, const QSize &originalSize)
{
    QElapsedTimer timer;
    timer.start();

    QImage upscaled = result.scaled(originalSize,
        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    qDebug() << "ImageScaler::upscaleResult:"
             << result.width() << "x" << result.height()
             << "->" << originalSize.width() << "x" << originalSize.height()
             << "in" << timer.elapsed() << "ms";
    return upscaled;
}

QImage ImageScaler::fastBlur(const QImage &image, int radius)
{
    if (radius <= 1) return image;

    int scaleFactor = std::max(2, radius);
    QImage small = image.scaled(
        std::max(1, image.width() / scaleFactor),
        std::max(1, image.height() / scaleFactor),
        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    return small.scaled(image.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QImage ImageScaler::extractHighFrequency(const QImage &image, int blurRadius)
{
    QImage src = image;
    if (src.format() != QImage::Format_ARGB32) {
        src = src.convertToFormat(QImage::Format_ARGB32);
    }

    QImage blurred = fastBlur(src, blurRadius);
    if (blurred.format() != QImage::Format_ARGB32) {
        blurred = blurred.convertToFormat(QImage::Format_ARGB32);
    }

    const int w = src.width();
    const int h = src.height();

    // High-frequency = original - blurred, stored with 128 offset
    QImage highFreq(w, h, QImage::Format_ARGB32);

    for (int y = 0; y < h; ++y) {
        const QRgb *srcLine = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        const QRgb *blurLine = reinterpret_cast<const QRgb*>(blurred.constScanLine(y));
        QRgb *outLine = reinterpret_cast<QRgb*>(highFreq.scanLine(y));

        for (int x = 0; x < w; ++x) {
            // delta = original - blurred + 128 (offset to keep in [0,255])
            int dr = qRed(srcLine[x])   - qRed(blurLine[x])   + 128;
            int dg = qGreen(srcLine[x]) - qGreen(blurLine[x]) + 128;
            int db = qBlue(srcLine[x])  - qBlue(blurLine[x])  + 128;

            dr = std::max(0, std::min(255, dr));
            dg = std::max(0, std::min(255, dg));
            db = std::max(0, std::min(255, db));

            outLine[x] = qRgba(dr, dg, db, 255);
        }
    }

    return highFreq;
}

QImage ImageScaler::compositeWithDetails(const QImage &stylized, const QImage &original,
                                          int blurRadius)
{
    QElapsedTimer timer;
    timer.start();

    QImage styledImg = stylized;
    if (styledImg.format() != QImage::Format_ARGB32) {
        styledImg = styledImg.convertToFormat(QImage::Format_ARGB32);
    }

    QImage origImg = original;
    if (origImg.format() != QImage::Format_ARGB32) {
        origImg = origImg.convertToFormat(QImage::Format_ARGB32);
    }

    // Ensure same size
    if (styledImg.size() != origImg.size()) {
        styledImg = styledImg.scaled(origImg.size(),
            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    // Extract luminance high-frequency from original
    QImage highFreq = extractHighFrequency(origImg, blurRadius);

    const int w = origImg.width();
    const int h = origImg.height();
    QImage result(w, h, QImage::Format_ARGB32);

    for (int y = 0; y < h; ++y) {
        const QRgb *styledLine = reinterpret_cast<const QRgb*>(styledImg.constScanLine(y));
        const QRgb *hfLine = reinterpret_cast<const QRgb*>(highFreq.constScanLine(y));
        QRgb *outLine = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 0; x < w; ++x) {
            // Extract luminance delta from HF layer (remove the 128 offset)
            int hfR = qRed(hfLine[x]) - 128;
            int hfG = qGreen(hfLine[x]) - 128;
            int hfB = qBlue(hfLine[x]) - 128;

            // Compute luminance delta: dY = 0.299*dR + 0.587*dG + 0.114*dB
            int lumDelta = static_cast<int>(0.299 * hfR + 0.587 * hfG + 0.114 * hfB);

            // Add luminance detail to styled pixel
            int r = std::max(0, std::min(255, qRed(styledLine[x]) + lumDelta));
            int g = std::max(0, std::min(255, qGreen(styledLine[x]) + lumDelta));
            int b = std::max(0, std::min(255, qBlue(styledLine[x]) + lumDelta));

            outLine[x] = qRgba(r, g, b, 255);
        }
    }

    qDebug() << "ImageScaler::compositeWithDetails:" << w << "x" << h
             << "in" << timer.elapsed() << "ms";
    return result;
}
