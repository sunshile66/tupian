#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QPixmap>
#include <QStringList>
#include <QListWidgetItem>
#include <QCache>
#include <QFutureWatcher>
#include <QScopedPointer>
#include <QStackedWidget>

#include "imageprocessor.h"
#include "animationmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 视图模式枚举
enum ViewMode {
    SingleView,
    GridView,
    SlideshowView
};

// 可点击图片标签
class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget* parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    // 文件
    void onOpenImage();
    void onClose();

    // 导航
    void onPrevious();
    void onNext();

    // 操作
    void onZoomIn();
    void onZoomOut();
    void onRotateLeft();
    void onRotateRight();

    // 视图模式
    void onViewModeChanged(int index);
    void onGridColumnsChanged(int value);

    // 幻灯片
    void onSlideshowToggle();
    void onSlideshowIntervalChanged(int value);
    void onLoopSlideshowToggled(bool checked);
    void onRandomOrderToggled(bool checked);
    void updateSlideshow();

    // 选择 / 滤镜
    void onSelectionModeToggled(bool checked);
    void onSelectAll();
    void onClearSelection();
    void onShowSelected();
    void onApplyFilter();

    // 动画
    void onAnimationTypeChanged(int index);
    void onAnimationSpeedChanged(int value);

    // 列表
    void onImageListItemClicked(QListWidgetItem *item);

    // 系统
    void onCheckUpdates();
    void onAbout();

    // 网格优化
    void delayedUpdateGridView();
    void onImageLoaded(int index, QPixmap pixmap);
    void onScrollBarValueChanged();
private:
    // 添加互斥锁保护共享资源
    QMutex m_gridMutex;

    // 添加视图切换标志
    bool m_isSwitchingView = false;

private:
    // UI
    QScopedPointer<Ui::MainWindow> ui;

    // 管理器
    ImageProcessor *m_imageProcessor = nullptr;
    AnimationManager *m_animationManager = nullptr;

    // 当前状态
    QPixmap m_currentPixmap;
    int m_currentRotation = 0;
    double m_currentScale = 1.0;
    int m_currentImageIndex = 0;

    // 文件 / 选择
    QStringList m_imagePaths;
    QList<int> m_selectedIndices;

    // 配置
    ViewMode m_viewMode = SingleView;
    int m_gridColumns = 4;
    bool m_loopSlideshow = true;
    int m_slideshowInterval = 3000;
    bool m_randomOrder = false;
    QVector<int> m_slideshowOrder;

    // 幻灯片 / 网格
    QTimer *m_slideShowTimer = nullptr;
    QTimer *m_gridUpdateTimer = nullptr;
    QCache<QString, QPixmap> m_pixmapCache;
    QHash<int, QPixmap> m_gridPixmaps;
    QFutureWatcher<QPixmap> *m_imageLoader = nullptr;
    int m_lastVisibleRow = -1;
    int m_lastVisibleColumn = -1;

    // 初始化
    void initUI();
    void initConnections();
    void applyModernStyle();

    // 核心功能
    void loadImage(const QString &path);
    void loadImageWithAnimation(const QString &path, int direction = 1);
    void updateImageDisplay();
    void updateGridView(bool force = false);
    void updateVisibleGridItems();
    void updateSlideshowOrder();
    void updateButtonStates();

    // 辅助方法
    QPixmap getScaledPixmap() const;
    void updateStatusInfo();
    void showStatusMessage(const QString &message, int timeout = 5000);
    QPixmap loadPixmapForGrid(const QString &path, int width, int height);

    Q_DISABLE_COPY(MainWindow)
};

#endif // MAINWINDOW_H
