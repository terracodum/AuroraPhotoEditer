#pragma once

#include "ImageEditorCommand.h"
#include "MLInferenceEngine.h"
#include <QColor>

class BackgroundCommand : public ImageEditorCommand {
public:
    enum BackgroundMode {
        ModeColor,
        ModeGradient,
        ModeBlur
    };
    BackgroundCommand();
    ~BackgroundCommand() override;

    QString name() const override;
    QImage execute(const QImage& input, MLProfiler* profiler = nullptr) const override;

    void setMode(BackgroundMode mode) { m_mode = mode; }
    void setColor1(const QColor& color) { m_color1 = color; }
    void setColor2(const QColor& color) { m_color2 = color; }
    void setBlurRadius(int radius) { m_blurRadius = radius; }

private:
    BackgroundMode m_mode = ModeBlur;
    QColor m_color1 = Qt::transparent;
    QColor m_color2 = Qt::transparent;
    int m_blurRadius = 50;

    mutable QImage m_cachedInput;
    mutable QImage m_cachedMask;
    mutable MLInferenceEngine engine;
    mutable bool isModelLoaded = false;
};
