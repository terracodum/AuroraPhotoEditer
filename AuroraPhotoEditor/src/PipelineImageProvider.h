#ifndef PIPELINEIMAGEPROVIDER_H
#define PIPELINEIMAGEPROVIDER_H

#include <QQuickImageProvider>
#include "PipelineManager.h"

class PipelineImageProvider : public QQuickImageProvider {
public:
    explicit PipelineImageProvider(PipelineManager* manager)
        : QQuickImageProvider(QQuickImageProvider::Image)
        , m_manager(manager) {}

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override {
        Q_UNUSED(id);
        Q_UNUSED(requestedSize);
        QImage img = m_manager->getCurrentImage();
        if (size) {
            *size = img.size();
        }
        return img;
    }

private:
    PipelineManager* m_manager;
};

#endif // PIPELINEIMAGEPROVIDER_H
