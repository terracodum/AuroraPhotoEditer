#pragma once

#include <QObject>
#include <QString>
#include <QColor>

#include <QVariant>

class ImageProcessor : public QObject {
    Q_OBJECT
public:
    explicit ImageProcessor(QObject *parent = nullptr);

    // Legacy mode
    Q_INVOKABLE QString processImage(const QString &originalPath, const QString &maskPath, const QString &mode, const QColor &bgColor = Qt::white);
    
    // Advanced composable effects
    Q_INVOKABLE QString processAdvancedBackground(const QString &originalPath, const QString &maskPath, const QVariantMap &settings);
    
    Q_INVOKABLE bool saveToGallery(const QString &resultPath);
};
