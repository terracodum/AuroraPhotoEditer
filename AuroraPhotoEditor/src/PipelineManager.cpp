#include "PipelineManager.h"
#include "EnhanceEngine.h"
#include "ImageScaler.h"
#include "MLInferenceEngine.h"
#include "StyleTransferEngine.h"
#include "BatchWorker.h"

#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QtConcurrent>

#include <cmath>
#include <algorithm>

// ============================================================================
// Concrete workers (defined internally — SRP: each worker does one job)
// ============================================================================

/**
 * @brief Worker for applying enhance algorithms in a background thread.
 */
class EnhanceWorker : public BackgroundWorker {
    Q_OBJECT
public:
    EnhanceWorker(const QImage &input, bool contrast, bool wb, bool clahe, double clipLimit)
        : m_input(input), m_contrast(contrast), m_wb(wb), m_clahe(clahe), m_clipLimit(clipLimit) {}

protected:
    void doWork() override {
        QImage image = m_input;
        if (image.format() != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
        }

        emit progress(10);

        if (m_contrast) {
            startPhaseTimer("AutoContrast");
            checkCancelled();
            EnhanceEngine::applyAutoContrast(image, 1.0f);
            endPhaseTimer();
            emit progress(30);
        }

        if (m_wb) {
            startPhaseTimer("GrayWorldWB");
            checkCancelled();
            EnhanceEngine::applyGrayWorldWB(image);
            endPhaseTimer();
            emit progress(60);
        }

        if (m_clahe) {
            startPhaseTimer("CLAHE");
            checkCancelled();
            EnhanceEngine::applyCLAHE(image, m_clipLimit, 8);
            endPhaseTimer();
            emit progress(90);
        }

        checkCancelled();
        emit progress(100);
        emit finished(image);
    }

private:
    QImage m_input;
    bool m_contrast, m_wb, m_clahe;
    double m_clipLimit;
};

/**
 * @brief Worker for running ONNX style transfer inference in a background thread.
 *
 * Implements the full pipeline: downscale → tensor prep → inference → decode → upscale → composite.
 */
class StyleTransferWorker : public BackgroundWorker {
    Q_OBJECT
public:
    StyleTransferWorker(const QImage &input, const QString &modelPath)
        : m_input(input), m_modelPath(modelPath) {}

protected:
    void doWork() override {
        startPhaseTimer("Preprocessing");
        emit progress(5);

        emit progress(10);
        
        QImage composited;
        try {
            composited = StyleTransferEngine::applyStyleTransfer(m_input, m_modelPath, &m_cancelled);
        } catch (const std::exception &e) {
            QString msg = QString::fromStdString(e.what());
            if (msg == "cancelled") {
                // BackgroundWorker handles cancellation internally if we just return
                return;
            }
            emit error(msg);
            return;
        }

        endPhaseTimer();
        emit progress(100);
        emit finished(composited);
    }

private:
    QImage m_input;
    QString m_modelPath;
};

// ============================================================================
// Lightweight command metadata classes for serialization (no execute logic)
// ============================================================================

/**
 * @brief Metadata-only command for enhance operations (used for serialization + undo tracking).
 * Actual execution happens in EnhanceWorker.
 */
class EnhanceCommandMeta : public ImageEditorCommand {
public:
    EnhanceCommandMeta(bool contrast, bool wb, bool clahe, double clipLimit)
        : m_contrast(contrast), m_wb(wb), m_clahe(clahe), m_clipLimit(clipLimit) {}

    bool execute(QImage &) override { return true; }  // Execution done by worker
    QString name() const override { return "enhance"; }

    QJsonObject toJson() const override {
        QJsonObject obj;
        obj["cmd"] = "enhance";
        obj["contrast"] = m_contrast;
        obj["whiteBalance"] = m_wb;
        obj["clahe"] = m_clahe;
        obj["clipLimit"] = m_clipLimit;
        return obj;
    }

private:
    bool m_contrast, m_wb, m_clahe;
    double m_clipLimit;
};

/**
 * @brief Metadata-only command for style transfer operations.
 */
class StyleCommandMeta : public ImageEditorCommand {
public:
    explicit StyleCommandMeta(const QString &modelPath) : m_modelPath(modelPath) {}

    bool execute(QImage &) override { return true; }
    QString name() const override { return "style"; }

    QJsonObject toJson() const override {
        QJsonObject obj;
        obj["cmd"] = "style";
        obj["modelPath"] = m_modelPath;
        return obj;
    }

private:
    QString m_modelPath;
};

// ============================================================================
// PipelineManager implementation
// ============================================================================

PipelineManager::PipelineManager(QObject *parent)
    : QObject(parent)
    , m_currentWorker(nullptr)
    , m_isProcessing(false)
    , m_layoutTimestamp(0)
{
    // Configure thread pool: limit to 1 concurrent heavy task
    m_threadPool.setMaxThreadCount(1);

    // Ensure global ONNX Runtime env is initialized
    MLInferenceEngine::instance();
}

PipelineManager::~PipelineManager()
{
    // Graceful shutdown (ISSUE-2.2)
    if (m_currentWorker) {
        m_currentWorker->cancel();
    }
    m_threadPool.waitForDone(3000);
    cleanupWorker();

    qDebug() << "PipelineManager: Destroyed, all threads stopped";
}

bool PipelineManager::isProcessing() const
{
    return m_isProcessing;
}

bool PipelineManager::canUndo() const
{
    QMutexLocker locker(&m_dataMutex);
    return !m_commandStack.isEmpty();
}

bool PipelineManager::canSave() const
{
    QMutexLocker locker(&m_dataMutex);
    return !m_workingCopy.isNull() && !m_commandStack.isEmpty();
}

bool PipelineManager::hasImage() const
{
    QMutexLocker locker(&m_dataMutex);
    return !m_original.isNull();
}

int PipelineManager::layoutTimestamp() const
{
    return m_layoutTimestamp;
}

int PipelineManager::commandCount() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_commandStack.size();
}

QString PipelineManager::currentImagePath() const
{
    QMutexLocker locker(&m_dataMutex);
    return m_currentImagePath;
}

QImage PipelineManager::workingCopy()
{
    QMutexLocker locker(&m_dataMutex);
    return m_workingCopy;
}

QImage PipelineManager::originalImage()
{
    QMutexLocker locker(&m_dataMutex);
    return m_original;
}

void PipelineManager::setIsProcessing(bool processing)
{
    if (m_isProcessing != processing) {
        m_isProcessing = processing;
        emit isProcessingChanged();
    }
}

void PipelineManager::updateLayout()
{
    m_layoutTimestamp = static_cast<int>(QDateTime::currentMSecsSinceEpoch() & 0x7FFFFFFF);
    emit layoutChanged();
}

void PipelineManager::cleanupWorker()
{
    if (m_currentWorker) {
        m_currentWorker->deleteLater();
        m_currentWorker = nullptr;
    }
}

QImage PipelineManager::normalizeImage(const QString &path)
{
    QElapsedTimer timer;
    timer.start();

    QImageReader reader(path);
    reader.setAutoTransform(true);  // Apply EXIF orientation
    QImage image = reader.read();

    if (image.isNull()) {
        qWarning() << "PipelineManager: Failed to load image:" << path
                    << reader.errorString();
        return image;
    }

    // Normalize to ARGB32 to prevent blue-tint issues (ISSUE-1.1)
    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    qDebug() << "PipelineManager: Image normalized:" << image.width() << "x" << image.height()
             << "in" << timer.elapsed() << "ms";
    return image;
}

// --- QML-invokable methods ---

void PipelineManager::loadImage(const QString &path)
{
    QString cleanPath = path;
    if (cleanPath.startsWith("file://")) {
        cleanPath = cleanPath.mid(7);
    }

    QImage normalized = normalizeImage(cleanPath);
    if (normalized.isNull()) {
        emit processingError("Failed to load image");
        return;
    }

    {
        QMutexLocker locker(&m_dataMutex);
        m_original = normalized;
        m_workingCopy = normalized.copy();
        m_commandStack.clear();
        m_preStyleSnapshot = QImage();
        m_cachedStyledResult = QImage();
        m_currentImagePath = cleanPath;
    }

    emit imageLoaded();
    emit commandStackChanged();
    updateLayout();

    qDebug() << "PipelineManager: Image loaded:" << cleanPath;
}

void PipelineManager::applyEnhance(bool enableContrast, bool enableWhiteBalance,
                                    bool enableClahe, double clipLimit)
{
    if (m_isProcessing) return;

    QImage input;
    {
        QMutexLocker locker(&m_dataMutex);
        if (m_workingCopy.isNull()) return;
        input = m_workingCopy.copy();
    }

    setIsProcessing(true);
    cleanupWorker();

    auto *worker = new EnhanceWorker(input, enableContrast, enableWhiteBalance,
                                      enableClahe, clipLimit);
    m_currentWorker = worker;

    connect(worker, &BackgroundWorker::finished, this, &PipelineManager::onWorkerFinished,
            Qt::QueuedConnection);
    connect(worker, &BackgroundWorker::error, this, &PipelineManager::onWorkerError,
            Qt::QueuedConnection);
    connect(worker, &BackgroundWorker::progress, this, &PipelineManager::onWorkerProgress,
            Qt::QueuedConnection);

    // Store command metadata for serialization
    auto cmd = std::make_shared<class EnhanceCommandMeta>(enableContrast, enableWhiteBalance,
                                                           enableClahe, clipLimit);
    {
        QMutexLocker locker(&m_dataMutex);
        CommandEntry entry;
        entry.command = cmd;
        entry.snapshotBefore = m_workingCopy.copy();
        m_commandStack.append(entry);
    }

    m_threadPool.start(worker);
}

void PipelineManager::applyStyle(const QString &modelPath)
{
    if (m_isProcessing) return;

    QImage input;
    {
        QMutexLocker locker(&m_dataMutex);
        if (m_workingCopy.isNull()) return;
        input = m_workingCopy.copy();
        // Save pre-style snapshot for opacity blending
        m_preStyleSnapshot = m_workingCopy.copy();
        m_cachedStyledResult = QImage();
    }

    setIsProcessing(true);
    cleanupWorker();

    auto *worker = new StyleTransferWorker(input, modelPath);
    m_currentWorker = worker;

    connect(worker, &BackgroundWorker::finished, this, [this, modelPath](const QImage &result) {
        {
            QMutexLocker locker(&m_dataMutex);
            m_cachedStyledResult = result;
            m_workingCopy = result;
        }
        setIsProcessing(false);
        updateLayout();
        emit commandStackChanged();
    }, Qt::QueuedConnection);

    connect(worker, &BackgroundWorker::error, this, &PipelineManager::onWorkerError,
            Qt::QueuedConnection);
    connect(worker, &BackgroundWorker::progress, this, &PipelineManager::onWorkerProgress,
            Qt::QueuedConnection);

    // Store command metadata
    auto cmd = std::make_shared<class StyleCommandMeta>(modelPath);
    {
        QMutexLocker locker(&m_dataMutex);
        CommandEntry entry;
        entry.command = cmd;
        entry.snapshotBefore = m_preStyleSnapshot.copy();
        m_commandStack.append(entry);
    }

    m_threadPool.start(worker);
}

void PipelineManager::blendStyle(double opacity)
{
    QMutexLocker locker(&m_dataMutex);

    if (m_preStyleSnapshot.isNull() || m_cachedStyledResult.isNull()) {
        return;
    }

    // Result = Original * (1 - opacity) + Styled * opacity
    const QImage &base = m_preStyleSnapshot;
    const QImage &styled = m_cachedStyledResult;

    QImage baseARGB = base;
    if (baseARGB.format() != QImage::Format_ARGB32) {
        baseARGB = baseARGB.convertToFormat(QImage::Format_ARGB32);
    }

    QImage styledARGB = styled;
    if (styledARGB.format() != QImage::Format_ARGB32) {
        styledARGB = styledARGB.convertToFormat(QImage::Format_ARGB32);
    }

    if (baseARGB.size() != styledARGB.size()) {
        styledARGB = styledARGB.scaled(baseARGB.size(),
            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    const int w = baseARGB.width();
    const int h = baseARGB.height();
    QImage result(w, h, QImage::Format_ARGB32);

    float alpha = static_cast<float>(std::max(0.0, std::min(1.0, opacity)));
    float invAlpha = 1.0f - alpha;

    for (int y = 0; y < h; ++y) {
        const QRgb *baseLine = reinterpret_cast<const QRgb*>(baseARGB.constScanLine(y));
        const QRgb *styledLine = reinterpret_cast<const QRgb*>(styledARGB.constScanLine(y));
        QRgb *outLine = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 0; x < w; ++x) {
            int r = static_cast<int>(qRed(baseLine[x]) * invAlpha + qRed(styledLine[x]) * alpha);
            int g = static_cast<int>(qGreen(baseLine[x]) * invAlpha + qGreen(styledLine[x]) * alpha);
            int b = static_cast<int>(qBlue(baseLine[x]) * invAlpha + qBlue(styledLine[x]) * alpha);

            r = std::max(0, std::min(255, r));
            g = std::max(0, std::min(255, g));
            b = std::max(0, std::min(255, b));

            outLine[x] = qRgba(r, g, b, 255);
        }
    }

    m_workingCopy = result;
    locker.unlock();
    updateLayout();
}

void PipelineManager::undoLast()
{
    QMutexLocker locker(&m_dataMutex);

    if (m_commandStack.isEmpty()) return;

    CommandEntry entry = m_commandStack.takeLast();
    m_workingCopy = entry.snapshotBefore;

    // Clear style cache if undoing a style command
    if (entry.command && entry.command->name() == "style") {
        m_cachedStyledResult = QImage();
        m_preStyleSnapshot = QImage();
    }

    locker.unlock();

    emit commandStackChanged();
    updateLayout();
    qDebug() << "PipelineManager: Undo performed, remaining commands:" << m_commandStack.size();
}

void PipelineManager::resetToOriginal()
{
    QMutexLocker locker(&m_dataMutex);

    if (m_original.isNull()) return;

    m_workingCopy = m_original.copy();
    m_commandStack.clear();
    m_preStyleSnapshot = QImage();
    m_cachedStyledResult = QImage();

    locker.unlock();

    emit commandStackChanged();
    updateLayout();
    qDebug() << "PipelineManager: Reset to original";
}

void PipelineManager::saveResult()
{
    if (m_isProcessing) return;

    QImage toSave;
    {
        QMutexLocker locker(&m_dataMutex);
        if (m_workingCopy.isNull()) return;
        toSave = m_workingCopy.copy();
    }

    // Asynchronous save (ISSUE-1.3)
    QtConcurrent::run([this, toSave]() {
        QString picturesDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        QDir().mkpath(picturesDir);

        QString fileName = "AuroraEditor_" +
            QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".jpg";
        QString destPath = picturesDir + "/" + fileName;

        // JPEG 95% quality as per spec (ISSUE-1.3)
        if (toSave.save(destPath, "JPEG", 95)) {
            emit const_cast<PipelineManager*>(this)->saveDone(destPath);
        } else {
            emit const_cast<PipelineManager*>(this)->saveError("Failed to save image");
        }
    });
}

QString PipelineManager::serializeCommandStack() const
{
    QMutexLocker locker(&m_dataMutex);

    QJsonArray commands;
    for (const auto &entry : m_commandStack) {
        if (entry.command) {
            commands.append(entry.command->toJson());
        }
    }

    QJsonDocument doc(commands);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

void PipelineManager::cancelProcessing()
{
    if (m_currentWorker) {
        m_currentWorker->cancel();
        qDebug() << "PipelineManager: Processing cancelled";
    }
}

void PipelineManager::startBatchProcessing(const QStringList &paths, const QString &serializedCommands)
{
    if (m_isProcessing) return;
    
    setIsProcessing(true);
    cleanupWorker();
    
    auto *worker = new BatchWorker(paths, serializedCommands);
    m_currentWorker = worker;
    
    connect(worker, &BatchWorker::batchProgress, this, &PipelineManager::batchProgress, Qt::QueuedConnection);
    connect(worker, &BatchWorker::batchFinished, this, [this](int success, int fail) {
        setIsProcessing(false);
        emit batchFinished(success, fail);
    }, Qt::QueuedConnection);
    connect(worker, &BackgroundWorker::error, this, &PipelineManager::onWorkerError, Qt::QueuedConnection);
    
    m_threadPool.start(worker);
}

void PipelineManager::saveProject(const QString &projectName)
{
    QString jsonStr = serializeCommandStack();
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dir.exists("projects")) {
        dir.mkpath("projects");
    }
    
    QFile file(dir.filePath("projects/" + projectName + ".json"));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonStr.toUtf8());
        file.close();
    }
}

QVariantList PipelineManager::getSavedProjects() const
{
    QVariantList result;
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/projects");
    if (!dir.exists()) return result;
    
    QFileInfoList files = dir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Time);
    for (const QFileInfo &fi : files) {
        QFile file(fi.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonArray arr = doc.array();
            
            QStringList summary;
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject obj = arr[i].toObject();
                if (obj["type"].toString() == "enhance") summary << "Улучшение";
                else if (obj["type"].toString() == "style") summary << "Стиль";
            }
            
            QVariantMap map;
            map["name"] = fi.baseName();
            map["date"] = fi.lastModified().toString("dd.MM.yyyy hh:mm");
            map["summary"] = summary.isEmpty() ? "Без изменений" : summary.join(", ");
            map["commands"] = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
            
            result.append(map);
        }
    }
    return result;
}

QString PipelineManager::loadProject(const QString &projectName) const
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/projects");
    QFile file(dir.filePath(projectName + ".json"));
    if (file.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(file.readAll());
    }
    return "";
}

// --- Slots ---

void PipelineManager::onWorkerFinished(const QImage &result)
{
    {
        QMutexLocker locker(&m_dataMutex);
        m_workingCopy = result;
    }

    setIsProcessing(false);
    updateLayout();
    emit commandStackChanged();
}

void PipelineManager::onWorkerError(const QString &message)
{
    // Remove the failed command from stack
    {
        QMutexLocker locker(&m_dataMutex);
        if (!m_commandStack.isEmpty()) {
            m_commandStack.removeLast();
        }
    }

    setIsProcessing(false);
    emit commandStackChanged();
    emit processingError(message);
    qWarning() << "PipelineManager: Worker error:" << message;
}

void PipelineManager::onWorkerProgress(int percent)
{
    Q_UNUSED(percent)
    // Could expose to QML if needed
}

// MOC needs to see worker Q_OBJECT definitions from this TU
#include "PipelineManager.moc"
