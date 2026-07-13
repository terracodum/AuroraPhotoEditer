#include "MLInferenceEngine.h"
#include <QDebug>

MLInferenceEngine* MLInferenceEngine::instance()
{
    static MLInferenceEngine s_instance;
    return &s_instance;
}

MLInferenceEngine::MLInferenceEngine()
{
    try {
        m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "AuroraPhotoEditor");
        qDebug() << "MLInferenceEngine: Global Ort::Env initialized";
    } catch (const Ort::Exception &e) {
        qCritical() << "MLInferenceEngine: Failed to initialize Ort::Env:" << e.what();
    }
}

MLInferenceEngine::~MLInferenceEngine()
{
    QMutexLocker locker(&m_mutex);
    m_sessions.clear();
    m_env.reset();
    qDebug() << "MLInferenceEngine: Destroyed, all sessions released";
}

Ort::SessionOptions MLInferenceEngine::createSessionOptions() const
{
    Ort::SessionOptions options;
    // Limit intra-op threads to 2 (optimal for ARM big.LITTLE — use only fast cores)
    options.SetIntraOpNumThreads(2);
    options.SetInterOpNumThreads(2);
    // Enable CPU memory arena to minimize heap fragmentation on mobile
    options.EnableCpuMemArena();
    // Enable all graph-level optimizations
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    return options;
}

Ort::Env& MLInferenceEngine::env()
{
    return *m_env;
}

Ort::Session* MLInferenceEngine::getSession(const QString &modelPath)
{
    QMutexLocker locker(&m_mutex);

    // Return cached session if it exists
    auto it = m_sessions.find(modelPath);
    if (it != m_sessions.end()) {
        return it.value().get();
    }

    // Create new session
    try {
        Ort::SessionOptions options = createSessionOptions();
        auto session = std::make_shared<Ort::Session>(
            *m_env,
            modelPath.toStdString().c_str(),
            options
        );
        Ort::Session* rawPtr = session.get();
        m_sessions.insert(modelPath, session);
        qDebug() << "MLInferenceEngine: Session loaded and cached:" << modelPath;
        return rawPtr;
    } catch (const Ort::Exception &e) {
        qWarning() << "MLInferenceEngine: Failed to load model:" << modelPath << "-" << e.what();
        return nullptr;
    }
}

void MLInferenceEngine::releaseSession(const QString &modelPath)
{
    QMutexLocker locker(&m_mutex);
    if (m_sessions.remove(modelPath)) {
        qDebug() << "MLInferenceEngine: Session released:" << modelPath;
    }
}

void MLInferenceEngine::releaseAllSessions()
{
    QMutexLocker locker(&m_mutex);
    int count = m_sessions.size();
    m_sessions.clear();
    qDebug() << "MLInferenceEngine: All sessions released, count:" << count;
}
