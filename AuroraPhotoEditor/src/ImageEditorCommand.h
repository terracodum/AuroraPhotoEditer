#pragma once

#include <QImage>
#include <QString>

class MLProfiler;

class ImageEditorCommand {
public:
    virtual ~ImageEditorCommand() = default;

    // Executes the command on the input image and returns the modified image.
    virtual QImage execute(const QImage& input, MLProfiler* profiler = nullptr) const = 0;

    // Returns the name/description of the command.
    virtual QString name() const = 0;
};
