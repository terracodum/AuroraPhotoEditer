#include "BatchWorker.h"
#include "EnhanceEngine.h"
#include "ImageScaler.h"
#include "MLInferenceEngine.h"
#include "StyleTransferEngine.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QImageReader>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QDebug>
#include <stdexcept>

BatchWorker::BatchWorker(const QStringList &inputPaths, const QString &serializedCommands)
    : m_inputPaths(inputPaths)
{
    QJsonDocument doc = QJsonDocument::fromJson(serializedCommands.toUtf8());
    if (doc.isArray()) {
        m_commands = doc.array();
    }
}

void BatchWorker::doWork()
{
    int total = m_inputPaths.size();
    if (total == 0) {
        emit batchFinished(0, 0);
        return;
    }

    int successCount = 0;
    int failCount = 0;

    QString picturesDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/AuroraBatch";
    QDir().mkpath(picturesDir);

    for (int i = 0; i < total; ++i) {
        checkCancelled();
        
        QString path = m_inputPaths[i];
        if (path.startsWith("file://")) {
            path = path.mid(7);
        }

        emit batchProgress(i, total);

        try {
            // Load and normalize
            QImageReader reader(path);
            reader.setAutoTransform(true);
            QImage image = reader.read();
            if (image.isNull()) throw std::runtime_error("Failed to load image");

            if (image.format() != QImage::Format_ARGB32) {
                image = image.convertToFormat(QImage::Format_ARGB32);
            }

            // Apply commands sequentially
            for (int j = 0; j < m_commands.size(); ++j) {
                checkCancelled();
                QJsonObject cmd = m_commands[j].toObject();
                QString type = cmd["cmd"].toString();

                if (type == "enhance") {
                    bool contrast = cmd["contrast"].toBool();
                    bool wb = cmd["whiteBalance"].toBool();
                    bool clahe = cmd["clahe"].toBool();
                    double clipLimit = cmd["clipLimit"].toDouble(2.0);

                    if (contrast) EnhanceEngine::applyAutoContrast(image, 1.0f);
                    if (wb) EnhanceEngine::applyGrayWorldWB(image);
                    if (clahe) EnhanceEngine::applyCLAHE(image, clipLimit, 8);

                } else if (type == "style") {
                    QString modelPath = cmd["modelPath"].toString();
                    
                    try {
                        image = StyleTransferEngine::applyStyleTransfer(image, modelPath, &m_cancelled);
                    } catch (const std::exception &e) {
                        if (QString::fromStdString(e.what()) == "cancelled") throw;
                        throw std::runtime_error("Style transfer failed");
                    }
                }
            }

            // Save result
            QString outFileName = "Batch_" + QFileInfo(path).baseName() + "_" + QDateTime::currentDateTime().toString("HHmmss") + ".jpg";
            QString outPath = picturesDir + "/" + outFileName;
            
            if (image.save(outPath, "JPEG", 95)) {
                successCount++;
            } else {
                failCount++;
            }

        } catch (const std::exception &e) {
            QString msg = QString::fromStdString(e.what());
            if (msg == "cancelled") throw; // propagate cancellation
            
            qWarning() << "BatchWorker failed for" << path << ":" << msg;
            failCount++;
        }
    }

    emit batchProgress(total, total);
    emit batchFinished(successCount, failCount);
}
