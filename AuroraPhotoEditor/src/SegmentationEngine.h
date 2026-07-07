#pragma once

#include <QObject>
#include <QString>
#include <onnxruntime_c_api.h>
#include <onnxruntime_cxx_api.h>
#include <memory>

class SegmentationEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
public:
    explicit SegmentationEngine(QObject *parent = nullptr);
    ~SegmentationEngine();

    bool isProcessing() const { return m_isProcessing; }

    Q_INVOKABLE void setModelPath(const QString &path);
    Q_INVOKABLE void process(const QString &imagePath);

signals:
    void isProcessingChanged();
    void segmentationDone(const QString &maskPath);
    void errorOccurred(const QString &error);

private slots:
    void onSegmentationSuccess(const QString &maskPath) {
        setIsProcessing(false);
        emit segmentationDone(maskPath);
    }
    void onSegmentationError(const QString &errorMsg) {
        setIsProcessing(false);
        emit errorOccurred(errorMsg);
    }

private:
    void processInternal(QString imagePath);
    void setIsProcessing(bool processing);

    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    QString m_modelPath;
    bool m_isProcessing;
};
