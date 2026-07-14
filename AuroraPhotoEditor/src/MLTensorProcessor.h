#pragma once

#include <QImage>
#include <vector>

namespace MLTensorProcessor {

    /**
     * @brief Безопасный препроцессинг изображения в формат NCHW.
     * @param original Исходное изображение.
     * @param targetWidth Требуемая ширина тензора.
     * @param targetHeight Требуемая высота тензора.
     * @param isRMBG Флаг для определения параметров нормализации.
     * @return Вектор значений типа float (плоский NCHW тензор).
     */
    std::vector<float> processInput(const QImage& original, int targetWidth, int targetHeight, bool isRMBG = false);

    /**
     * @brief Извлечение вероятностной карты (QImage маски) из выходного тензора.
     * @param outputTensorValues Выходной плоский тензор.
     * @param outW Ширина маски.
     * @param outH Высота маски.
     * @return 8-битное полутоновое изображение маски.
     */
    QImage processOutput(const std::vector<float>& outputTensorValues, int outW, int outH);

} // namespace MLTensorProcessor
