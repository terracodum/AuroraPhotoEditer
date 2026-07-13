#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#include <atomic>

/**
 * @brief Engine for neural style transfer using Fast Neural Style ONNX models.
 *
 * Supports pre-trained style models from ONNX Model Zoo (candy, mosaic, udnie, etc.).
 * Each model is ~6.6 MB and produces high-quality stylized output.
 *
 * Uses MLInferenceEngine singleton for session management (shared Ort::Env).
 *
 * Thread-safe: inference runs via QtConcurrent::run(),
 * results are delivered via queued signals to the GUI thread.
 *
 * This class is retained for backward compatibility with existing QML pages.
 * New code should prefer PipelineManager::applyStyle() for Command pattern integration.
 */
class StyleTransferEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)

public:
    explicit StyleTransferEngine(QObject *parent = nullptr);
    ~StyleTransferEngine();

    bool isProcessing() const { return m_isProcessing; }

    /** Load an ONNX style transfer model via MLInferenceEngine. */
    Q_INVOKABLE void setModelPath(const QString &path);

    /** Run style transfer on the given image. Model must be loaded first. */
    Q_INVOKABLE void process(const QString &imagePath);

    /** 
     * @brief Run style transfer synchronously and return the result.
     * Accessible by BackgroundWorker for reuse.
     * @param cancelFlag Optional flag to abort processing early.
     */
    static QImage applyStyleTransfer(const QImage &source, const QString &modelPath, std::atomic<bool> *cancelFlag = nullptr);

signals:
    void isProcessingChanged();
    void styleDone(const QString &resultPath);
    void errorOccurred(const QString &error);

private slots:
    void onStyleSuccess(const QString &resultPath) {
        setIsProcessing(false);
        emit styleDone(resultPath);
    }
    void onStyleError(const QString &errorMsg) {
        setIsProcessing(false);
        emit errorOccurred(errorMsg);
    }

private:
    void processInternal(QString imagePath);
    void setIsProcessing(bool processing);

    QString m_modelPath;
    bool m_isProcessing;
};
