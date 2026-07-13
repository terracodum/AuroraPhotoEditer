#pragma once

#include <QImage>
#include <QSize>

/**
 * @brief Utility class for image scaling operations required by ML inference pipeline.
 *
 * Provides static methods for:
 * - Downscaling high-resolution photos to fixed tensor dimensions (prepareModelInput)
 * - Upscaling inference results back to original resolution (upscaleResult)
 * - Extracting and merging high-frequency detail layers for style transfer compositing
 *
 * All methods are static and thread-safe (no shared state).
 */
class ImageScaler {
public:
    /**
     * @brief Downscale an image to fit within targetSize, maintaining aspect ratio.
     * Uses fast transformation for speed on ARM processors.
     * @param source Source image (any resolution).
     * @param targetSize Maximum dimension (width and height).
     * @return Scaled image with even dimensions (some models require this).
     */
    static QImage prepareModelInput(const QImage &source, int targetSize);

    /**
     * @brief Downscale to exact square dimensions (for models with fixed input).
     * @param source Source image.
     * @param width Target width.
     * @param height Target height.
     * @return Scaled image.
     */
    static QImage prepareModelInputExact(const QImage &source, int width, int height);

    /**
     * @brief Upscale a result image to original dimensions using smooth transformation.
     * @param result Inference result (small resolution).
     * @param originalSize Target size to upscale to.
     * @return Upscaled image.
     */
    static QImage upscaleResult(const QImage &result, const QSize &originalSize);

    /**
     * @brief Extract high-frequency detail layer from an image.
     *
     * Computes: highFreq = original - gaussianBlur(original)
     * Used in style transfer compositing to preserve fine details at full resolution.
     *
     * @param image Source image.
     * @param blurRadius Radius for the Gaussian-like blur (via downscale/upscale approximation).
     * @return High-frequency detail layer (signed values stored as offset-128 grayscale per channel).
     */
    static QImage extractHighFrequency(const QImage &image, int blurRadius = 16);

    /**
     * @brief Composite a stylized base layer with high-frequency details from the original.
     *
     * Implements the Luminosity blend: adds high-frequency luminance details
     * from the original over the stylized color layer.
     *
     * @param stylized Upscaled stylized image (base color layer).
     * @param original Original full-resolution image (source of HF details).
     * @param blurRadius Controls the frequency split point.
     * @return Composited result preserving both style and fine detail.
     */
    static QImage compositeWithDetails(const QImage &stylized, const QImage &original,
                                        int blurRadius = 16);

private:
    ImageScaler() = default;  // Static-only class

    /**
     * @brief Fast approximate Gaussian blur using downscale + upscale.
     * Much faster than kernel convolution for large images on ARM.
     */
    static QImage fastBlur(const QImage &image, int radius);
};
