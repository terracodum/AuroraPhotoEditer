#pragma once

#include <QQuickImageProvider>
#include <QImage>

class PipelineManager;

/**
 * @brief QQuickImageProvider that serves the current working copy from PipelineManager.
 *
 * Registered in QML engine as "editor", enabling QML Image elements to use:
 *   source: "image://editor/working_copy?" + pipelineManager.layoutTimestamp
 *
 * The timestamp query parameter forces QML to invalidate its image cache
 * and request a fresh frame after each edit operation.
 *
 * Thread-safe: delegates to PipelineManager::workingCopy() which is mutex-protected.
 */
class EditorImageProvider : public QQuickImageProvider {
public:
    explicit EditorImageProvider(PipelineManager *pipeline);

    /**
     * @brief Called by QML engine when an image with "image://editor/..." URL is requested.
     * @param id The image ID (e.g., "working_copy" or "original").
     * @param size Output parameter for the image dimensions.
     * @param requestedSize Requested size (ignored — we return full resolution).
     * @return The current working copy QImage.
     */
    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

private:
    PipelineManager *m_pipeline;
};
