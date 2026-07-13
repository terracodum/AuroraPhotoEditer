#pragma once

#include <QImage>
#include <QString>
#include <QJsonObject>
#include <memory>

/**
 * @brief Abstract interface for all image editing operations (Command pattern).
 *
 * Each action (enhance, stylize, etc.) is encapsulated as a command with
 * execute() and undo() methods. Commands store a snapshot of the working copy
 * before execution to enable undo.
 *
 * Designed for integration with PipelineManager's command stack.
 */
class ImageEditorCommand {
public:
    virtual ~ImageEditorCommand() = default;

    /**
     * @brief Apply this editing operation to the working copy.
     * @param workingCopy Reference to the current working image (modified in-place).
     * @return true on success, false on failure.
     */
    virtual bool execute(QImage &workingCopy) = 0;

    /**
     * @brief Revert this operation by restoring the snapshot taken before execute().
     * @param workingCopy Reference to the current working image (restored in-place).
     */
    virtual void undo(QImage &workingCopy);

    /**
     * @brief Human-readable name for this command (used in UI and logging).
     */
    virtual QString name() const = 0;

    /**
     * @brief Serialize this command's parameters to JSON for batch processing.
     */
    virtual QJsonObject toJson() const = 0;

    /**
     * @brief Check if the command has been executed and snapshot is available.
     */
    bool hasSnapshot() const { return !m_snapshot.isNull(); }

protected:
    /**
     * @brief Save the current state of workingCopy before modifying it.
     * Must be called at the beginning of execute().
     */
    void saveSnapshot(const QImage &workingCopy);

    /**
     * @brief Get the saved snapshot (used by undo()).
     */
    const QImage& snapshot() const { return m_snapshot; }

private:
    QImage m_snapshot;
};
