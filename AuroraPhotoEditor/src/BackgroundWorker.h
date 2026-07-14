#pragma once

#include <QObject>
#include <QImage>
#include <QSharedPointer>
#include <atomic>
#include "ImageEditorCommand.h"

class BackgroundWorker : public QObject {
    Q_OBJECT
public:
    explicit BackgroundWorker(QSharedPointer<ImageEditorCommand> command, const QImage& inputImage, QObject* parent = nullptr);
    virtual ~BackgroundWorker();

    // Сразу прервать выполнение
    void cancel();

    // Проверка, была ли запрошена отмена
    bool isCanceled() const;

public slots:
    void process();

signals:
    void success(const QImage& result);
    void canceled();

protected:
    std::atomic<bool> m_isCanceled;
    QSharedPointer<ImageEditorCommand> m_command;
    QImage m_inputImage;
};
