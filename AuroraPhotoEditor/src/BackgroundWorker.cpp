#include "BackgroundWorker.h"
#include "MLProfiler.h"

BackgroundWorker::BackgroundWorker(QSharedPointer<ImageEditorCommand> command, const QImage& inputImage, QObject* parent)
    : QObject(parent)
    , m_isCanceled(false)
    , m_command(command)
    , m_inputImage(inputImage)
{
}

BackgroundWorker::~BackgroundWorker()
{
}

void BackgroundWorker::process()
{
    if (m_isCanceled.load(std::memory_order_relaxed)) {
        emit canceled();
        return;
    }

    if (m_command) {
        MLProfiler profiler;
        QImage result = m_command->execute(m_inputImage, &profiler);
        
        profiler.dumpLog(m_command->name());
        
        if (m_isCanceled.load(std::memory_order_relaxed)) {
            emit canceled();
            return;
        }
        
        emit success(result);
    } else {
        emit canceled();
    }
}

void BackgroundWorker::cancel()
{
    m_isCanceled.store(true, std::memory_order_relaxed);
    emit canceled();
}

bool BackgroundWorker::isCanceled() const
{
    return m_isCanceled.load(std::memory_order_relaxed);
}
