#include "BackgroundCommand.h"
#include "MLProfiler.h"
#include <auroraapp.h>
#include <QUrl>
#include <QDebug>
#include <QtConcurrent>
#include <QPainter>
#include <QLinearGradient>
#include <QMutex>
#include <memory>
#include <opencv2/opencv.hpp>

BackgroundCommand::BackgroundCommand() = default;
BackgroundCommand::~BackgroundCommand() = default;

QString BackgroundCommand::name() const {
    return "Background";
}

// Многопоточная функция попиксельного смешивания двух QImage на базе маски
static QImage blendTwoImages(const QImage& fg, const QImage& bg, const QImage& mask) {
    // Используем Premultiplied, так как формула смешивания с прозрачным фоном 
    // по сути выполняет pre-multiplication RGB каналов.
    QImage result(fg.size(), QImage::Format_ARGB32_Premultiplied);
    QImage fg32 = fg.convertToFormat(QImage::Format_ARGB32);
    QImage bg32 = bg.convertToFormat(QImage::Format_ARGB32);
    
    QVector<int> rows(result.height());
    std::iota(rows.begin(), rows.end(), 0);

    auto blendRow = [&](int y) {
        const QRgb* fgLine = reinterpret_cast<const QRgb*>(fg32.constScanLine(y));
        const QRgb* bgLine = reinterpret_cast<const QRgb*>(bg32.constScanLine(y));
        const uchar* maskLine = mask.constScanLine(y);
        QRgb* resultLine = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 0; x < result.width(); ++x) {
            int alpha = maskLine[x];
            int invAlpha = 255 - alpha;

            QRgb f = fgLine[x];
            QRgb b = bgLine[x];

            int red = (qRed(f) * alpha + qRed(b) * invAlpha) / 255;
            int green = (qGreen(f) * alpha + qGreen(b) * invAlpha) / 255;
            int blue = (qBlue(f) * alpha + qBlue(b) * invAlpha) / 255;
            int a = (qAlpha(f) * alpha + qAlpha(b) * invAlpha) / 255;

            resultLine[x] = qRgba(red, green, blue, a);
        }
    };

    // Распараллеливаем по строкам с помощью QtConcurrent
    QtConcurrent::blockingMap(rows, blendRow);
    return result;
}

namespace {
    std::unique_ptr<MLInferenceEngine> g_engine;
    QMutex g_engineMutex;
    bool g_isModelLoaded = false;
}

QImage BackgroundCommand::execute(const QImage& input, MLProfiler* profiler) const {
    if (input.isNull()) return input;

    QImage mask;

    // Check if we can reuse the cached mask
    if (m_cachedInput == input && !m_cachedMask.isNull()) {
        mask = m_cachedMask;
    } else {
        QMutexLocker locker(&g_engineMutex);
        if (!g_engine) {
            g_engine = std::make_unique<MLInferenceEngine>();
        }
        
        if (!g_isModelLoaded) {
            if (profiler) profiler->startPhase("LoadModel");
            
            QUrl modelUrl = Aurora::Application::pathTo(QStringLiteral("data/models/u2net.onnx"));
            QString modelPathStr = modelUrl.isLocalFile() ? modelUrl.toLocalFile() : modelUrl.toString();
            
            bool success = g_engine->loadModel(modelPathStr.toStdString());
            if (!success) {
                qWarning() << "Failed to load model from" << modelPathStr;
                if (profiler) profiler->endPhase();
                return input;
            }
            g_isModelLoaded = true;
            if (profiler) profiler->endPhase();
        }

        // Run Inference
        mask = g_engine->runInference(input, profiler);
        if (mask.isNull()) {
            qWarning() << "Inference returned a null mask";
            return input;
        }

        // Upscale the mask
        mask = MLInferenceEngine::upscaleResult(mask, input.size(), profiler);

        // Apply blur to mask edges for smoothing
        if (profiler) profiler->startPhase("BlurMask");
        cv::Mat maskMat(mask.height(), mask.width(), CV_8UC1, (void*)mask.bits(), mask.bytesPerLine());
        cv::GaussianBlur(maskMat, maskMat, cv::Size(21, 21), 0);
        if (profiler) profiler->endPhase();

        // Cache the result
        m_cachedInput = input;
        m_cachedMask = mask;
    }

    if (profiler) profiler->startPhase("ApplyMask");
    
    // Create the background image
    QImage bgImage(input.size(), QImage::Format_ARGB32);
    
    if (m_mode == ModeColor) {
        bgImage.fill(m_color1);
    } else if (m_mode == ModeGradient) {
        QPainter painter(&bgImage);
        QLinearGradient gradient(0, 0, bgImage.width(), bgImage.height());
        gradient.setColorAt(0.0, m_color1);
        gradient.setColorAt(1.0, m_color2);
        painter.fillRect(bgImage.rect(), gradient);
    } else if (m_mode == ModeBlur) {
        if (m_blurRadius > 0) {
            // Быстрое, но честное Гауссово размытие: уменьшаем, размываем, увеличиваем
            int scaleFactor = 4; // 4 - оптимальный баланс качества и скорости для 12 Мп
            QImage small = input.scaled(input.width() / scaleFactor, input.height() / scaleFactor, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            small = small.convertToFormat(QImage::Format_ARGB32);
            
            int ksize = m_blurRadius / scaleFactor;
            if (ksize > 0) {
                if (ksize % 2 == 0) ksize++; // ksize должен быть нечетным
                cv::Mat bgMat(small.height(), small.width(), CV_8UC4, (void*)small.bits(), small.bytesPerLine());
                cv::GaussianBlur(bgMat, bgMat, cv::Size(ksize, ksize), 0);
            }
            bgImage = small.scaled(input.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            bgImage = bgImage.convertToFormat(QImage::Format_ARGB32);
        } else {
            bgImage = input.convertToFormat(QImage::Format_ARGB32);
        }
    }

    // Многопоточное смешивание
    QImage resultImage = blendTwoImages(input, bgImage, mask);
    if (profiler) profiler->endPhase();

    return resultImage;
}
