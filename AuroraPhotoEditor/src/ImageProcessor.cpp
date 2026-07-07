#include "ImageProcessor.h"
#include <QImage>
#include <QPainter>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>

ImageProcessor::ImageProcessor(QObject *parent) : QObject(parent) {}

QString ImageProcessor::processImage(const QString &originalPath, const QString &maskPath, const QString &mode, const QColor &bgColor)
{
    QString oPath = originalPath;
    if (oPath.startsWith("file://")) oPath = oPath.mid(7);
    QString mPath = maskPath;
    if (mPath.startsWith("file://")) mPath = mPath.mid(7);

    QImage original(oPath);
    QImage mask(mPath);

    if (original.isNull() || mask.isNull()) return "";

    if (original.size() != mask.size()) {
        mask = mask.scaled(original.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    // Гарантируем Grayscale8 (1 байт на пиксель) для маски после возможного изменения формата при scaled()
    if (mask.format() != QImage::Format_Grayscale8) {
        mask = mask.convertToFormat(QImage::Format_Grayscale8);
    }

    QImage result(original.size(), QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    if (mode == "color") {
        result.fill(bgColor);
    } else if (mode == "blur") {
        // Fast blur by scaling down and up
        QImage small = original.scaled(original.width() / 16, original.height() / 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QImage blurred = small.scaled(original.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QPainter p(&result);
        p.drawImage(0, 0, blurred);
    }

    // Apply mask to original
    QImage fg = original.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < fg.height(); ++y) {
        QRgb *fgLine = reinterpret_cast<QRgb*>(fg.scanLine(y));
        const uchar *maskLine = mask.constScanLine(y);
        for (int x = 0; x < fg.width(); ++x) {
            int alpha = maskLine[x];
            // Apply alpha channel. We use ARGB32 (non-premultiplied) for easier modification, then painter will handle it
            fgLine[x] = (fgLine[x] & 0x00ffffff) | (alpha << 24);
        }
    }
    
    QPainter p(&result);
    // Draw foreground on top
    p.drawImage(0, 0, fg);
    p.end();

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(tempDir);
    QString resultFilePath = tempDir + "/result_" + QUuid::createUuid().toString().remove('{').remove('}') + ".png";
    
    if (result.save(resultFilePath)) {
        return resultFilePath;
    }
    return "";
}

QString ImageProcessor::processAdvancedBackground(const QString &originalPath, const QString &maskPath, const QVariantMap &settings)
{
    QString oPath = originalPath;
    if (oPath.startsWith("file://")) oPath = oPath.mid(7);
    QString mPath = maskPath;
    if (mPath.startsWith("file://")) mPath = mPath.mid(7);

    QImage original(oPath);
    QImage mask(mPath);

    if (original.isNull() || mask.isNull()) return "";

    if (original.size() != mask.size()) {
        mask = mask.scaled(original.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    if (mask.format() != QImage::Format_Grayscale8) {
        mask = mask.convertToFormat(QImage::Format_Grayscale8);
    }

    QImage result(original.size(), QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    QPainter p(&result);

    bool enableBlur = settings.value("enableBlur", false).toBool();
    bool enableColor = settings.value("enableColor", false).toBool();
    bool enableGradient = settings.value("enableGradient", false).toBool();

    if (enableBlur) {
        int blurRadius = settings.value("blurRadius", 10).toInt();
        int scaleFactor = qMax(2, blurRadius);
        QImage small = original.scaled(original.width() / scaleFactor, original.height() / scaleFactor, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QImage blurred = small.scaled(original.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        p.drawImage(0, 0, blurred);
    }

    if (enableColor) {
        QColor solidColor = qvariant_cast<QColor>(settings.value("solidColor", QColor(Qt::white)));
        p.fillRect(result.rect(), solidColor);
    }

    if (enableGradient) {
        int gradType = settings.value("gradientType", 0).toInt();
        QColor startC = qvariant_cast<QColor>(settings.value("gradStartColor", QColor(Qt::white)));
        QColor endC = qvariant_cast<QColor>(settings.value("gradEndColor", QColor(Qt::black)));
        
        if (gradType == 0) {
            QLinearGradient grad(0, 0, result.width(), result.height());
            grad.setColorAt(0.0, startC);
            grad.setColorAt(1.0, endC);
            p.fillRect(result.rect(), grad);
        } else {
            QRadialGradient grad(result.rect().center(), qMax(result.width(), result.height()) / 2.0);
            grad.setColorAt(0.0, startC);
            grad.setColorAt(1.0, endC);
            p.fillRect(result.rect(), grad);
        }
    }

    // Apply mask to original
    QImage fg = original.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < fg.height(); ++y) {
        QRgb *fgLine = reinterpret_cast<QRgb*>(fg.scanLine(y));
        const uchar *maskLine = mask.constScanLine(y);
        for (int x = 0; x < fg.width(); ++x) {
            int alpha = maskLine[x];
            fgLine[x] = (fgLine[x] & 0x00ffffff) | (alpha << 24);
        }
    }
    
    p.drawImage(0, 0, fg);
    p.end();

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(tempDir);
    QString resultFilePath = tempDir + "/result_" + QUuid::createUuid().toString().remove('{').remove('}') + ".png";
    
    if (result.save(resultFilePath)) {
        return resultFilePath;
    }
    return "";
}

bool ImageProcessor::saveToGallery(const QString &resultPath)
{
    QString rPath = resultPath;
    if (rPath.startsWith("file://")) rPath = rPath.mid(7);
    
    QString picturesDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir().mkpath(picturesDir);
    
    QString fileName = "AuroraEditor_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png";
    QString destPath = picturesDir + "/" + fileName;
    
    return QFile::copy(rPath, destPath);
}
