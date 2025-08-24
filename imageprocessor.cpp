#include "imageprocessor.h"
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QColor>

ImageProcessor::ImageProcessor(QObject *parent)
    : QObject(parent)
{
}

QPixmap ImageProcessor::applyFilter(const QPixmap &pixmap, FilterType filter, int param)
{
    if (pixmap.isNull()) return QPixmap();

    switch (filter) {
    case GrayscaleFilter:
        return applyGrayscale(pixmap);
    case SepiaFilter:
        return applySepia(pixmap);
    case BlurFilter:
        return applyBlur(pixmap, param);
    case BrightnessFilter:
        return applyBrightness(pixmap, param);
    default:
        return pixmap;
    }
}

QPixmap ImageProcessor::scalePixmap(const QPixmap &pixmap, const QSize &size, double scale, int rotation)
{
    if (pixmap.isNull()) return QPixmap();

    // 应用旋转
    QTransform transform;
    transform.rotate(rotation);
    QPixmap rotatedPixmap = pixmap.transformed(transform, Qt::SmoothTransformation);

    // 计算适合标签大小的尺寸，保持宽高比
    QSize scaledSize = rotatedPixmap.size();
    scaledSize.scale(size * scale, Qt::KeepAspectRatio);

    // 应用缩放
    return rotatedPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap ImageProcessor::optimizePixmapSize(const QPixmap &pixmap, int maxWidth, int maxHeight)
{
    if (pixmap.isNull() ||
        (pixmap.width() <= maxWidth && pixmap.height() <= maxHeight)) {
        return pixmap;
    }

    return pixmap.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap ImageProcessor::applyGrayscale(const QPixmap &pixmap)
{
    QImage image = pixmap.toImage();
    if (image.isNull()) return QPixmap();

    // 使用更高效的转换方法
    if (image.format() != QImage::Format_ARGB32 &&
        image.format() != QImage::Format_RGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    for (int y = 0; y < image.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QRgb color = line[x];
            int gray = qGray(color);
            line[x] = qRgba(gray, gray, gray, qAlpha(color));
        }
    }
    return QPixmap::fromImage(image);
}

QPixmap ImageProcessor::applySepia(const QPixmap &pixmap)
{
    QImage image = pixmap.toImage();
    if (image.isNull()) return QPixmap();

    // 使用更高效的转换方法
    if (image.format() != QImage::Format_ARGB32 &&
        image.format() != QImage::Format_RGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    for (int y = 0; y < image.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QRgb color = line[x];
            int r = qRed(color);
            int g = qGreen(color);
            int b = qBlue(color);
            int a = qAlpha(color);

            int tr = qMin(255, static_cast<int>(0.393 * r + 0.769 * g + 0.189 * b));
            int tg = qMin(255, static_cast<int>(0.349 * r + 0.686 * g + 0.168 * b));
            int tb = qMin(255, static_cast<int>(0.272 * r + 0.534 * g + 0.131 * b));

            line[x] = qRgba(tr, tg, tb, a);
        }
    }
    return QPixmap::fromImage(image);
}

QPixmap ImageProcessor::applyBlur(const QPixmap &pixmap, int radius)
{
    if (pixmap.isNull() || radius <= 0) return pixmap;

    // 使用QImage进行模糊处理，避免QGraphicsEffect的线程问题
    QImage image = pixmap.toImage();
    if (image.isNull()) return pixmap;

    // 简单的模糊算法（高斯模糊简化版）
    QImage blurred(image.size(), image.format());

    int kernelSize = radius * 2 + 1;
    float kernel[kernelSize];
    float sigma = radius / 2.0;
    float sum = 0.0;

    // 创建高斯核
    for (int i = 0; i < kernelSize; i++) {
        int x = i - radius;
        kernel[i] = exp(-(x * x) / (2 * sigma * sigma)) / (sqrt(2 * M_PI) * sigma);
        sum += kernel[i];
    }

    // 归一化
    for (int i = 0; i < kernelSize; i++) {
        kernel[i] /= sum;
    }

    // 水平模糊
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            float r = 0, g = 0, b = 0, a = 0;

            for (int i = 0; i < kernelSize; i++) {
                int px = x + i - radius;
                if (px < 0) px = 0;
                if (px >= image.width()) px = image.width() - 1;

                QRgb pixel = image.pixel(px, y);
                r += qRed(pixel) * kernel[i];
                g += qGreen(pixel) * kernel[i];
                b += qBlue(pixel) * kernel[i];
                a += qAlpha(pixel) * kernel[i];
            }

            blurred.setPixel(x, y, qRgba(r, g, b, a));
        }
    }

    // 垂直模糊
    QImage result(blurred.size(), blurred.format());
    for (int y = 0; y < blurred.height(); y++) {
        for (int x = 0; x < blurred.width(); x++) {
            float r = 0, g = 0, b = 0, a = 0;

            for (int i = 0; i < kernelSize; i++) {
                int py = y + i - radius;
                if (py < 0) py = 0;
                if (py >= blurred.height()) py = blurred.height() - 1;

                QRgb pixel = blurred.pixel(x, py);
                r += qRed(pixel) * kernel[i];
                g += qGreen(pixel) * kernel[i];
                b += qBlue(pixel) * kernel[i];
                a += qAlpha(pixel) * kernel[i];
            }

            result.setPixel(x, y, qRgba(r, g, b, a));
        }
    }

    return QPixmap::fromImage(result);
}
QPixmap ImageProcessor::applyBrightness(const QPixmap &pixmap, int factor)
{
    QImage image = pixmap.toImage();
    if (image.isNull()) return QPixmap();

    // 使用更高效的转换方法
    if (image.format() != QImage::Format_ARGB32 &&
        image.format() != QImage::Format_RGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    for (int y = 0; y < image.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QRgb color = line[x];
            int r = qMin(255, qMax(0, qRed(color) + factor));
            int g = qMin(255, qMax(0, qGreen(color) + factor));
            int b = qMin(255, qMax(0, qBlue(color) + factor));
            int a = qAlpha(color);
            line[x] = qRgba(r, g, b, a);
        }
    }
    return QPixmap::fromImage(image);
}
