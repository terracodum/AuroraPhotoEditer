#include "PipelineManager.h"
#include <QMutexLocker>
#include <QUrl>
#include <QImageReader>
#include <QDebug>

PipelineManager::PipelineManager(QObject* parent)
    : QObject(parent) {
}

PipelineManager::~PipelineManager() {
}

bool PipelineManager::hasImage() const {
    QMutexLocker locker(&m_mutex);
    return !m_current.isNull();
}

bool PipelineManager::loadFromUri(const QString& uriString) {
    QUrl url(uriString);
    QString localFile = url.isLocalFile() ? url.toLocalFile() : uriString;

    QImageReader reader(localFile);
    reader.setAutoTransform(true); // Account for EXIF orientation

    QImage image = reader.read();
    if (image.isNull()) {
        qWarning() << "Failed to read image:" << reader.errorString();
        return false;
    }

    // Convert to normalized RGB format to prevent blue tint (RGB888)
    image = image.convertToFormat(QImage::Format_RGB888);

    setOriginalImage(image);
    return true;
}



void PipelineManager::setOriginalImage(const QImage& image) {
    QImage prevCurrent;
    QImage prevOriginal;
    bool currentChanged = false;
    bool originalChanged = false;
    bool stackChanged = false;

    {
        QMutexLocker locker(&m_mutex);
        if (m_original != image) {
            m_original = image;
            prevOriginal = m_original;
            originalChanged = true;
        }
        if (m_current != image) {
            m_current = image;
            prevCurrent = m_current;
            currentChanged = true;
        }
        if (!m_commandStack.isEmpty()) {
            m_commandStack.clear();
            stackChanged = true;
        }
    }

    if (originalChanged) {
        emit originalImageChanged(prevOriginal);
    }
    if (currentChanged) {
        emit currentImageChanged(prevCurrent);
    }
    if (stackChanged) {
        emit commandStackChanged();
    }
}

QImage PipelineManager::getCurrentImage() const {
    QMutexLocker locker(&m_mutex);
    return m_current;
}

QImage PipelineManager::getOriginalImage() const {
    QMutexLocker locker(&m_mutex);
    return m_original;
}

bool PipelineManager::applyCommand(QSharedPointer<ImageEditorCommand> command) {
    if (!command) {
        return false;
    }

    QImage nextImage;
    {
        QMutexLocker locker(&m_mutex);
        nextImage = command->execute(m_current);
        m_current = nextImage;
        m_commandStack.append({command, nextImage});
    }

    emit currentImageChanged(nextImage);
    emit commandStackChanged();
    return true;
}

bool PipelineManager::undoLast() {
    QImage newCurrent;
    bool changed = false;

    {
        QMutexLocker locker(&m_mutex);
        if (!m_commandStack.isEmpty()) {
            m_commandStack.removeLast();
            if (m_commandStack.isEmpty()) {
                newCurrent = m_original;
            } else {
                newCurrent = m_commandStack.last().resultImage;
            }
            m_current = newCurrent;
            changed = true;
        }
    }

    if (changed) {
        emit currentImageChanged(newCurrent);
        emit commandStackChanged();
        return true;
    }
    return false;
}

void PipelineManager::resetToOriginal() {
    QImage newCurrent;
    bool changed = false;

    {
        QMutexLocker locker(&m_mutex);
        if (!m_commandStack.isEmpty() || m_current != m_original) {
            m_commandStack.clear();
            m_current = m_original;
            newCurrent = m_current;
            changed = true;
        }
    }

    if (changed) {
        emit currentImageChanged(newCurrent);
        emit commandStackChanged();
    }
}

int PipelineManager::commandCount() const {
    QMutexLocker locker(&m_mutex);
    return m_commandStack.size();
}
