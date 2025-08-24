#include "animationmanager.h"
#include <QGraphicsOpacityEffect>
#include <QGraphicsBlurEffect>
#include <QGraphicsColorizeEffect>
#include <QEasingCurve>
#include <QVariantAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

AnimationManager::AnimationManager(QObject *parent)
    : QObject(parent)
    , m_duration(300)
    , m_soundEnabled(true)
{
}

void AnimationManager::applyAnimation(QLabel *target, const QPixmap &newPixmap, AnimationType type, int direction)
{
    if (!target) return;

    // 彻底停止所有现有动画
    if (target->graphicsEffect()) {
        QGraphicsEffect* effect = target->graphicsEffect();
        effect->setEnabled(false);
        target->setGraphicsEffect(nullptr);
        effect->deleteLater();
    }

    // 停止所有可能正在运行的动画
    QList<QPropertyAnimation*> animations = target->findChildren<QPropertyAnimation*>();
    for (QPropertyAnimation* animation : animations) {
        animation->stop();
        animation->deleteLater();
    }

    // 播放动画音效
    playAnimationSound();

    emit animationStarted();

    switch (type) {
    case NoAnimation:
        target->setPixmap(newPixmap);
        emit animationFinished();
        break;
    case SlideAnimation:
        slideAnimation(target, newPixmap, direction);
        break;
    case FadeAnimation:
        fadeAnimation(target, newPixmap);
        break;
    case ZoomAnimation:
        zoomAnimation(target, newPixmap);
        break;
    case FlipAnimation:
        flipAnimation(target, newPixmap);
        break;
    case RotateAnimation:
        rotateAnimation(target, newPixmap);
        break;
    case CubeAnimation:
        cubeAnimation(target, newPixmap, direction);
        break;
    case BlurAnimation:
        blurAnimation(target, newPixmap);
        break;
    case GlowAnimation:
        glowAnimation(target, newPixmap);
        break;
    case WaterDropAnimation:
        waterDropAnimation(target, newPixmap);
        break;
    case PageCurlAnimation:
        pageCurlAnimation(target, newPixmap);
        break;
    }
}

void AnimationManager::cleanupAnimation(QObject *animation)
{
    if (animation) {
        animation->deleteLater();
    }
}

void AnimationManager::playAnimationSound()
{
    if (!m_soundEnabled) return;
    // 音效功能暂时移除，需要QtMultimedia支持
}

void AnimationManager::slideAnimation(QLabel *target, const QPixmap &pixmap, int direction)
{
    // 创建临时标签用于动画
    QLabel *tempLabel = new QLabel(target->parentWidget());
    tempLabel->setPixmap(target->pixmap().isNull() ? QPixmap() : target->pixmap());
    tempLabel->setGeometry(target->geometry());
    tempLabel->setAlignment(Qt::AlignCenter);
    tempLabel->setAttribute(Qt::WA_DeleteOnClose);
    tempLabel->show();
    tempLabel->raise();

    // 更新主标签
    target->setPixmap(pixmap);

    // 设置动画
    QPropertyAnimation *animation = new QPropertyAnimation(tempLabel, "geometry");
    animation->setDuration(m_duration);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    QRect startRect = target->geometry();
    QRect endRect = startRect;

    if (direction > 0) {
        // 向右滑动
        startRect.moveTo(-startRect.width(), 0);
        tempLabel->setGeometry(startRect);
        endRect.moveTo(startRect.width(), 0);
    } else {
        // 向左滑动
        startRect.moveTo(startRect.width(), 0);
        tempLabel->setGeometry(startRect);
        endRect.moveTo(-startRect.width(), 0);
    }

    animation->setStartValue(startRect);
    animation->setEndValue(endRect);

    // 动画结束后删除临时标签
    connect(animation, &QPropertyAnimation::finished, this, [tempLabel, animation, this]() {
        tempLabel->close();
        cleanupAnimation(animation);
        emit animationFinished();
    });

    animation->start();
}

void AnimationManager::fadeAnimation(QLabel *target, const QPixmap &pixmap)
{
    // 保存原始pixmap
    QPixmap oldPixmap = target->pixmap();

    // 设置淡入效果
    QGraphicsOpacityEffect *fadeInEffect = new QGraphicsOpacityEffect(target);
    target->setGraphicsEffect(fadeInEffect);
    target->setPixmap(pixmap);

    QPropertyAnimation *fadeInAnimation = new QPropertyAnimation(fadeInEffect, "opacity");
    fadeInAnimation->setDuration(m_duration);
    fadeInAnimation->setStartValue(0.0);
    fadeInAnimation->setEndValue(1.0);
    fadeInAnimation->setEasingCurve(QEasingCurve::InCubic);

    // 动画结束后清理效果
    connect(fadeInAnimation, &QPropertyAnimation::finished, this, [target, fadeInEffect, fadeInAnimation, this]() {
        target->setGraphicsEffect(nullptr);
        cleanupAnimation(fadeInEffect);
        cleanupAnimation(fadeInAnimation);
        emit animationFinished();
    });

    fadeInAnimation->start();
}

void AnimationManager::zoomAnimation(QLabel *target, const QPixmap &pixmap)
{
    // 创建缩放动画序列
    QSequentialAnimationGroup *group = new QSequentialAnimationGroup(this);

    // 第一步：缩小当前图片
    QGraphicsOpacityEffect *zoomOutEffect = new QGraphicsOpacityEffect(target);
    target->setGraphicsEffect(zoomOutEffect);

    QPropertyAnimation *zoomOut = new QPropertyAnimation(target, "geometry");
    zoomOut->setDuration(m_duration/2);
    zoomOut->setEasingCurve(QEasingCurve::OutCubic);

    QRect startRect = target->geometry();
    QRect midRect = startRect;
    midRect.setSize(QSize(startRect.width()/2, startRect.height()/2));
    midRect.moveCenter(startRect.center());

    zoomOut->setStartValue(startRect);
    zoomOut->setEndValue(midRect);

    QPropertyAnimation *fadeOut = new QPropertyAnimation(zoomOutEffect, "opacity");
    fadeOut->setDuration(m_duration/2);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);

    QParallelAnimationGroup *zoomOutGroup = new QParallelAnimationGroup;
    zoomOutGroup->addAnimation(zoomOut);
    zoomOutGroup->addAnimation(fadeOut);

    // 第二步：放大新图片
    QGraphicsOpacityEffect *zoomInEffect = new QGraphicsOpacityEffect(target);
    target->setGraphicsEffect(zoomInEffect);
    target->setPixmap(pixmap);

    QPropertyAnimation *zoomIn = new QPropertyAnimation(target, "geometry");
    zoomIn->setDuration(m_duration/2);
    zoomIn->setEasingCurve(QEasingCurve::InCubic);

    QRect endRect = startRect;

    zoomIn->setStartValue(midRect);
    zoomIn->setEndValue(endRect);

    QPropertyAnimation *fadeIn = new QPropertyAnimation(zoomInEffect, "opacity");
    fadeIn->setDuration(m_duration/2);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);

    QParallelAnimationGroup *zoomInGroup = new QParallelAnimationGroup;
    zoomInGroup->addAnimation(zoomIn);
    zoomInGroup->addAnimation(fadeIn);

    group->addAnimation(zoomOutGroup);
    group->addAnimation(zoomInGroup);

    // 动画结束后清理
    connect(group, &QSequentialAnimationGroup::finished, this, [target, zoomOutEffect, zoomInEffect, group, this]() {
        target->setGraphicsEffect(nullptr);
        cleanupAnimation(zoomOutEffect);
        cleanupAnimation(zoomInEffect);
        cleanupAnimation(group);
        emit animationFinished();
    });

    group->start();
}

void AnimationManager::flipAnimation(QLabel *target, const QPixmap &pixmap)
{
    // 创建3D翻转效果
    QVariantAnimation *flipAnimation = new QVariantAnimation(this);
    flipAnimation->setDuration(m_duration);
    flipAnimation->setStartValue(0);
    flipAnimation->setEndValue(180);

    QPixmap oldPixmap = target->pixmap();

    connect(flipAnimation, &QVariantAnimation::valueChanged, [target, oldPixmap, pixmap](const QVariant &value) {
        int angle = value.toInt();

        if (angle == 90) {
            // 在90度时切换图片
            target->setPixmap(pixmap);
        }

        // 设置3D变换 - 使用QGraphicsEffect替代setTransform
        QGraphicsEffect *oldEffect = target->graphicsEffect();
        if (oldEffect) {
            target->setGraphicsEffect(nullptr);
            oldEffect->deleteLater();
        }

        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(target);
        effect->setOpacity(qAbs(angle - 90) / 90.0); // 在90度时最透明
        target->setGraphicsEffect(effect);
    });

    connect(flipAnimation, &QVariantAnimation::finished, this, [target, flipAnimation, this]() {
        target->setGraphicsEffect(nullptr);
        cleanupAnimation(flipAnimation);
        emit animationFinished();
    });

    flipAnimation->start();
}

void AnimationManager::rotateAnimation(QLabel *target, const QPixmap &pixmap)
{
    // 保存原始pixmap
    QPixmap oldPixmap = target->pixmap();

    // 设置旋转和淡出效果
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(target);
    target->setGraphicsEffect(effect);

    // 旋转动画
    QVariantAnimation *rotateAnim = new QVariantAnimation(this);
    rotateAnim->setDuration(m_duration);
    rotateAnim->setStartValue(0);
    rotateAnim->setEndValue(360);

    connect(rotateAnim, &QVariantAnimation::valueChanged, [target, effect](const QVariant &value) {
        double angle = value.toDouble();
        // 使用透明度模拟旋转效果
        effect->setOpacity(1.0 - (angle / 360.0));
    });

    // 透明度动画
    QPropertyAnimation *opacityAnim = new QPropertyAnimation(effect, "opacity");
    opacityAnim->setDuration(m_duration);
    opacityAnim->setStartValue(1.0);
    opacityAnim->setEndValue(0.0);

    // 并行执行动画
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    group->addAnimation(rotateAnim);
    group->addAnimation(opacityAnim);

    // 动画中途切换图片
    QTimer::singleShot(m_duration/2, this, [target, pixmap]() {
        target->setPixmap(pixmap);

        // 重置透明度
        if (target->graphicsEffect()) {
            QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(target->graphicsEffect());
            if (effect) {
                effect->setOpacity(0.0);
            }
        }
    });

    // 动画结束后清理
    connect(group, &QParallelAnimationGroup::finished, this, [target, effect, group, this]() {
        target->setGraphicsEffect(nullptr);
        cleanupAnimation(effect);
        cleanupAnimation(group);
        emit animationFinished();
    });

    group->start();
}

void AnimationManager::cubeAnimation(QLabel *target, const QPixmap &pixmap, int direction)
{
    // 创建两个临时标签用于3D效果
    QLabel *frontLabel = new QLabel(target->parentWidget());
    frontLabel->setPixmap(target->pixmap());
    frontLabel->setGeometry(target->geometry());
    frontLabel->setAlignment(Qt::AlignCenter);
    frontLabel->setAttribute(Qt::WA_DeleteOnClose);
    frontLabel->show();
    frontLabel->raise();

    QLabel *backLabel = new QLabel(target->parentWidget());
    backLabel->setPixmap(pixmap);
    backLabel->setGeometry(target->geometry());
    backLabel->setAlignment(Qt::AlignCenter);
    backLabel->setAttribute(Qt::WA_DeleteOnClose);
    backLabel->show();
    backLabel->raise();

    // 更新主标签
    target->setPixmap(pixmap);

    // 创建动画组
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

    // 前标签动画 - 3D旋转
    QVariantAnimation *frontAnim = new QVariantAnimation(this);
    frontAnim->setDuration(m_duration);
    frontAnim->setStartValue(0);
    frontAnim->setEndValue(90 * direction);

    connect(frontAnim, &QVariantAnimation::valueChanged, [frontLabel, direction](const QVariant &value) {
        double angle = value.toDouble();

        // 使用透明度模拟3D旋转效果
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(frontLabel);
        effect->setOpacity(1.0 - qAbs(angle) / 90.0);
        frontLabel->setGraphicsEffect(effect);
    });

    // 后标签动画 - 3D旋转
    QVariantAnimation *backAnim = new QVariantAnimation(this);
    backAnim->setDuration(m_duration);
    backAnim->setStartValue(-90 * direction);
    backAnim->setEndValue(0);

    connect(backAnim, &QVariantAnimation::valueChanged, [backLabel, direction](const QVariant &value) {
        double angle = value.toDouble();

        // 使用透明度模拟3D旋转效果
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(backLabel);
        effect->setOpacity(1.0 - qAbs(angle) / 90.0);
        backLabel->setGraphicsEffect(effect);
    });

    group->addAnimation(frontAnim);
    group->addAnimation(backAnim);

    // 动画结束后删除临时标签
    connect(group, &QParallelAnimationGroup::finished, this, [frontLabel, backLabel, group, this]() {
        frontLabel->close();
        backLabel->close();
        cleanupAnimation(group);
        emit animationFinished();
    });

    group->start();
}

/*oid AnimationManager::blurAnimation(QLabel *target, const QPixmap &pixmap)
{
    // 保存原始pixmap
    QPixmap oldPixmap = target->pixmap();

    // 设置模糊效果
    QGraphicsBlurEffect *blurEffect = new QGraphicsBlurEffect(target);
    target->setGraphicsEffect(blurEffect);

    // 模糊动画
    QPropertyAnimation *blurAnimation = new QPropertyAnimation(blurEffect, "blurRadius");
    blurAnimation->setDuration(m_duration/2);
    blurAnimation->setStartValue(0);
    blurAnimation->setEndValue(10);
    blurAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 透明度动画
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(target);
    target->setGraphicsEffect(opacityEffect);

    QPropertyAnimation *opacityAnimation = new QPropertyAnimation(opacityEffect, "opacity");
    opacityAnimation->setDuration(m_duration/2);
    opacityAnimation->setStartValue(1.0);
    opacityAnimation->setEndValue(0.3);
    opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 第一阶段：模糊和淡化
    QParallelAnimationGroup *phase1 = new QParallelAnimationGroup(this);
    phase1->addAnimation(blurAnimation);
    phase1->addAnimation(opacityAnimation);

    // 第二阶段：切换图片并恢复
    QSequentialAnimationGroup *group = new QSequentialAnimationGroup(this);
    group->addAnimation(phase1);

    // 在第二阶段切换图片
    connect(phase1, &QParallelAnimationGroup::finished, this, [target, pixmap]() {
        target->setPixmap(pixmap);
    });

    // 恢复动画
    QPropertyAnimation *blurBackAnimation = new QPropertyAnimation(blurEffect, "blurRadius");
    blurBackAnimation->setDuration(m_duration/2);
    blurBackAnimation->setStartValue(10);
    blurBackAnimation->setEndValue(0);
    blurBackAnimation->setEasingCurve(QEasingCurve::InCubic);

    QPropertyAnimation *opacityBackAnimation = new QPropertyAnimation(opacityEffect, "opacity");
    opacityBackAnimation->setDuration(m_duration/2);
    opacityBackAnimation->setStartValue(0.3);
    opacityBackAnimation->setEndValue(1.0);
    opacityBackAnimation->setEasingCurve(QEasingCurve::InCubic);

    QParallelAnimationGroup *phase2 = new QParallelAnimationGroup(this);
    phase2->addAnimation(blurBackAnimation);
    phase2->addAnimation(opacityBackAnimation);

    group->addAnimation(phase2);

    // 动画结束后清理效果
    connect(group, &QSequentialAnimationGroup::finished, this, [target, blurEffect, opacityEffect, group, this]() {
        target->setGraphicsEffect(nullptr);
        cleanupAnimation(blurEffect);
        cleanupAnimation(opacityEffect);
        cleanupAnimation(group);
        emit animationFinished();
    });

    group->start();
}*/
    void AnimationManager::blurAnimation(QLabel *target, const QPixmap &pixmap)
{
    // 保存原始pixmap
    QPixmap oldPixmap = target->pixmap();

    // 创建临时标签用于显示模糊效果
    QLabel *tempLabel = new QLabel(target->parentWidget());
    tempLabel->setPixmap(oldPixmap);
    tempLabel->setGeometry(target->geometry());
    tempLabel->setAlignment(Qt::AlignCenter);
    tempLabel->setAttribute(Qt::WA_DeleteOnClose);
    tempLabel->show();
    tempLabel->raise();

    // 更新主标签
    target->setPixmap(pixmap);
    target->setGraphicsEffect(nullptr); // 确保没有旧的效果

    // 设置模糊效果到临时标签
    QGraphicsBlurEffect *blurEffect = new QGraphicsBlurEffect(tempLabel);
    tempLabel->setGraphicsEffect(blurEffect);

    // 模糊动画
    QPropertyAnimation *blurAnimation = new QPropertyAnimation(blurEffect, "blurRadius");
    blurAnimation->setDuration(m_duration);
    blurAnimation->setStartValue(0);
    blurAnimation->setEndValue(10);
    blurAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 透明度动画
    QPropertyAnimation *opacityAnimation = new QPropertyAnimation(tempLabel, "windowOpacity");
    opacityAnimation->setDuration(m_duration);
    opacityAnimation->setStartValue(1.0);
    opacityAnimation->setEndValue(0.0);
    opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 并行执行动画
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    group->addAnimation(blurAnimation);
    group->addAnimation(opacityAnimation);

    // 动画结束后删除临时标签
    connect(group, &QParallelAnimationGroup::finished, this, [tempLabel, blurEffect, group, this]() {
        tempLabel->close();
        cleanupAnimation(blurEffect);
        cleanupAnimation(group);
        emit animationFinished();
    });

    group->start();
}

void AnimationManager::glowAnimation(QLabel *target, const QPixmap &pixmap)
{
    // 保存原始pixmap
    QPixmap oldPixmap = target->pixmap();

    // 创建临时标签用于显示发光效果
    QLabel *tempLabel = new QLabel(target->parentWidget());
    tempLabel->setPixmap(oldPixmap);
    tempLabel->setGeometry(target->geometry());
    tempLabel->setAlignment(Qt::AlignCenter);
    tempLabel->setAttribute(Qt::WA_DeleteOnClose);
    tempLabel->show();
    tempLabel->raise();

    // 更新主标签
    target->setPixmap(pixmap);
    target->setGraphicsEffect(nullptr); // 确保没有旧的效果

    // 设置发光效果到临时标签
    QGraphicsColorizeEffect *glowEffect = new QGraphicsColorizeEffect(tempLabel);
    glowEffect->setColor(Qt::yellow);
    glowEffect->setStrength(0);
    tempLabel->setGraphicsEffect(glowEffect);

    // 发光动画
    QPropertyAnimation *glowAnimation = new QPropertyAnimation(glowEffect, "strength");
    glowAnimation->setDuration(m_duration);
    glowAnimation->setStartValue(0.0);
    glowAnimation->setEndValue(1.0);
    glowAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    // 透明度动画
    QPropertyAnimation *opacityAnimation = new QPropertyAnimation(tempLabel, "windowOpacity");
    opacityAnimation->setDuration(m_duration);
    opacityAnimation->setStartValue(1.0);
    opacityAnimation->setEndValue(0.0);
    opacityAnimation->setEasingCurve(QEasingCurve::InQuad);

    // 并行执行动画
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    group->addAnimation(glowAnimation);
    group->addAnimation(opacityAnimation);

    // 动画结束后删除临时标签
    connect(group, &QParallelAnimationGroup::finished, this, [tempLabel, glowEffect, group, this]() {
        tempLabel->close();
        cleanupAnimation(glowEffect);
        cleanupAnimation(group);
        emit animationFinished();
    });

    group->start();
}
void AnimationManager::waterDropAnimation(QLabel *target, const QPixmap &pixmap)
{
    // 创建波纹效果
    QLabel *rippleLabel = new QLabel(target->parentWidget());
    rippleLabel->setPixmap(target->pixmap());
    rippleLabel->setGeometry(target->geometry());
    rippleLabel->setAlignment(Qt::AlignCenter);
    rippleLabel->setAttribute(Qt::WA_DeleteOnClose);
    rippleLabel->show();
    rippleLabel->raise();

    // 更新主标签
    target->setPixmap(pixmap);

    // 创建波纹动画
    QVariantAnimation *rippleAnimation = new QVariantAnimation(this);
    rippleAnimation->setDuration(m_duration);
    rippleAnimation->setStartValue(0.0);
    rippleAnimation->setEndValue(1.0);

    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(rippleLabel);
    rippleLabel->setGraphicsEffect(opacityEffect);

    connect(rippleAnimation, &QVariantAnimation::valueChanged, [rippleLabel, opacityEffect](const QVariant &value) {
        double progress = value.toDouble();

        // 创建波纹效果
        QPixmap original = rippleLabel->pixmap();
        QPixmap ripplePixmap(original.size());
        ripplePixmap.fill(Qt::transparent);

        QPainter painter(&ripplePixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制原始图像
        painter.drawPixmap(0, 0, original);

        // 应用波纹效果
        QPainterPath clipPath;
        int centerX = ripplePixmap.width() / 2;
        int centerY = ripplePixmap.height() / 2;
        int maxRadius = qSqrt(centerX * centerX + centerY * centerY);
        int radius = maxRadius * progress;

        clipPath.addEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);
        painter.setClipPath(clipPath);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(ripplePixmap.rect(), Qt::transparent);

        // 设置透明度
        opacityEffect->setOpacity(1.0 - progress);
    });

    connect(rippleAnimation, &QVariantAnimation::finished, this, [rippleLabel, rippleAnimation, this]() {
        rippleLabel->close();
        cleanupAnimation(rippleAnimation);
        emit animationFinished();
    });

    rippleAnimation->start();
}

void AnimationManager::pageCurlAnimation(QLabel *target, const QPixmap &pixmap)
{
    // 创建翻页效果
    QLabel *curlLabel = new QLabel(target->parentWidget());
    curlLabel->setPixmap(target->pixmap());
    curlLabel->setGeometry(target->geometry());
    curlLabel->setAlignment(Qt::AlignCenter);
    curlLabel->setAttribute(Qt::WA_DeleteOnClose);
    curlLabel->show();
    curlLabel->raise();

    // 更新主标签
    target->setPixmap(pixmap);

    // 创建翻页动画
    QVariantAnimation *curlAnimation = new QVariantAnimation(this);
    curlAnimation->setDuration(m_duration);
    curlAnimation->setStartValue(0.0);
    curlAnimation->setEndValue(1.0);

    connect(curlAnimation, &QVariantAnimation::valueChanged, [curlLabel](const QVariant &value) {
        double progress = value.toDouble();

        // 创建翻页效果
        QPixmap original = curlLabel->pixmap();
        QPixmap curlPixmap(original.size());
        curlPixmap.fill(Qt::transparent);

        QPainter painter(&curlPixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制翻页效果
        QTransform transform;
        transform.translate(original.width() * progress, 0);
        transform.rotate(90 * progress, Qt::YAxis);
        painter.setTransform(transform);

        painter.drawPixmap(0, 0, original);

        curlLabel->setPixmap(curlPixmap);
    });

    connect(curlAnimation, &QVariantAnimation::finished, this, [curlLabel, curlAnimation, this]() {
        curlLabel->close();
        cleanupAnimation(curlAnimation);
        emit animationFinished();
    });

    curlAnimation->start();
}
