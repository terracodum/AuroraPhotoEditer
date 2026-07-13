#include "EnhanceEngine.h"
#include <QDebug>
#include <QtConcurrent>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <cmath>
#include <algorithm>
#include <vector>

EnhanceEngine::EnhanceEngine(QObject *parent)
    : QObject(parent), m_isProcessing(false)
{
}

void EnhanceEngine::setIsProcessing(bool processing)
{
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        emit isProcessingChanged();
    }
}

void EnhanceEngine::process(const QString &imagePath,
                            bool enableContrast,
                            bool enableWhiteBalance,
                            bool enableClahe,
                            double clipLimit)
{
    if (m_isProcessing) {
        return;
    }

    setIsProcessing(true);

    QtConcurrent::run([this, imagePath, enableContrast, enableWhiteBalance, enableClahe, clipLimit]() {
        this->processInternal(imagePath, enableContrast, enableWhiteBalance, enableClahe, clipLimit);
    });
}

void EnhanceEngine::processInternal(QString imagePath,
                                    bool enableContrast,
                                    bool enableWhiteBalance,
                                    bool enableClahe,
                                    double clipLimit)
{
    if (imagePath.startsWith("file://")) {
        imagePath = imagePath.mid(7);
    }

    QImage image;
    if (!image.load(imagePath)) {
        QMetaObject::invokeMethod(this, "onEnhanceError",
            Qt::QueuedConnection,
            Q_ARG(QString, "Failed to load image"));
        return;
    }

    // Convert to ARGB32 for uniform pixel access
    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    try {
        // Pipeline: Contrast → White Balance → CLAHE
        if (enableContrast) {
            qDebug() << "EnhanceEngine: Applying auto-contrast (Histogram Clipping)";
            applyAutoContrast(image, 1.0f);
        }

        if (enableWhiteBalance) {
            qDebug() << "EnhanceEngine: Applying Gray World white balance";
            applyGrayWorldWB(image);
        }

        if (enableClahe) {
            qDebug() << "EnhanceEngine: Applying CLAHE, clipLimit =" << clipLimit;
            applyCLAHE(image, clipLimit, 8);
        }

        // Save result
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir().mkpath(tempDir);
        QString resultPath = tempDir + "/enhance_" +
            QUuid::createUuid().toString().remove('{').remove('}') + ".png";

        if (image.save(resultPath)) {
            QMetaObject::invokeMethod(this, "onEnhanceSuccess",
                Qt::QueuedConnection,
                Q_ARG(QString, resultPath));
        } else {
            throw std::runtime_error("Failed to save enhanced image");
        }

    } catch (const std::exception &e) {
        QString errorMsg = QString::fromStdString(e.what());
        QMetaObject::invokeMethod(this, "onEnhanceError",
            Qt::QueuedConnection,
            Q_ARG(QString, errorMsg));
    }
}

// =============================================================================
// Algorithm 1: Histogram Clipping Auto-Contrast
// =============================================================================
// For each R, G, B channel independently:
//   1. Build a histogram (256 bins)
//   2. Find the low/high cutoff values that clip `clipPercent`% of pixels
//   3. Linearly remap the remaining range to [0..255]
// This removes outlier dark/bright pixels and stretches the usable range,
// making the image more vibrant and contrasty.
// =============================================================================

void EnhanceEngine::applyAutoContrast(QImage &image, float clipPercent)
{
    const int w = image.width();
    const int h = image.height();
    const int totalPixels = w * h;

    // Build histogram for each channel
    int histR[256] = {0};
    int histG[256] = {0};
    int histB[256] = {0};

    for (int y = 0; y < h; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            histR[qRed(line[x])]++;
            histG[qGreen(line[x])]++;
            histB[qBlue(line[x])]++;
        }
    }

    // Find clip boundaries for a single channel
    // Returns (lowVal, highVal) — the pixel values at clipPercent% from each tail
    auto findClipBounds = [&](const int hist[256]) -> std::pair<int, int> {
        int clipCount = static_cast<int>(totalPixels * clipPercent / 100.0f);

        int lowVal = 0;
        int cumLow = 0;
        for (int i = 0; i < 256; ++i) {
            cumLow += hist[i];
            if (cumLow > clipCount) {
                lowVal = i;
                break;
            }
        }

        int highVal = 255;
        int cumHigh = 0;
        for (int i = 255; i >= 0; --i) {
            cumHigh += hist[i];
            if (cumHigh > clipCount) {
                highVal = i;
                break;
            }
        }

        if (lowVal >= highVal) {
            lowVal = 0;
            highVal = 255;
        }

        return std::make_pair(lowVal, highVal);
    };

    auto boundsR = findClipBounds(histR);
    auto boundsG = findClipBounds(histG);
    auto boundsB = findClipBounds(histB);

    // Build lookup tables for fast remapping
    uchar lutR[256], lutG[256], lutB[256];

    auto buildLUT = [](uchar lut[256], int low, int high) {
        float scale = (high > low) ? 255.0f / (high - low) : 1.0f;
        for (int i = 0; i < 256; ++i) {
            int val = static_cast<int>((std::max(low, std::min(high, i)) - low) * scale);
            lut[i] = static_cast<uchar>(std::max(0, std::min(255, val)));
        }
    };

    buildLUT(lutR, boundsR.first, boundsR.second);
    buildLUT(lutG, boundsG.first, boundsG.second);
    buildLUT(lutB, boundsB.first, boundsB.second);

    // Apply LUT
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = lutR[qRed(line[x])];
            int g = lutG[qGreen(line[x])];
            int b = lutB[qBlue(line[x])];
            int a = qAlpha(line[x]);
            line[x] = qRgba(r, g, b, a);
        }
    }
}

// =============================================================================
// Algorithm 2: Gray World White Balance
// =============================================================================
// Assumption: the average color of a natural scene is gray.
// Steps:
//   1. Compute average R, G, B across all pixels
//   2. Compute overall average luminance = (avgR + avgG + avgB) / 3
//   3. Scale each channel: pixel_R *= (avgLum / avgR), etc.
// This effectively removes color casts (too blue, too yellow).
// =============================================================================

void EnhanceEngine::applyGrayWorldWB(QImage &image)
{
    const int w = image.width();
    const int h = image.height();
    const long long totalPixels = static_cast<long long>(w) * h;

    if (totalPixels == 0) return;

    // Accumulate channel sums
    long long sumR = 0, sumG = 0, sumB = 0;

    for (int y = 0; y < h; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            sumR += qRed(line[x]);
            sumG += qGreen(line[x]);
            sumB += qBlue(line[x]);
        }
    }

    double avgR = static_cast<double>(sumR) / totalPixels;
    double avgG = static_cast<double>(sumG) / totalPixels;
    double avgB = static_cast<double>(sumB) / totalPixels;

    // Overall average luminance
    double avgLum = (avgR + avgG + avgB) / 3.0;

    // Compute scaling factors (avoid division by zero)
    double scaleR = (avgR > 0.001) ? avgLum / avgR : 1.0;
    double scaleG = (avgG > 0.001) ? avgLum / avgG : 1.0;
    double scaleB = (avgB > 0.001) ? avgLum / avgB : 1.0;

    qDebug() << "GrayWorld: avgR=" << avgR << "avgG=" << avgG << "avgB=" << avgB
             << "scales:" << scaleR << scaleG << scaleB;

    // Apply scaling
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = std::min(255, std::max(0, static_cast<int>(qRed(line[x])   * scaleR)));
            int g = std::min(255, std::max(0, static_cast<int>(qGreen(line[x]) * scaleG)));
            int b = std::min(255, std::max(0, static_cast<int>(qBlue(line[x])  * scaleB)));
            int a = qAlpha(line[x]);
            line[x] = qRgba(r, g, b, a);
        }
    }
}

// =============================================================================
// Algorithm 3: CLAHE (Contrast Limited Adaptive Histogram Equalization)
// =============================================================================
// Unlike global histogram equalization, CLAHE works on local tiles:
//   1. Convert to YCbCr — work on Y (luminance) channel only
//   2. Divide the Y channel into gridSize×gridSize tiles
//   3. For each tile, build a histogram and clip it at clipLimit
//   4. Redistribute excess counts equally across all bins
//   5. Compute CDF (cumulative distribution) for each tile
//   6. For each pixel, bilinearly interpolate between the 4 nearest tile CDFs
//   7. Convert back to RGB
// This pulls detail out of shadows without blowing highlights.
// =============================================================================

void EnhanceEngine::applyCLAHE(QImage &image, double clipLimit, int gridSize)
{
    const int w = image.width();
    const int h = image.height();

    if (w == 0 || h == 0 || gridSize <= 0) return;

    // Step 1: Extract luminance channel (Y from YCbCr)
    // Y = 0.299*R + 0.587*G + 0.114*B
    std::vector<uchar> luminance(w * h);
    std::vector<uchar> cbChannel(w * h);
    std::vector<uchar> crChannel(w * h);

    for (int y = 0; y < h; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = qRed(line[x]);
            int g = qGreen(line[x]);
            int b = qBlue(line[x]);

            // RGB -> YCbCr (ITU-R BT.601)
            int Y  = static_cast<int>( 0.299 * r + 0.587 * g + 0.114 * b);
            int Cb = static_cast<int>(-0.169 * r - 0.331 * g + 0.500 * b + 128);
            int Cr = static_cast<int>( 0.500 * r - 0.419 * g - 0.081 * b + 128);

            luminance[y * w + x] = static_cast<uchar>(std::max(0, std::min(255, Y)));
            cbChannel[y * w + x] = static_cast<uchar>(std::max(0, std::min(255, Cb)));
            crChannel[y * w + x] = static_cast<uchar>(std::max(0, std::min(255, Cr)));
        }
    }

    // Step 2: Compute tile dimensions
    int tileW = (w + gridSize - 1) / gridSize;
    int tileH = (h + gridSize - 1) / gridSize;

    // Step 3: Build clipped histograms and CDFs for each tile
    // CDFs are stored as lookup tables [0..255] -> [0..255]
    int totalTiles = gridSize * gridSize;
    std::vector<std::vector<uchar>> tileCDFs(totalTiles, std::vector<uchar>(256, 0));

    for (int ty = 0; ty < gridSize; ++ty) {
        for (int tx = 0; tx < gridSize; ++tx) {
            int x0 = tx * tileW;
            int y0 = ty * tileH;
            int x1 = std::min(x0 + tileW, w);
            int y1 = std::min(y0 + tileH, h);
            int tilePixels = (x1 - x0) * (y1 - y0);

            if (tilePixels <= 0) continue;

            // Build histogram for this tile
            int hist[256] = {0};
            for (int py = y0; py < y1; ++py) {
                for (int px = x0; px < x1; ++px) {
                    hist[luminance[py * w + px]]++;
                }
            }

            // Clip histogram at clipLimit (proportional to tile size)
            int clipThreshold = static_cast<int>(clipLimit * tilePixels / 256.0);
            if (clipThreshold < 1) clipThreshold = 1;

            int excess = 0;
            for (int i = 0; i < 256; ++i) {
                if (hist[i] > clipThreshold) {
                    excess += hist[i] - clipThreshold;
                    hist[i] = clipThreshold;
                }
            }

            // Redistribute excess equally
            int avgIncrease = excess / 256;
            int remainder = excess % 256;
            for (int i = 0; i < 256; ++i) {
                hist[i] += avgIncrease;
                if (i < remainder) {
                    hist[i]++;
                }
            }

            // Compute CDF and normalize to [0..255]
            int cdf[256];
            cdf[0] = hist[0];
            for (int i = 1; i < 256; ++i) {
                cdf[i] = cdf[i - 1] + hist[i];
            }

            int cdfMin = 0;
            for (int i = 0; i < 256; ++i) {
                if (cdf[i] > 0) { cdfMin = cdf[i]; break; }
            }

            int tileIdx = ty * gridSize + tx;
            float denom = static_cast<float>(std::max(1, tilePixels - cdfMin));
            for (int i = 0; i < 256; ++i) {
                float val = static_cast<float>(cdf[i] - cdfMin) / denom * 255.0f;
                tileCDFs[tileIdx][i] = static_cast<uchar>(std::max(0.0f, std::min(255.0f, val)));
            }
        }
    }

    // Step 4: Bilinear interpolation — for each pixel, blend the 4 nearest tile CDFs
    for (int py = 0; py < h; ++py) {
        for (int px = 0; px < w; ++px) {
            uchar lum = luminance[py * w + px];

            // Find the fractional tile position (center of each tile)
            float ftx = (static_cast<float>(px) / tileW) - 0.5f;
            float fty = (static_cast<float>(py) / tileH) - 0.5f;

            int tx1 = static_cast<int>(std::floor(ftx));
            int ty1 = static_cast<int>(std::floor(fty));
            int tx2 = tx1 + 1;
            int ty2 = ty1 + 1;

            // Clamp to valid tile indices
            tx1 = std::max(0, std::min(gridSize - 1, tx1));
            ty1 = std::max(0, std::min(gridSize - 1, ty1));
            tx2 = std::max(0, std::min(gridSize - 1, tx2));
            ty2 = std::max(0, std::min(gridSize - 1, ty2));

            float fx = ftx - std::floor(ftx);
            float fy = fty - std::floor(fty);

            // Get CDF values from 4 neighboring tiles
            float v11 = tileCDFs[ty1 * gridSize + tx1][lum];
            float v12 = tileCDFs[ty2 * gridSize + tx1][lum];
            float v21 = tileCDFs[ty1 * gridSize + tx2][lum];
            float v22 = tileCDFs[ty2 * gridSize + tx2][lum];

            // Bilinear interpolation
            float result = v11 * (1 - fx) * (1 - fy) +
                           v21 * fx * (1 - fy) +
                           v12 * (1 - fx) * fy +
                           v22 * fx * fy;

            luminance[py * w + px] = static_cast<uchar>(std::max(0.0f, std::min(255.0f, result)));
        }
    }

    // Step 5: Convert back from YCbCr to RGB
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int Y  = luminance[y * w + x];
            int Cb = cbChannel[y * w + x] - 128;
            int Cr = crChannel[y * w + x] - 128;

            // YCbCr -> RGB (ITU-R BT.601)
            int r = static_cast<int>(Y + 1.402 * Cr);
            int g = static_cast<int>(Y - 0.344 * Cb - 0.714 * Cr);
            int b = static_cast<int>(Y + 1.772 * Cb);

            r = std::max(0, std::min(255, r));
            g = std::max(0, std::min(255, g));
            b = std::max(0, std::min(255, b));

            int a = qAlpha(line[x]);
            line[x] = qRgba(r, g, b, a);
        }
    }
}
