#pragma once

#include "ImageEditorCommand.h"
#include "MLInferenceEngine.h"

class BackgroundCommand : public ImageEditorCommand {
public:
    BackgroundCommand();
    ~BackgroundCommand() override;

    QString name() const override;
    QImage execute(const QImage& input, MLProfiler* profiler = nullptr) const override;

private:
    mutable MLInferenceEngine engine;
    mutable bool isModelLoaded = false;
};
