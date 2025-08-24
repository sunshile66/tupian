#include "animationmanager.h"
#include <QGraphicsOpacityEffect>
#include <QGraphicsBlurEffect>
#include <QEasingCurve>
#include <QVariantAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QDebug>

AnimationManager::AnimationManager(QObject *parent)
    : QObject(parent)
    , m_duration(300)
{
}

void AnimationManager::applyAnimation(QLabel *target, const QPixmap &newPixmap, AnimationType type, int direction)
{
    if (!target) return;

    // 如果正在执行动画，先停止所有动画
    if (target->graphicsEffect()) {
        QGraphicsEffect* effect = target->graphicsEffect();
        // 移除动画关联
        effect->setEnabled(false);
        target->setGraphicsEffect(nullptr);
        effect->deleteLater();
    }

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
    }
}

void AnimationManager::cleanupAnimation(QObject *animation)
{
    if (animation) {
        // 先停止动画
        if (QAbstractAnimation* anim = qobject_cast<QAbstractAnimation*>(animation)) {
            anim->stop();
            anim->disconnect();
        }
        animation->deleteLater();
    }
}

void AnimationManager::slideAnimation(QLabel *target, const QPixmap &pixmap, int direction)
{
    if (!target || !target->parentWidget()) return;

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
        startRect.moveTo(-startRect.width(), startRect.y());
        tempLabel->setGeometry(startRect);
        endRect.moveTo(startRect.width(), startRect.y());
    } else {
        // 向左滑动
        startRect.moveTo(startRect.width(), startRect.y());
        tempLabel->setGeometry(startRect);
        endRect.moveTo(-startRect.width(), startRect.y());
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
    if (!target) return;

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
        if (target) {
            target->setGraphicsEffect(nullptr);
        }
        cleanupAnimation(fadeInEffect);
        cleanupAnimation(fadeInAnimation);
        emit animationFinished();
    });

    fadeInAnimation->start();
}

void AnimationManager::zoomAnimation(QLabel *target, const QPixmap &pixmap)
{
    if (!target) return;

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
        if (target) {
            target->setGraphicsEffect(nullptr);
        }
        cleanupAnimation(zoomOutEffect);
        cleanupAnimation(zoomInEffect);
        cleanupAnimation(group);
        emit animationFinished();
    });

    group->start();
}

void AnimationManager::flipAnimation(QLabel *target, const QPixmap &pixmap)
{
    if (!target) return;

    // 创建3D翻转效果
    QVariantAnimation *flipAnimation = new QVariantAnimation(this);
    flipAnimation->setDuration(m_duration);
    flipAnimation->setStartValue(0);
    flipAnimation->setEndValue(180);

    QPixmap oldPixmap = target->pixmap();

    connect(flipAnimation, &QVariantAnimation::valueChanged, [target, oldPixmap, pixmap](const QVariant &value) {
        if (!target) return;

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
        if (target) {
            target->setGraphicsEffect(nullptr);
        }
        cleanupAnimation(flipAnimation);
        emit animationFinished();
    });

    flipAnimation->start();
}

void AnimationManager::rotateAnimation(QLabel *target, const QPixmap &pixmap)
{
    if (!target) return;

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
        if (!target || !effect) return;

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
        if (target) {
            target->setPixmap(pixmap);

            // 重置透明度
            if (target->graphicsEffect()) {
                QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(target->graphicsEffect());
                if (effect) {
                    effect->setOpacity(0.0);
                }
            }
        }
    });

    // 动画结束后清理
    connect(group, &QParallelAnimationGroup::finished, this, [target, effect, group, this]() {
        if (target) {
            target->setGraphicsEffect(nullptr);
        }
        cleanupAnimation(effect);
        cleanupAnimation(group);
        emit animationFinished();
    });

    group->start();
}

void AnimationManager::cubeAnimation(QLabel *target, const QPixmap &pixmap, int direction)
{
    if (!target || !target->parentWidget()) return;

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
        if (!frontLabel) return;

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
        if (!backLabel) return;

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
        if (frontLabel) frontLabel->close();
        if (backLabel) backLabel->close();
        cleanupAnimation(group);
        emit animationFinished();
    });

    group->start();
}

void AnimationManager::blurAnimation(QLabel *target, const QPixmap &pixmap)
{
    if (!target) return;

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
        if (target) {
            target->setPixmap(pixmap);
        }
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
        if (target) {
            target->setGraphicsEffect(nullptr);
        }
        cleanupAnimation(blurEffect);
        cleanupAnimation(opacityEffect);
        cleanupAnimation(group);
        emit animationFinished();
    });

    group->start();
}
