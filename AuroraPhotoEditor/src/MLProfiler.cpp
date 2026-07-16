#include "MLProfiler.h"
#include <QJsonDocument>
#include <QDebug>

void MLProfiler::startPhase(const QString& phaseName) {
    if (!m_currentPhase.isEmpty()) {
        endPhase();
    }
    m_currentPhase = phaseName;
    m_timer.start();
}

void MLProfiler::endPhase() {
    if (!m_currentPhase.isEmpty()) {
        m_phases[m_currentPhase] += m_timer.nsecsElapsed() / 1000;
        m_currentPhase.clear();
    }
}

QJsonObject MLProfiler::toJson() const {
    QJsonObject obj;
    qint64 total = 0;
    for (auto it = m_phases.constBegin(); it != m_phases.constEnd(); ++it) {
        obj[it.key()] = (qint64)it.value();
        total += it.value();
    }
    QJsonObject root;
    root["phases"] = obj;
    root["total_us"] = (qint64)total;
    return root;
}

void MLProfiler::dumpLog(const QString& commandName) const {
    QJsonObject logData = toJson();
    logData["event"] = "MLProfiler";
    logData["command"] = commandName;
    
    QJsonDocument doc(logData);
    qDebug().noquote() << doc.toJson(QJsonDocument::Indented);
}
