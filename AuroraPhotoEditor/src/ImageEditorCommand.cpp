#include "ImageEditorCommand.h"

void ImageEditorCommand::saveSnapshot(const QImage &workingCopy)
{
    m_snapshot = workingCopy.copy();
}

void ImageEditorCommand::undo(QImage &workingCopy)
{
    if (!m_snapshot.isNull()) {
        workingCopy = m_snapshot.copy();
    }
}
