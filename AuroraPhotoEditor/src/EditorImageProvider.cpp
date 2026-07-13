#include "EditorImageProvider.h"
#include "PipelineManager.h"
#include <QDebug>

EditorImageProvider::EditorImageProvider(PipelineManager *pipeline)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_pipeline(pipeline)
{
}

QImage EditorImageProvider::requestImage(const QString &id, QSize *size,
                                          const QSize &requestedSize)
{
    Q_UNUSED(requestedSize)

    // Strip any query parameters (e.g., "working_copy?12345" → "working_copy")
    QString cleanId = id.split("?").first();

    QImage image;

    if (cleanId == "working_copy") {
        image = m_pipeline->workingCopy();
    } else if (cleanId == "original") {
        image = m_pipeline->originalImage();
    } else {
        qWarning() << "EditorImageProvider: Unknown image id:" << cleanId;
    }

    if (image.isNull()) {
        // Return a small transparent placeholder if no image loaded
        image = QImage(1, 1, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
    }

    if (size) {
        *size = image.size();
    }

    return image;
}
