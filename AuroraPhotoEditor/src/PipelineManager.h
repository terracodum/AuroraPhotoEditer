#pragma once

#include <QObject>
#include <QImage>
#include <QList>
#include <QSharedPointer>
#include <QMutex>
#include <QThread>
#include "ImageEditorCommand.h"

class ExportWorker : public QObject {
    Q_OBJECT
public:
    explicit ExportWorker(QObject* parent = nullptr);

public slots:
    void exportImage(const QImage& image);

signals:
    void exportCompleted(bool success, const QString& filePath);
};

class PipelineManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY currentImageChanged)

public:
    explicit PipelineManager(QObject* parent = nullptr);
    ~PipelineManager() override;

    // Checks if a valid image is loaded. Thread-safe.
    bool hasImage() const;

    // Loads image from local file or URI, respects EXIF, converts to RGB888.
    Q_INVOKABLE bool loadFromUri(const QString& uriString);

    // Sets the original image, resets the current image to this, and clears the command stack.
    void setOriginalImage(const QImage& image);

    // Returns a copy of the current working image. Thread-safe.
    QImage getCurrentImage() const;

    // Returns a copy of the original image. Thread-safe.
    QImage getOriginalImage() const;

    // Executes the command on the current image, adds it to the command stack,
    // and updates the working image. Thread-safe.
    bool applyCommand(QSharedPointer<ImageEditorCommand> command);

    // Reverts the last applied command, restoring the previous image state. Thread-safe.
    bool undoLast();

    // Clears the command stack and resets the current image to the original. Thread-safe.
    void resetToOriginal();

    // Returns the number of commands currently in the stack. Thread-safe.
    int commandCount() const;

    // Asynchronously exports the current image to PicturesLocation
    Q_INVOKABLE void exportImage();

signals:
    void exportRequested(const QImage& image);
    // Emitted when the working image is updated.
    void currentImageChanged(const QImage& image);

    // Emitted when the original image is set/updated.
    void originalImageChanged(const QImage& image);

    // Emitted when commands are added or removed from the stack.
    void commandStackChanged();

    // Emitted when the image export completes.
    void exportCompleted(bool success, const QString& filePath);

private:
    struct StackElement {
        QSharedPointer<ImageEditorCommand> command;
        QImage resultImage;
    };

    mutable QMutex m_mutex;
    QImage m_original;
    QImage m_current;
    QList<StackElement> m_commandStack;

    ExportWorker* m_exportWorker;
    QThread m_exportThread;
};
