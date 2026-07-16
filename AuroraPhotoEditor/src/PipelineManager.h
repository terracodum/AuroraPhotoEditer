#pragma once

#include <QObject>
#include <QImage>
#include <QColor>
#include <QList>
#include <QSharedPointer>
#include <QMutex>
#include <QThread>
#include <QVariantList>
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
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY commandStackChanged)

    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    // List of {"name": <command name>} objects, oldest first — backs the
    // History panel (design_handoff README, screen 7). Built from the same
    // command stack Undo/Reset already use, so it's real data, not mocked.
    Q_PROPERTY(QVariantList historySteps READ historySteps NOTIFY commandStackChanged)
    Q_PROPERTY(qreal filterStrength READ filterStrength WRITE setFilterStrength NOTIFY filterStrengthChanged)

public:
    explicit PipelineManager(QObject* parent = nullptr);
    ~PipelineManager() override;

    // Checks if a valid image is loaded. Thread-safe.
    bool hasImage() const;

    // Returns whether background processing is active
    bool isProcessing() const;

    // Loads image from local file or URI, respects EXIF, converts to RGB888.
    Q_INVOKABLE bool loadFromUri(const QString& uriString);

    // Sets the original image, resets the current image to this, and clears the command stack.
    void setOriginalImage(const QImage& image);

    // Returns a copy of the current working image. Thread-safe.
    QImage getCurrentImage() const;

    // Returns a copy of the original image. Thread-safe.
    QImage getOriginalImage() const;

    // Asynchronously executes the command on the current image.
    bool applyCommand(QSharedPointer<ImageEditorCommand> command);

    // Reverts the last applied command, restoring the previous image state. Thread-safe.
    Q_INVOKABLE bool undoLast();

    qreal filterStrength() const { return m_filterStrength; }
    void setFilterStrength(qreal strength);

    // Clears the command stack and resets the current image to the original. Thread-safe.
    Q_INVOKABLE void resetToOriginal();

    // Trigger commands from QML
    Q_INVOKABLE void applyBackgroundRemoval();
    // mode: 0=color, 1=gradient, 2=blur, 3=custom image (imageUri required).
    Q_INVOKABLE void updateBackground(int mode, const QColor& c1, const QColor& c2, int blurRadius, const QString& imageUri = QString());

    // Runs the built-in CLAHE + Gray World auto-enhance (EnhanceCommand) in
    // the background. This is the real wiring for the "Улучшение" tool —
    // QML cannot construct ImageEditorCommand subclasses itself.
    Q_INVOKABLE void applyEnhance();
    Q_INVOKABLE void applyStyle(const QString& modelName);
    
    // Returns a list of available styles dynamically loaded from the models directory
    Q_INVOKABLE QVariantList getAvailableStyles();

    // Returns the number of commands currently in the stack. Thread-safe.
    // Q_INVOKABLE so QML (PullDownMenu badge, Batch/History panels) can
    // call it directly.
    Q_INVOKABLE int commandCount() const;

    // Returns the command stack as a list of {"name": ...} maps, oldest
    // first, for display in the History panel. Thread-safe.
    QVariantList historySteps() const;

    // Returns whether there are any commands to undo.
    bool canUndo() const;

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

    // Emitted when processing state changes
    void isProcessingChanged();

    // Emitted when loadFromUri() rejects a file (oversized/corrupt), so the
    // UI can show a toast instead of silently doing nothing.
    void loadFailed(const QString& reason);

    // Emitted when a running command is superseded by a new one before it
    // finished, so the UI can show "previous operation canceled".
    void operationCanceled();

    // Emitted when the filter strength changes
    void filterStrengthChanged(qreal strength);

private:
    void setIsProcessing(bool processing);
    
    QImage blendImages(const QImage& bottom, const QImage& top, qreal alpha) const;

    struct StackElement {
        QSharedPointer<ImageEditorCommand> command;
        QImage resultImage;
    };

    mutable QMutex m_mutex;
    QImage m_original;
    QImage m_current;
    QList<StackElement> m_commandStack;

    QVariantList m_availableStylesCache;
    bool m_stylesCached = false;

    qreal m_filterStrength = 1.0;

    ExportWorker* m_exportWorker;
    QThread m_exportThread;

    class BackgroundWorker* m_activeWorker = nullptr;
    bool m_isProcessing = false;

    bool m_updatePending = false;
    int m_pendingMode = 0;
    QColor m_pendingC1;
    QColor m_pendingC2;
    int m_pendingBlur = 0;
    QString m_pendingImageUri;
};

