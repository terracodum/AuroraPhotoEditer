#pragma once

#include <QObject>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QThreadPool>
#include <QList>
#include <QJsonArray>
#include <QJsonObject>
#include <memory>

#include "ImageEditorCommand.h"
#include "BackgroundWorker.h"

/**
 * @brief Central pipeline manager for the image editor (Command pattern coordinator).
 *
 * Manages:
 * - Original image (immutable after load) and working copy (mutated by commands)
 * - Command stack for undo/redo with QImage snapshots
 * - Background thread pool for async command execution
 * - Graceful shutdown with waitForDone()
 * - EXIF normalization on image load
 * - Serialization of command stack to JSON for batch processing
 *
 * Exposed to QML via setContextProperty() for property bindings.
 *
 * Thread-safety: all image data access is protected by QMutex.
 * Background workers communicate results via queued signals.
 */
class PipelineManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY commandStackChanged)
    Q_PROPERTY(bool canSave READ canSave NOTIFY commandStackChanged)
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY imageLoaded)
    Q_PROPERTY(int layoutTimestamp READ layoutTimestamp NOTIFY layoutChanged)
    Q_PROPERTY(int commandCount READ commandCount NOTIFY commandStackChanged)
    Q_PROPERTY(QString currentImagePath READ currentImagePath NOTIFY imageLoaded)

public:
    explicit PipelineManager(QObject *parent = nullptr);
    ~PipelineManager() override;

    bool isProcessing() const;
    bool canUndo() const;
    bool canSave() const;
    bool hasImage() const;
    int layoutTimestamp() const;
    int commandCount() const;
    QString currentImagePath() const;

    /**
     * @brief Get a copy of the current working image (thread-safe).
     * Used by EditorImageProvider to supply images to QML.
     */
    QImage workingCopy();

    /**
     * @brief Get a copy of the original (unmodified) image (thread-safe).
     */
    QImage originalImage();

    // --- QML-invokable methods ---

    /**
     * @brief Load an image from path, normalize EXIF orientation, store as original + working copy.
     */
    Q_INVOKABLE void loadImage(const QString &path);

    /**
     * @brief Apply an enhancement command (CLAHE, Gray World, Auto-Contrast).
     * Runs asynchronously in a background worker.
     */
    Q_INVOKABLE void applyEnhance(bool enableContrast, bool enableWhiteBalance,
                                   bool enableClahe, double clipLimit);

    /**
     * @brief Apply a style transfer command.
     * Runs asynchronously in a background worker.
     * @param modelPath Path to the .onnx style model.
     */
    Q_INVOKABLE void applyStyle(const QString &modelPath);

    /**
     * @brief Blend the working copy with the cached styled layer using the given opacity.
     * Does NOT run inference — uses the cached result from applyStyle().
     * @param opacity Blend factor 0.0 (original) to 1.0 (fully styled).
     */
    Q_INVOKABLE void blendStyle(double opacity);

    /**
     * @brief Undo the last command in the stack.
     */
    Q_INVOKABLE void undoLast();

    /**
     * @brief Reset working copy to the original image and clear the command stack.
     */
    Q_INVOKABLE void resetToOriginal();

    /**
     * @brief Save the working copy to the Pictures directory.
     * Asynchronous, emits saveDone/saveError on completion.
     */
    Q_INVOKABLE void saveResult();

    /**
     * @brief Serialize the current command stack to JSON for batch processing.
     * @return JSON string representing the command sequence.
     */
    Q_INVOKABLE QString serializeCommandStack() const;

    /**
     * @brief Cancel the currently running background task.
     */
    Q_INVOKABLE void cancelProcessing();

    /**
     * @brief Start batch processing on the given images with the serialized command stack.
     */
    Q_INVOKABLE void startBatchProcessing(const QStringList &paths, const QString &serializedCommands);

    /**
     * @brief Save the current command stack as a project.
     */
    Q_INVOKABLE void saveProject(const QString &projectName);

    /**
     * @brief Get a list of saved projects (names and info).
     * Returns a list of QVariantMap with keys: name, date, summary
     */
    Q_INVOKABLE QVariantList getSavedProjects() const;

    /**
     * @brief Load a project's command stack by name.
     * Returns the serialized JSON string.
     */
    Q_INVOKABLE QString loadProject(const QString &projectName) const;

signals:
    void isProcessingChanged();
    void commandStackChanged();
    void imageLoaded();
    void layoutChanged();

    void saveDone(const QString &filePath);
    void saveError(const QString &errorMessage);
    void processingError(const QString &errorMessage);
    void batchProgress(int current, int total);
    void batchFinished(int successCount, int failCount);

private slots:
    void onWorkerFinished(const QImage &result);
    void onWorkerError(const QString &message);
    void onWorkerProgress(int percent);

private:
    void setIsProcessing(bool processing);
    void updateLayout();
    void cleanupWorker();

    /**
     * @brief Normalize image: apply EXIF orientation and convert to ARGB32.
     */
    static QImage normalizeImage(const QString &path);

    QImage m_original;
    QImage m_workingCopy;

    /** Snapshot taken before the most recent style transfer, used for blendStyle(). */
    QImage m_preStyleSnapshot;
    /** Cached styled result for opacity blending without re-inference. */
    QImage m_cachedStyledResult;

    struct CommandEntry {
        std::shared_ptr<ImageEditorCommand> command;
        QImage snapshotBefore;  // State before this command was applied
    };
    QList<CommandEntry> m_commandStack;

    QThreadPool m_threadPool;
    BackgroundWorker *m_currentWorker;

    mutable QMutex m_dataMutex;
    bool m_isProcessing;
    int m_layoutTimestamp;
    QString m_currentImagePath;
};
