#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QFuture>

class ImageProcessor : public QObject
{
    Q_OBJECT

public:
    explicit ImageProcessor(QObject *parent = nullptr);

    enum FilterType {
        NoFilter,
        GrayscaleFilter,
        SepiaFilter,
        BlurFilter,
        BrightnessFilter,
        ContrastFilter,
        SharpnessFilter,
        EnhanceFilter,
        SuperResolutionFilter,
        RemoveWatermarkFilter
    };

    enum EnhancementType {
        EnhanceResolution,
        EnhanceColors,
        ReduceNoise,
        RemoveWatermark
    };

    QPixmap applyFilter(const QPixmap &pixmap, FilterType filter, int param = 0);
    QPixmap scalePixmap(const QPixmap &pixmap, const QSize &size, double scale, int rotation);
    QPixmap optimizePixmapSize(const QPixmap &pixmap, int maxWidth = 1920, int maxHeight = 1080);

    // 异步增强方法
    QFuture<QPixmap> enhanceImageAsync(const QPixmap &pixmap, EnhancementType type, int strength = 50);
    QFuture<QPixmap> removeWatermarkAsync(const QPixmap &pixmap, const QRect &watermarkArea);

    // 滤镜函数
    static QPixmap applyGrayscale(const QPixmap &pixmap);
    static QPixmap applySepia(const QPixmap &pixmap);
    static QPixmap applyBlur(const QPixmap &pixmap, int radius);
    static QPixmap applyBrightness(const QPixmap &pixmap, int factor);
    static QPixmap applyContrast(const QPixmap &pixmap, int factor);
    static QPixmap applySharpness(const QPixmap &pixmap, int factor);
    static QPixmap applyEnhance(const QPixmap &pixmap);
    static QPixmap applySuperResolution(const QPixmap &pixmap);
    static QPixmap removeWatermark(const QPixmap &pixmap, const QRect &watermarkArea);

signals:
    void imageProcessed(const QPixmap &result);
    void enhancementProgress(int percent);
    void enhancementFinished(const QPixmap &result);

private:
    static QImage enhanceResolution(const QImage &image, int scaleFactor);
    static QImage enhanceColors(const QImage &image, int strength);
    static QImage reduceNoise(const QImage &image, int strength);

    Q_DISABLE_COPY(ImageProcessor)
};

#endif // IMAGEPROCESSOR_H
