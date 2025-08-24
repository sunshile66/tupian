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
        BlurAnimation,
        GlowAnimation,
        WaterDropAnimation,
        PageCurlAnimation
    };

    explicit AnimationManager(QObject *parent = nullptr);

    void setDuration(int duration) { m_duration = duration; }
    int duration() const { return m_duration; }
    void setSoundEnabled(bool enabled) { m_soundEnabled = enabled; }
    bool isSoundEnabled() const { return m_soundEnabled; }

    void applyAnimation(QLabel *target, const QPixmap &newPixmap, AnimationType type, int direction = 1);

    // 静态工具方法
    static void cleanupAnimation(QObject *animation);

signals:
    void animationFinished();
    void animationStarted();

private:
    void slideAnimation(QLabel *target, const QPixmap &pixmap, int direction);
    void fadeAnimation(QLabel *target, const QPixmap &pixmap);
    void zoomAnimation(QLabel *target, const QPixmap &pixmap);
    void flipAnimation(QLabel *target, const QPixmap &pixmap);
    void rotateAnimation(QLabel *target, const QPixmap &pixmap);
    void cubeAnimation(QLabel *target, const QPixmap &pixmap, int direction);
    void blurAnimation(QLabel *target, const QPixmap &pixmap);
    void glowAnimation(QLabel *target, const QPixmap &pixmap);
    void waterDropAnimation(QLabel *target, const QPixmap &pixmap);
    void pageCurlAnimation(QLabel *target, const QPixmap &pixmap);

    void playAnimationSound();

    int m_duration;
    bool m_soundEnabled;

    Q_DISABLE_COPY(AnimationManager)
};

#endif // ANIMATIONMANAGER_H
