#include "PipelineManager.h"
#include "BackgroundWorker.h"
#include "BackgroundCommand.h"
#include "EnhanceCommand.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QMetaObject>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>

ExportWorker::ExportWorker(QObject *parent) : QObject(parent) {}

void ExportWorker::exportImage(const QImage &image) {
  QString picturesLocation =
      QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
  QDir dir(picturesLocation);
  if (!dir.exists()) {
    dir.mkpath(".");
  }

  QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
  QString filePath = dir.absoluteFilePath(
      QString("AuroraPhotoEditor_%1.jpg").arg(timestamp));

  // Avoid silently overwriting an existing file (two saves within one second).
  for (int i = 1; QFileInfo::exists(filePath); ++i) {
    filePath = dir.absoluteFilePath(
        QString("AuroraPhotoEditor_%1_%2.jpg").arg(timestamp).arg(i));
  }

  bool success = image.save(filePath, "JPEG", 95);
  emit exportCompleted(success, filePath);
}

PipelineManager::PipelineManager(QObject *parent) : QObject(parent) {

  m_exportWorker = new ExportWorker();
  m_exportWorker->moveToThread(&m_exportThread);

  connect(&m_exportThread, &QThread::finished, m_exportWorker,
          &QObject::deleteLater);

  connect(this, &PipelineManager::exportRequested, m_exportWorker,
          &ExportWorker::exportImage);
  connect(m_exportWorker, &ExportWorker::exportCompleted, this,
          &PipelineManager::exportCompleted);

  m_exportThread.start();
}

PipelineManager::~PipelineManager() {
  if (m_activeWorker) {
    QThread *workerThread = m_activeWorker->thread();
    m_activeWorker->cancel();
    if (workerThread) {
      workerThread->requestInterruption();
      workerThread->quit();
      if (!workerThread->wait(3000)) {
          workerThread->terminate();
          workerThread->wait();
      }
    }
  }

  m_exportThread.requestInterruption();
  m_exportThread.quit();
  if (!m_exportThread.wait(3000)) {
      m_exportThread.terminate();
      m_exportThread.wait();
  }
}

bool PipelineManager::hasImage() const {
  QMutexLocker locker(&m_mutex);
  return !m_current.isNull();
}

bool PipelineManager::loadFromUri(const QString &uriString) {
  QUrl url(uriString);
  QString localFile = url.isLocalFile() ? url.toLocalFile() : uriString;

  QImageReader reader(localFile);
  reader.setAutoTransform(true); // Account for EXIF orientation

  // Guard against decompression bombs: reject absurd dimensions before
  // allocating. reader.size() reads only the header.
  const QSize dims = reader.size();
  if (dims.isValid()) {
    const qint64 maxPixels = 64LL * 1024 * 1024; // 64 megapixels
    if (static_cast<qint64>(dims.width()) * dims.height() > maxPixels) {
      qWarning() << "Refusing to load oversized image:" << dims;
      return false;
    }
  }

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

void PipelineManager::setOriginalImage(const QImage &image) {
  bool currentChanged = false;
  bool originalChanged = false;
  bool stackChanged = false;

  {
    QMutexLocker locker(&m_mutex);
    if (m_original != image) {
      m_original = image;
      originalChanged = true;
    }
    if (m_current != image) {
      m_current = image;
      currentChanged = true;
    }
    if (!m_commandStack.isEmpty()) {
      m_commandStack.clear();
      stackChanged = true;
    }
  }

  // Signals carry the NEW image, matching applyCommand/undoLast/resetToOriginal.
  if (originalChanged) {
    emit originalImageChanged(image);
  }
  if (currentChanged) {
    emit currentImageChanged(image);
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

bool PipelineManager::isProcessing() const { return m_isProcessing; }

void PipelineManager::setIsProcessing(bool processing) {
  if (m_isProcessing != processing) {
    m_isProcessing = processing;
    emit isProcessingChanged();
  }
}

bool PipelineManager::applyCommand(QSharedPointer<ImageEditorCommand> command) {
  if (!command) {
    return false;
  }

  QImage currentImage;
  {
    QMutexLocker locker(&m_mutex);
    if (m_current.isNull()) {
      return false;
    }
    currentImage = m_current;
  }

  if (m_activeWorker) {
    // Drop our state handlers first, then cancel. The worker->thread quit
    // connections below are independent of `this`, so the superseded thread
    // still finishes and cleans itself up (no leaked QThread/worker).
    disconnect(m_activeWorker, nullptr, this, nullptr);
    m_activeWorker->cancel();
    m_activeWorker = nullptr;
  }

  QThread *thread = new QThread();
  BackgroundWorker *worker = new BackgroundWorker(command, currentImage);
  worker->moveToThread(thread);

  connect(thread, &QThread::started, worker, &BackgroundWorker::process);

  // Thread lifecycle: quit on completion regardless of the state handlers,
  // so a superseded worker still tears its thread down.
  connect(worker, &BackgroundWorker::success, thread, &QThread::quit);
  connect(worker, &BackgroundWorker::canceled, thread, &QThread::quit);

  connect(worker, &BackgroundWorker::success, this,
          [this, worker, command](const QImage &resultImage) {
            if (m_activeWorker != worker)
              return;

            {
              QMutexLocker locker(&m_mutex);
              m_current = resultImage;
              m_commandStack.append({command, resultImage});
            }

            emit currentImageChanged(resultImage);
            emit commandStackChanged();

            setIsProcessing(false);
            m_activeWorker = nullptr;
          });

  connect(worker, &BackgroundWorker::canceled, this, [this, worker]() {
    if (m_activeWorker == worker) {
      setIsProcessing(false);
      m_activeWorker = nullptr;
    }
  });

  // Memory management
  connect(thread, &QThread::finished, worker, &QObject::deleteLater);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);

  m_activeWorker = worker;
  setIsProcessing(true);
  thread->start();

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

void PipelineManager::applyBackgroundRemoval() {
    applyCommand(QSharedPointer<BackgroundCommand>::create());
}

void PipelineManager::applyEnhance() {
    applyCommand(QSharedPointer<EnhanceCommand>::create());
}

int PipelineManager::commandCount() const {
  QMutexLocker locker(&m_mutex);
  return m_commandStack.size();
}

bool PipelineManager::canUndo() const {
  QMutexLocker locker(&m_mutex);
  return !m_commandStack.isEmpty();
}

void PipelineManager::exportImage() {
  QImage imageToSave = getCurrentImage();
  if (imageToSave.isNull()) {
    emit exportCompleted(false, "");
    return;
  }

  emit exportRequested(imageToSave);
}
