#pragma once

#include "ImageEditorCommand.h"

class EnhanceCommand : public ImageEditorCommand {
public:
    EnhanceCommand();
    ~EnhanceCommand() override;

    QImage execute(const QImage& input, MLProfiler* profiler = nullptr) const override;
    QString name() const override;
};
