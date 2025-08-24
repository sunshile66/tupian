#include "imageprocessor.h"
#include <QPainter>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QColor>
#include <QtConcurrent>
#include <QElapsedTimer>
#include <cmath>

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
    case ContrastFilter:
        return applyContrast(pixmap, param);
    case SharpnessFilter:
        return applySharpness(pixmap, param);
    case EnhanceFilter:
        return applyEnhance(pixmap);
    case SuperResolutionFilter:
        return applySuperResolution(pixmap);
    case RemoveWatermarkFilter:
        return removeWatermark(pixmap, QRect(0, 0, pixmap.width()/4, pixmap.height()/4));
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

QFuture<QPixmap> ImageProcessor::enhanceImageAsync(const QPixmap &pixmap, EnhancementType type, int strength)
{
    return QtConcurrent::run([=]() {
        QElapsedTimer timer;
        timer.start();

        QImage image = pixmap.toImage();
        if (image.isNull()) return QPixmap();

        QImage result;

        switch (type) {
        case EnhanceResolution:
            result = enhanceResolution(image, strength / 10);
            break;
        case EnhanceColors:
            result = enhanceColors(image, strength);
            break;
        case ReduceNoise:
            result = reduceNoise(image, strength);
            break;
        default:
            result = image;
            break;
        }

        qDebug() << "Enhancement took" << timer.elapsed() << "ms";
        return QPixmap::fromImage(result);
    });
}

QFuture<QPixmap> ImageProcessor::removeWatermarkAsync(const QPixmap &pixmap, const QRect &watermarkArea)
{
    return QtConcurrent::run([=]() {
        return removeWatermark(pixmap, watermarkArea);
    });
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

QPixmap ImageProcessor::applyContrast(const QPixmap &pixmap, int factor)
{
    QImage image = pixmap.toImage();
    if (image.isNull()) return QPixmap();

    if (image.format() != QImage::Format_ARGB32 &&
        image.format() != QImage::Format_RGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    double contrast = qPow((100.0 + factor) / 100.0, 2);

    for (int y = 0; y < image.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QRgb color = line[x];
            double r = qRed(color);
            double g = qGreen(color);
            double b = qBlue(color);
            int a = qAlpha(color);

            r = (((r / 255.0) - 0.5) * contrast + 0.5) * 255.0;
            g = (((g / 255.0) - 0.5) * contrast + 0.5) * 255.0;
            b = (((b / 255.0) - 0.5) * contrast + 0.5) * 255.0;

            r = qMin(255.0, qMax(0.0, r));
            g = qMin(255.0, qMax(0.0, g));
            b = qMin(255.0, qMax(0.0, b));

            line[x] = qRgba(r, g, b, a);
        }
    }
    return QPixmap::fromImage(image);
}

QPixmap ImageProcessor::applySharpness(const QPixmap &pixmap, int factor)
{
    if (pixmap.isNull()) return pixmap;

    // 简单锐化算法 - 使用拉普拉斯算子
    QImage image = pixmap.toImage();
    if (image.isNull()) return pixmap;

    QImage sharpened(image.size(), image.format());

    // 锐化内核
    int kernel[3][3] = {
        {-1, -1, -1},
        {-1,  9, -1},
        {-1, -1, -1}
    };

    int kernelSize = 3;
    int kernelRadius = kernelSize / 2;

    for (int y = kernelRadius; y < image.height() - kernelRadius; y++) {
        for (int x = kernelRadius; x < image.width() - kernelRadius; x++) {
            int r = 0, g = 0, b = 0;

            for (int ky = -kernelRadius; ky <= kernelRadius; ky++) {
                for (int kx = -kernelRadius; kx <= kernelRadius; kx++) {
                    QRgb pixel = image.pixel(x + kx, y + ky);
                    int weight = kernel[ky + kernelRadius][kx + kernelRadius];

                    r += qRed(pixel) * weight;
                    g += qGreen(pixel) * weight;
                    b += qBlue(pixel) * weight;
                }
            }

            r = qMin(255, qMax(0, r * factor / 10));
            g = qMin(255, qMax(0, g * factor / 10));
            b = qMin(255, qMax(0, b * factor / 10));

            sharpened.setPixel(x, y, qRgb(r, g, b));
        }
    }

    return QPixmap::fromImage(sharpened);
}

QPixmap ImageProcessor::applyEnhance(const QPixmap &pixmap)
{
    // 综合增强：亮度、对比度和锐化
    QPixmap result = applyBrightness(pixmap, 10);
    result = applyContrast(result, 15);
    result = applySharpness(result, 5);
    return result;
}

QPixmap ImageProcessor::applySuperResolution(const QPixmap &pixmap)
{
    // 简单的超分辨率实现 - 使用高质量缩放
    if (pixmap.isNull()) return pixmap;

    // 放大2倍
    QSize newSize = pixmap.size() * 2;
    return pixmap.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap ImageProcessor::removeWatermark(const QPixmap &pixmap, const QRect &watermarkArea)
{
    if (pixmap.isNull()) return pixmap;

    QImage image = pixmap.toImage();
    if (image.isNull()) return pixmap;

    QImage result = image.copy();

    // 简单的水印去除 - 使用周围像素进行填充
    int padding = 5;
    QRect area = watermarkArea.intersected(image.rect());

    for (int y = area.top(); y < area.bottom(); y++) {
        for (int x = area.left(); x < area.right(); x++) {
            // 使用周围像素的平均值
            int r = 0, g = 0, b = 0, count = 0;

            for (int dy = -padding; dy <= padding; dy++) {
                for (int dx = -padding; dx <= padding; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < image.width() &&
                        ny >= 0 && ny < image.height() &&
                        !area.contains(nx, ny)) {
                        QRgb pixel = image.pixel(nx, ny);
                        r += qRed(pixel);
                        g += qGreen(pixel);
                        b += qBlue(pixel);
                        count++;
                    }
                }
            }

            if (count > 0) {
                r /= count;
                g /= count;
                b /= count;
                result.setPixel(x, y, qRgb(r, g, b));
            }
        }
    }

    return QPixmap::fromImage(result);
}

QImage ImageProcessor::enhanceResolution(const QImage &image, int scaleFactor)
{
    if (image.isNull()) return image;

    // 简单的分辨率增强 - 使用高质量缩放
    QSize newSize = image.size() * scaleFactor;
    return image.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage ImageProcessor::enhanceColors(const QImage &image, int strength)
{
    if (image.isNull()) return image;

    QImage result = image;
    if (result.format() != QImage::Format_ARGB32 &&
        result.format() != QImage::Format_RGB32) {
        result = result.convertToFormat(QImage::Format_ARGB32);
    }

    double factor = strength / 50.0;

    for (int y = 0; y < result.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            QRgb color = line[x];
            int r = qRed(color);
            int g = qGreen(color);
            int b = qBlue(color);
            int a = qAlpha(color);

            // 增强饱和度
            int avg = (r + g + b) / 3;
            r = qMin(255, qMax(0, static_cast<int>(avg + (r - avg) * factor)));
            g = qMin(255, qMax(0, static_cast<int>(avg + (g - avg) * factor)));
            b = qMin(255, qMax(0, static_cast<int>(avg + (b - avg) * factor)));

            line[x] = qRgba(r, g, b, a);
        }
    }

    return result;
}

QImage ImageProcessor::reduceNoise(const QImage &image, int strength)
{
    if (image.isNull()) return image;

    // 简单的降噪算法 - 使用中值滤波
    QImage result = image;
    if (result.format() != QImage::Format_ARGB32 &&
        result.format() != QImage::Format_RGB32) {
        result = result.convertToFormat(QImage::Format_ARGB32);
    }

    int radius = strength / 20;
    if (radius < 1) radius = 1;

    for (int y = radius; y < result.height() - radius; y++) {
        for (int x = radius; x < result.width() - radius; x++) {
            QVector<int> rValues, gValues, bValues;

            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    QRgb pixel = image.pixel(x + dx, y + dy);
                    rValues.append(qRed(pixel));
                    gValues.append(qGreen(pixel));
                    bValues.append(qBlue(pixel));
                }
            }

            std::sort(rValues.begin(), rValues.end());
            std::sort(gValues.begin(), gValues.end());
            std::sort(bValues.begin(), bValues.end());

            int medianIndex = rValues.size() / 2;
            result.setPixel(x, y, qRgb(
                                      rValues[medianIndex],
                                      gValues[medianIndex],
                                      bValues[medianIndex]
                                      ));
        }
    }

    return result;
}
