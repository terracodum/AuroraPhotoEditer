#pragma once

#include <QImage>
#include <vector>

namespace MLTensorProcessor {

    enum class NormalizationMode {
        Standard,
        RMBG,
        None
    };

    /**
     * @brief Безопасный препроцессинг изображения в формат NCHW.
     * @param original Исходное изображение.
     * @param targetWidth Требуемая ширина тензора.
     * @param targetHeight Требуемая высота тензора.
     * @param mode Режим нормализации.
     * @return Вектор значений типа float (плоский NCHW тензор).
     */
    std::vector<float> processInput(const QImage& original, int targetWidth, int targetHeight, NormalizationMode mode = NormalizationMode::Standard);

    /**
     * @brief Извлечение вероятностной карты (QImage маски) из выходного тензора.
     * @param outputTensorValues Выходной плоский тензор.
     * @param outW Ширина маски.
     * @param outH Высота маски.
     * @return 8-битное полутоновое изображение маски.
     */
    QImage processOutput(const std::vector<float>& outputTensorValues, int outW, int outH);

    /**
     * @brief Извлечение RGB изображения из выходного тензора.
     * @param outputTensorValues Выходной плоский тензор.
     * @param outW Ширина изображения.
     * @param outH Высота изображения.
     * @return 24-битное RGB изображение.
     */
    QImage processOutputRGB(const std::vector<float>& outputTensorValues, int outW, int outH);

} // namespace MLTensorProcessor
