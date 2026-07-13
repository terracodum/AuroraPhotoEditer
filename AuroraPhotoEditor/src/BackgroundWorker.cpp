#include "BackgroundWorker.h"
#include <QDebug>
#include <stdexcept>

BackgroundWorker::BackgroundWorker(QObject *parent)
    : QObject(parent)
    , m_cancelled(false)
{
    // Do NOT autoDelete — PipelineManager manages lifetime
    setAutoDelete(false);
}

BackgroundWorker::~BackgroundWorker()
{
}

void BackgroundWorker::cancel()
{
    m_cancelled.store(true, std::memory_order_release);
    qDebug() << "BackgroundWorker: Cancellation requested";
}

bool BackgroundWorker::isCancelled() const
{
    return m_cancelled.load(std::memory_order_acquire);
}

void BackgroundWorker::run()
{
    try {
        doWork();
    } catch (const std::runtime_error &e) {
        QString msg = QString::fromStdString(e.what());
        if (msg == "cancelled") {
            qDebug() << "BackgroundWorker: Task cancelled";
        } else {
            qDebug() << "BackgroundWorker: Error:" << msg;
            emit error(msg);
        }
    } catch (const std::exception &e) {
        emit error(QString::fromStdString(e.what()));
    }
}

bool BackgroundWorker::checkCancelled()
{
    if (m_cancelled.load(std::memory_order_acquire)) {
        throw std::runtime_error("cancelled");
    }
    return true;
}

void BackgroundWorker::startPhaseTimer(const QString &phaseName)
{
    m_currentPhase = phaseName;
    m_phaseTimer.start();
}

void BackgroundWorker::endPhaseTimer()
{
    qint64 elapsed = m_phaseTimer.elapsed();
    qDebug() << "BackgroundWorker [" << m_currentPhase << "]:" << elapsed << "ms";
}
