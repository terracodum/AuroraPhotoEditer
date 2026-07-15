#pragma once

#include "ImageEditorCommand.h"
#include <QString>

class StyleTransferCommand : public ImageEditorCommand {
public:
    explicit StyleTransferCommand(const QString& modelName);
    ~StyleTransferCommand() override;

    QImage execute(const QImage& input, MLProfiler* profiler = nullptr) const override;
    QString name() const override;

private:
    QString m_modelName;
};
