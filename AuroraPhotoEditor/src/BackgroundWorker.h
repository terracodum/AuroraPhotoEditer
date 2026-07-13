#pragma once

#include <QObject>
#include <QRunnable>
#include <QImage>
#include <QElapsedTimer>
#include <atomic>

/**
 * @brief Base class for all background processing tasks.
 *
 * Combines QRunnable (for QThreadPool submission) and QObject (for signals).
 * Provides:
 * - Cancellation via atomic flag (checked between processing steps)
 * - Progress reporting via signal
 * - QElapsedTimer-based profiling for each phase
 * - Automatic cleanup after completion (setAutoDelete)
 *
 * Subclasses must implement doWork() and call checkCancelled() periodically.
 *
 * Thread-safety: m_cancelled is std::atomic, signals are delivered via
 * Qt::QueuedConnection to the GUI thread.
 */
class BackgroundWorker : public QObject, public QRunnable {
    Q_OBJECT

public:
    explicit BackgroundWorker(QObject *parent = nullptr);
    ~BackgroundWorker() override;

    /**
     * @brief Request cancellation of the running task.
     * The task will stop at the next checkCancelled() call.
     */
    void cancel();

    /**
     * @brief Check if cancellation has been requested.
     */
    bool isCancelled() const;

    /**
     * @brief QRunnable entry point — delegates to doWork().
     */
    void run() override;

signals:
    /**
     * @brief Emitted to report processing progress.
     * @param percent Progress value 0-100.
     */
    void progress(int percent);

    /**
     * @brief Emitted on successful completion with the result image.
     */
    void finished(const QImage &result);

    /**
     * @brief Emitted on error.
     */
    void error(const QString &message);

protected:
    /**
     * @brief Override this to implement the actual processing logic.
     * Must call checkCancelled() between major steps.
     */
    virtual void doWork() = 0;

    /**
     * @brief Check cancellation and throw if cancelled.
     * Call this between processing phases (pre/inference/post).
     * @return true if NOT cancelled (safe to continue).
     * @throws std::runtime_error if cancelled.
     */
    bool checkCancelled();

    /**
     * @brief Start a profiling timer for a named phase.
     */
    void startPhaseTimer(const QString &phaseName);

    /**
     * @brief Stop the current phase timer and log the elapsed time.
     */
    void endPhaseTimer();

    std::atomic<bool> m_cancelled;

private:
    QElapsedTimer m_phaseTimer;
    QString m_currentPhase;
};
