#pragma once

#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QHash>
#include <onnxruntime_c_api.h>
#include <onnxruntime_cxx_api.h>
#include <memory>

/**
 * @brief Singleton manager for ONNX Runtime inference sessions.
 *
 * Owns a single global Ort::Env instance (created once for the entire app lifecycle).
 * Caches Ort::Session objects by model path to avoid repeated loading and memory allocation.
 * Configures sessions with EnableCpuMemArena() and optimal thread counts for ARM big.LITTLE.
 *
 * Thread-safe: all access to sessions is protected by QMutex.
 *
 * Usage:
 *   auto* engine = MLInferenceEngine::instance();
 *   Ort::Session* session = engine->getSession("/path/to/model.onnx");
 *   // Use session for inference...
 */
class MLInferenceEngine {
public:
    static MLInferenceEngine* instance();

    /**
     * @brief Get or create an Ort::Session for the given model path.
     * Sessions are cached — subsequent calls with the same path return the existing session.
     * @param modelPath Absolute path to the .onnx model file.
     * @return Pointer to the session, or nullptr on failure.
     */
    Ort::Session* getSession(const QString &modelPath);

    /**
     * @brief Release a cached session (e.g., to free memory).
     * @param modelPath Path of the model whose session should be released.
     */
    void releaseSession(const QString &modelPath);

    /**
     * @brief Release all cached sessions.
     */
    void releaseAllSessions();

    /**
     * @brief Get the global Ort::Env reference.
     */
    Ort::Env& env();

    // Non-copyable, non-movable
    MLInferenceEngine(const MLInferenceEngine&) = delete;
    MLInferenceEngine& operator=(const MLInferenceEngine&) = delete;
    MLInferenceEngine(MLInferenceEngine&&) = delete;
    MLInferenceEngine& operator=(MLInferenceEngine&&) = delete;

private:
    MLInferenceEngine();
    ~MLInferenceEngine();

    Ort::SessionOptions createSessionOptions() const;

    std::unique_ptr<Ort::Env> m_env;
    QHash<QString, std::shared_ptr<Ort::Session>> m_sessions;
    mutable QMutex m_mutex;
};
