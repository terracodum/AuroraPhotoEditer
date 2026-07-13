#pragma once

#include <QObject>
#include <QString>
#include <QImage>

/**
 * @brief Engine for automatic image enhancement without neural networks.
 *
 * Implements three classical image processing algorithms:
 * 1. Histogram Clipping Auto-Contrast — removes "grayness" by stretching histogram
 * 2. Gray World White Balance — removes color cast
 * 3. CLAHE — adaptive contrast enhancement for dark/low-contrast images
 *
 * Thread-safe: heavy processing runs via QtConcurrent::run(),
 * results are delivered via queued signals to the GUI thread.
 */
class EnhanceEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)

public:
    explicit EnhanceEngine(QObject *parent = nullptr);

    bool isProcessing() const { return m_isProcessing; }

    /**
     * @brief Run enhancement pipeline on the given image.
     * @param imagePath Path to the source image (file:// prefix is stripped automatically).
     * @param enableContrast Enable Histogram Clipping auto-contrast.
     * @param enableWhiteBalance Enable Gray World white balance.
     * @param enableClahe Enable CLAHE adaptive contrast.
     * @param clipLimit CLAHE clip limit (typical range 1.0–4.0, default 2.0).
     */
    Q_INVOKABLE void process(const QString &imagePath,
                             bool enableContrast = true,
                             bool enableWhiteBalance = true,
                             bool enableClahe = false,
                             double clipLimit = 2.0);

signals:
    void isProcessingChanged();
    void enhanceDone(const QString &resultPath);
    void errorOccurred(const QString &error);

private slots:
    void onEnhanceSuccess(const QString &resultPath) {
        setIsProcessing(false);
        emit enhanceDone(resultPath);
    }
    void onEnhanceError(const QString &errorMsg) {
        setIsProcessing(false);
        emit errorOccurred(errorMsg);
    }

public:
    // --- Public static image processing algorithms ---
    // Accessible by EnhanceWorker and other components for reuse.

    /** Histogram Clipping: trim 1% tails per channel, stretch to [0..255]. */
    static void applyAutoContrast(QImage &image, float clipPercent = 1.0f);

    /** Gray World: equalize average R, G, B channel values. */
    static void applyGrayWorldWB(QImage &image);

    /** CLAHE on luminance channel (YCbCr space), with given clip limit and grid size. */
    static void applyCLAHE(QImage &image, double clipLimit = 2.0, int gridSize = 8);

private:
    void processInternal(QString imagePath,
                         bool enableContrast,
                         bool enableWhiteBalance,
                         bool enableClahe,
                         double clipLimit);

    void setIsProcessing(bool processing);

    bool m_isProcessing;
};
