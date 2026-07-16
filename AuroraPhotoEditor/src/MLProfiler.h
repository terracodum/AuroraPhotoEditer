#pragma once

#include <QString>
#include <QMap>
#include <QElapsedTimer>
#include <QJsonObject>

class MLProfiler {
public:
    MLProfiler() = default;
    ~MLProfiler() = default;

    void startPhase(const QString& phaseName);
    void endPhase();

    QJsonObject toJson() const;
    void dumpLog(const QString& commandName) const;

private:
    QElapsedTimer m_timer;
    QString m_currentPhase;
    QMap<QString, qint64> m_phases;
};
