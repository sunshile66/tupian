#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>
#include <QPixmap>
#include <QImage>

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
        BrightnessFilter
    };

    QPixmap applyFilter(const QPixmap &pixmap, FilterType filter, int param = 0);
    QPixmap scalePixmap(const QPixmap &pixmap, const QSize &size, double scale, int rotation);
    QPixmap optimizePixmapSize(const QPixmap &pixmap, int maxWidth = 1920, int maxHeight = 1080);

    // 滤镜函数
    static QPixmap applyGrayscale(const QPixmap &pixmap);
    static QPixmap applySepia(const QPixmap &pixmap);
    static QPixmap applyBlur(const QPixmap &pixmap, int radius);
    static QPixmap applyBrightness(const QPixmap &pixmap, int factor);

signals:
    void imageProcessed(const QPixmap &result);

private:
    Q_DISABLE_COPY(ImageProcessor)
};

#endif // IMAGEPROCESSOR_H
