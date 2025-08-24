#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H

#include <QObject>
#include <QLabel>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

class AnimationManager : public QObject
{
    Q_OBJECT

public:
    enum AnimationType {
        NoAnimation,
        SlideAnimation,
        FadeAnimation,
        ZoomAnimation,
        FlipAnimation,
        RotateAnimation,
        CubeAnimation,
        BlurAnimation
    };

    explicit AnimationManager(QObject *parent = nullptr);

    void setDuration(int duration) { m_duration = duration; }
    int duration() const { return m_duration; }

    void applyAnimation(QLabel *target, const QPixmap &newPixmap, AnimationType type, int direction = 1);

    // 静态工具方法
    static void cleanupAnimation(QObject *animation);

signals:
    void animationFinished();

private:
    void slideAnimation(QLabel *target, const QPixmap &pixmap, int direction);
    void fadeAnimation(QLabel *target, const QPixmap &pixmap);
    void zoomAnimation(QLabel *target, const QPixmap &pixmap);
    void flipAnimation(QLabel *target, const QPixmap &pixmap);
    void rotateAnimation(QLabel *target, const QPixmap &pixmap);
    void cubeAnimation(QLabel *target, const QPixmap &pixmap, int direction);
    void blurAnimation(QLabel *target, const QPixmap &pixmap);

    int m_duration;

    Q_DISABLE_COPY(AnimationManager)
};

#endif // ANIMATIONMANAGER_H
