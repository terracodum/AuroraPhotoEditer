#pragma once

#include "BackgroundWorker.h"
#include <QStringList>
#include <QString>
#include <QJsonArray>

/**
 * @brief Worker for applying a sequence of commands to multiple images in batch mode.
 *
 * Runs completely asynchronously.
 * Deserializes JSON command list and for each image:
 * - Loads image
 * - Runs commands (Enhance, Style) sequentially
 * - Saves result to a new file in Pictures directory
 */
class BatchWorker : public BackgroundWorker {
    Q_OBJECT

public:
    /**
     * @param inputPaths List of image file paths to process.
     * @param serializedCommands JSON string produced by PipelineManager::serializeCommandStack().
     */
    BatchWorker(const QStringList &inputPaths, const QString &serializedCommands);

signals:
    void batchProgress(int current, int total);
    void batchFinished(int successCount, int failCount);

protected:
    void doWork() override;

private:
    QStringList m_inputPaths;
    QJsonArray m_commands;
};
