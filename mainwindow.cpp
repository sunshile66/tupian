#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QtConcurrent>
#include <QStandardPaths>
#include <QCloseEvent>
#include <QMouseEvent>

// ------------------ ClickableLabel ------------------
ClickableLabel::ClickableLabel(QWidget* parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setStyleSheet("border: 1px solid #ccc; margin: 2px; background: #f8f9fa;");
    setMinimumSize(100, 100);
}

void ClickableLabel::mousePressEvent(QMouseEvent* e) {
    QLabel::mousePressEvent(e);
    emit clicked();
}

// ------------------ MainWindow ------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    m_imageProcessor(new ImageProcessor(this)),
    m_animationManager(new AnimationManager(this)),
    m_slideShowTimer(new QTimer(this)),
    m_gridUpdateTimer(new QTimer(this)),
    m_pixmapCache(50 * 1024 * 1024), // 50MB缓存
    m_imageLoader(new QFutureWatcher<QPixmap>(this)),
    m_enhancementLoader(new QFutureWatcher<QPixmap>(this)),
    m_isSwitchingView(false)
{
    ui->setupUi(this);

    initUI();
    initConnections();
    applyModernStyle();

    // 初始化滤镜列表
    ui->filterCombo->addItem("无滤镜", ImageProcessor::NoFilter);
    ui->filterCombo->addItem("灰度滤镜", ImageProcessor::GrayscaleFilter);
    ui->filterCombo->addItem("复古滤镜", ImageProcessor::SepiaFilter);
    ui->filterCombo->addItem("模糊滤镜", ImageProcessor::BlurFilter);
    ui->filterCombo->addItem("亮度调节", ImageProcessor::BrightnessFilter);
    ui->filterCombo->addItem("对比度调节", ImageProcessor::ContrastFilter);
    ui->filterCombo->addItem("锐化", ImageProcessor::SharpnessFilter);
    ui->filterCombo->addItem("综合增强", ImageProcessor::EnhanceFilter);
    ui->filterCombo->addItem("超分辨率", ImageProcessor::SuperResolutionFilter);
    ui->filterCombo->addItem("去除水印", ImageProcessor::RemoveWatermarkFilter);

    connect(m_imageLoader, &QFutureWatcher<QPixmap>::finished, this, [this]() {
        // 清理已完成的任务
        m_imageLoader->disconnect();
    });

    connect(m_enhancementLoader, &QFutureWatcher<QPixmap>::finished, this, [this]() {
        if (m_enhancementLoader->isCanceled()) return;

        QPixmap result = m_enhancementLoader->result();
        if (!result.isNull()) {
            m_currentPixmap = result;
            updateImageDisplay();
            statusBar()->showMessage("图片增强完成", 3000);
        }
    });
}

MainWindow::~MainWindow() {
    // 停止所有动画和计时器
    m_slideShowTimer->stop();
    m_gridUpdateTimer->stop();

    // 取消异步加载
    m_imageLoader->cancel();
    m_imageLoader->waitForFinished();

    m_enhancementLoader->cancel();
    m_enhancementLoader->waitForFinished();

    // 清除所有缓存
    m_pixmapCache.clear();
    m_gridPixmaps.clear();
}

// ------------------ Events ------------------
void MainWindow::resizeEvent(QResizeEvent *e) {
    QMainWindow::resizeEvent(e);
    if (!m_currentPixmap.isNull()) updateImageDisplay();
    if (m_viewMode == GridView && !m_imagePaths.isEmpty()) m_gridUpdateTimer->start(100);
}

void MainWindow::showEvent(QShowEvent *e) {
    QMainWindow::showEvent(e);
    if (auto bar = ui->scrollArea->verticalScrollBar()) {
        connect(bar, &QScrollBar::valueChanged, this, &MainWindow::onScrollBarValueChanged, Qt::UniqueConnection);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // 确保所有异步操作完成
    m_imageLoader->cancel();
    m_imageLoader->waitForFinished();

    m_enhancementLoader->cancel();
    m_enhancementLoader->waitForFinished();

    event->accept();
}

// ------------------ UI Init ------------------
void MainWindow::initUI() {
    setWindowTitle(QApplication::applicationName() + " v" + QApplication::applicationVersion());
    ui->imageLabel->setAlignment(Qt::AlignCenter);
    ui->imageLabel->setMinimumSize(400, 300);
    ui->imageLabel->setText("请点击'加载图片'");
    ui->imageLabel->setStyleSheet("QLabel { background: #f0f0f0; border: 1px solid #ccc; }");

    // 初始化动画类型下拉框
    ui->animationTypeCombo->addItem("无动画", AnimationManager::NoAnimation);
    ui->animationTypeCombo->addItem("滑动动画", AnimationManager::SlideAnimation);
    ui->animationTypeCombo->addItem("淡入淡出", AnimationManager::FadeAnimation);
    ui->animationTypeCombo->addItem("缩放动画", AnimationManager::ZoomAnimation);
    ui->animationTypeCombo->addItem("3D翻转", AnimationManager::FlipAnimation);
    ui->animationTypeCombo->addItem("旋转动画", AnimationManager::RotateAnimation);
    ui->animationTypeCombo->addItem("立方体旋转", AnimationManager::CubeAnimation);
    ui->animationTypeCombo->addItem("模糊过渡", AnimationManager::BlurAnimation);
    ui->animationTypeCombo->addItem("发光效果", AnimationManager::GlowAnimation);
    ui->animationTypeCombo->addItem("水滴效果", AnimationManager::WaterDropAnimation);
    ui->animationTypeCombo->addItem("翻页效果", AnimationManager::PageCurlAnimation);
    ui->animationTypeCombo->setCurrentIndex(1);

    // 初始化视图模式下拉框
    ui->viewModeCombo->addItem("单图视图", SingleView);
    ui->viewModeCombo->addItem("网格视图", GridView);
    ui->viewModeCombo->addItem("幻灯片视图", SlideshowView);
    ui->viewModeCombo->setCurrentIndex(0);

    // 设置动画速度
    ui->animationSpeedSlider->setRange(100, 2000);
    ui->animationSpeedSlider->setValue(500);
    ui->animationSpeedLabel->setText("动画速度: 500ms");
    m_animationManager->setDuration(500);

    // 设置网格列数
    ui->gridColumnsSpin->setRange(2, 6);
    ui->gridColumnsSpin->setValue(4);

    // 设置幻灯片间隔
    ui->slideshowIntervalSpin->setRange(1000, 10000);
    ui->slideshowIntervalSpin->setValue(3000);
    ui->slideshowIntervalSpin->setSuffix(" ms");

    m_gridUpdateTimer->setSingleShot(true);
    updateButtonStates();

    // 设置列表控件
    ui->imageListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    // 设置状态栏信息
    statusBar()->showMessage("就绪", 3000);
}

void MainWindow::initConnections() {
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenImage);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onClose);
    connect(ui->actionSingleView, &QAction::triggered, [this]{ui->viewModeCombo->setCurrentIndex(0);});
    connect(ui->actionGridView,   &QAction::triggered, [this]{ui->viewModeCombo->setCurrentIndex(1);});
    connect(ui->actionSlideshow,  &QAction::triggered, [this]{ui->viewModeCombo->setCurrentIndex(2);});

    connect(ui->showImageBtn, &QPushButton::clicked, this, &MainWindow::onOpenImage);
    connect(ui->prevBtn, &QPushButton::clicked, this, &MainWindow::onPrevious);
    connect(ui->nextBtn, &QPushButton::clicked, this, &MainWindow::onNext);
    connect(ui->zoomInBtn, &QPushButton::clicked, this, &MainWindow::onZoomIn);
    connect(ui->zoomOutBtn, &QPushButton::clicked, this, &MainWindow::onZoomOut);
    connect(ui->rotateLeftBtn, &QPushButton::clicked, this, &MainWindow::onRotateLeft);
    connect(ui->rotateRightBtn, &QPushButton::clicked, this, &MainWindow::onRotateRight);
    connect(ui->slideShowBtn, &QPushButton::clicked, this, &MainWindow::onSlideshowToggle);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onClose);

    connect(ui->animationTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAnimationTypeChanged);
    connect(ui->animationSpeedSlider, &QSlider::valueChanged, this, &MainWindow::onAnimationSpeedChanged);
    connect(ui->viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onViewModeChanged);
    connect(ui->gridColumnsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onGridColumnsChanged);

    connect(m_slideShowTimer, &QTimer::timeout, this, &MainWindow::updateSlideshow);
    connect(m_gridUpdateTimer, &QTimer::timeout, this, &MainWindow::delayedUpdateGridView);
    connect(m_animationManager, &AnimationManager::animationFinished, this, &MainWindow::updateStatusInfo);

    connect(m_imageLoader, &QFutureWatcher<QPixmap>::resultReadyAt, this,
            [this](int i){ onImageLoaded(i, m_imageLoader->resultAt(i)); });

    // 连接其他UI信号
    connect(ui->slideshowIntervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onSlideshowIntervalChanged);
    connect(ui->loopSlideshowCheck, &QCheckBox::toggled,
            this, &MainWindow::onLoopSlideshowToggled);
    connect(ui->randomOrderCheck, &QCheckBox::toggled,
            this, &MainWindow::onRandomOrderToggled);
    connect(ui->selectionModeCheck, &QCheckBox::toggled,
            this, &MainWindow::onSelectionModeToggled);
    connect(ui->selectAllBtn, &QPushButton::clicked,
            this, &MainWindow::onSelectAll);
    connect(ui->clearSelectionBtn, &QPushButton::clicked,
            this, &MainWindow::onClearSelection);
    connect(ui->showSelectedBtn, &QPushButton::clicked,
            this, &MainWindow::onShowSelected);
    connect(ui->applyFilterBtn, &QPushButton::clicked,
            this, &MainWindow::onApplyFilter);
    connect(ui->imageListWidget, &QListWidget::itemClicked,
            this, &MainWindow::onImageListItemClicked);
    connect(ui->checkUpdatesBtn, &QPushButton::clicked,
            this, &MainWindow::onCheckUpdates);
    connect(ui->actionAbout, &QAction::triggered,
            this, &MainWindow::onAbout);
    connect(ui->soundEffectsCheck, &QCheckBox::toggled, this, &MainWindow::onSoundEffectsToggled);
    connect(ui->backgroundMusicCheck, &QCheckBox::toggled, this, &MainWindow::onBackgroundMusicToggled);
}

// ------------------ Core Slots ------------------
void MainWindow::onOpenImage() {
    QString picturesPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QStringList files = QFileDialog::getOpenFileNames(this, "选择图片", picturesPath,
                                                      "图片 (*.png *.jpg *.jpeg *.bmp *.gif *.webp)");
    if (files.isEmpty()) return;

    m_imagePaths = files;
    m_currentImageIndex = 0;

    // 清空缓存
    m_pixmapCache.clear();
    m_gridPixmaps.clear();

    // 更新图片列表
    ui->imageListWidget->clear();
    for (const auto &f : files) {
        QListWidgetItem *item = new QListWidgetItem(QFileInfo(f).fileName());
        item->setData(Qt::UserRole, f);
        ui->imageListWidget->addItem(item);
    }

    // 更新幻灯片顺序
    updateSlideshowOrder();

    // 根据视图模式加载图片
    if (m_viewMode == SingleView) {
        loadImage(m_imagePaths.first());
    } else if (m_viewMode == GridView) {
        updateGridView(true);
    } else if (m_viewMode == SlideshowView) {
        loadImage(m_imagePaths.first());
        if (m_slideShowTimer->isActive()) {
            m_slideShowTimer->start();
        }
    }

    updateButtonStates();
    statusBar()->showMessage(QString("已加载 %1 张图片").arg(m_imagePaths.size()), 3000);
}

void MainWindow::onClose(){
    close();
}

void MainWindow::onPrevious(){
    if (m_imagePaths.isEmpty()) return;
    m_currentImageIndex = (m_currentImageIndex - 1 + m_imagePaths.size()) % m_imagePaths.size();
    loadImageWithAnimation(m_imagePaths[m_currentImageIndex], -1);

    // 更新列表选择
    ui->imageListWidget->setCurrentRow(m_currentImageIndex);
}

void MainWindow::onNext(){
    if (m_imagePaths.isEmpty()) return;
    m_currentImageIndex = (m_currentImageIndex + 1) % m_imagePaths.size();
    loadImageWithAnimation(m_imagePaths[m_currentImageIndex], 1);

    // 更新列表选择
    ui->imageListWidget->setCurrentRow(m_currentImageIndex);
}

void MainWindow::onZoomIn(){
    m_currentScale = qMin(5.0, m_currentScale * 1.2);
    updateImageDisplay();
}

void MainWindow::onZoomOut(){
    m_currentScale = qMax(0.1, m_currentScale / 1.2);
    updateImageDisplay();
}

void MainWindow::onRotateLeft(){
    m_currentRotation = (m_currentRotation - 90) % 360;
    updateImageDisplay();
}

void MainWindow::onRotateRight(){
    m_currentRotation = (m_currentRotation + 90) % 360;
    updateImageDisplay();
}

void MainWindow::onSlideshowToggle(){
    if (m_imagePaths.size() < 2) {
        QMessageBox::information(this, "提示", "请先加载多张图片以启动幻灯片播放");
        return;
    }

    if (m_slideShowTimer->isActive()) {
        m_slideShowTimer->stop();
        ui->slideShowBtn->setText("开始播放");
        statusBar()->showMessage("幻灯片播放已停止", 3000);
    } else {
        m_slideShowTimer->start(m_slideshowInterval);
        ui->slideShowBtn->setText("停止播放");
        statusBar()->showMessage("幻灯片播放已开始", 3000);
    }
}

void MainWindow::onAnimationTypeChanged(int index) {
    Q_UNUSED(index);
    // 动画类型已更改，下次切换图片时会使用新类型
}

void MainWindow::onAnimationSpeedChanged(int value) {
    m_animationManager->setDuration(value);
    ui->animationSpeedLabel->setText(QString("动画速度: %1ms").arg(value));
}

void MainWindow::onViewModeChanged(int index) {
    // 设置标志防止重入
    if (m_isSwitchingView) return;
    m_isSwitchingView = true;

    // 停止所有动画和计时器
    m_slideShowTimer->stop();
    m_animationManager->cleanupAnimation(m_animationManager);

    // 取消异步加载
    m_imageLoader->cancel();
    m_imageLoader->waitForFinished();

    m_viewMode = static_cast<ViewMode>(ui->viewModeCombo->itemData(index).toInt());

    // 更新UI显示
    ui->tabWidget->setCurrentIndex(m_viewMode);

    if (m_viewMode == SingleView && !m_imagePaths.isEmpty()) {
        loadImage(m_imagePaths[m_currentImageIndex]);
    } else if (m_viewMode == GridView && !m_imagePaths.isEmpty()) {
        updateGridView(true);
    } else if (m_viewMode == SlideshowView && !m_imagePaths.isEmpty()) {
        loadImage(m_imagePaths[m_currentImageIndex]);
        if (m_slideShowTimer->isActive()) {
            m_slideShowTimer->start(m_slideshowInterval);
        }
    }

    updateButtonStates();
    m_isSwitchingView = false;
}

void MainWindow::toggleBackgroundMusic(bool enabled)
{
    if (enabled) {
        // 实现背景音乐播放逻辑
        // 例如: m_mediaPlayer->play();
        statusBar()->showMessage("背景音乐已开启", 2000);
    } else {
        // 实现背景音乐停止逻辑
        // 例如: m_mediaPlayer->stop();
        statusBar()->showMessage("背景音乐已关闭", 2000);
    }
}

void MainWindow::onSoundEffectsToggled(bool checked)
{
    m_soundEffectsEnabled = checked;
    m_animationManager->setSoundEnabled(checked);
}

void MainWindow::onBackgroundMusicToggled(bool checked)
{
    m_backgroundMusicEnabled = checked;
    toggleBackgroundMusic(checked);
}

// ------------------ 新增的槽函数实现 ------------------
void MainWindow::onGridColumnsChanged(int value) {
    m_gridColumns = value;
    if (m_viewMode == GridView && !m_imagePaths.isEmpty()) {
        updateGridView(true);
    }
}

void MainWindow::onSlideshowIntervalChanged(int value) {
    m_slideshowInterval = value;
    if (m_slideShowTimer->isActive()) {
        m_slideShowTimer->setInterval(value);
    }
}

void MainWindow::onLoopSlideshowToggled(bool checked) {
    m_loopSlideshow = checked;
}

void MainWindow::onRandomOrderToggled(bool checked) {
    m_randomOrder = checked;
    updateSlideshowOrder();
}

void MainWindow::onSelectionModeToggled(bool checked) {
    if (checked) {
        ui->imageListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    } else {
        ui->imageListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->imageListWidget->clearSelection();
        if (m_currentImageIndex < ui->imageListWidget->count()) {
            ui->imageListWidget->setCurrentRow(m_currentImageIndex);
        }
    }
}

void MainWindow::onSelectAll() {
    ui->imageListWidget->selectAll();
}

void MainWindow::onClearSelection() {
    ui->imageListWidget->clearSelection();
    if (m_currentImageIndex < ui->imageListWidget->count()) {
        ui->imageListWidget->setCurrentRow(m_currentImageIndex);
    }
}

void MainWindow::onShowSelected() {
    QList<QListWidgetItem*> selectedItems = ui->imageListWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        int index = ui->imageListWidget->row(selectedItems.first());
        m_currentImageIndex = index;
        loadImageWithAnimation(m_imagePaths[index], 1);
    }
}

void MainWindow::onApplyFilter() {
    if (m_currentPixmap.isNull()) return;

    int filterIndex = ui->filterCombo->currentData().toInt();
    ImageProcessor::FilterType filterType = static_cast<ImageProcessor::FilterType>(filterIndex);

    int param = 0;
    if (filterType == ImageProcessor::BlurFilter) {
        param = 5; // 默认模糊半径
    } else if (filterType == ImageProcessor::BrightnessFilter) {
        param = 30; // 默认亮度增加值
    } else if (filterType == ImageProcessor::ContrastFilter) {
        param = 20; // 默认对比度增加值
    } else if (filterType == ImageProcessor::SharpnessFilter) {
        param = 5; // 默认锐化强度
    }

    // 应用滤镜
    QPixmap filtered = m_imageProcessor->applyFilter(m_currentPixmap, filterType, param);
    if (!filtered.isNull()) {
        m_currentPixmap = filtered;
        updateImageDisplay();
        statusBar()->showMessage("滤镜已应用", 2000);
    }
}

void MainWindow::onEnhanceImage() {
    if (m_currentPixmap.isNull()) return;

    statusBar()->showMessage("正在增强图片...", 0);

    // 异步增强图片
    m_enhancementLoader->setFuture(
        m_imageProcessor->enhanceImageAsync(m_currentPixmap, ImageProcessor::EnhanceColors, 70)
        );
}

void MainWindow::onRemoveWatermark() {
    if (m_currentPixmap.isNull()) return;

    // 假设水印在右上角区域
    QRect watermarkArea(m_currentPixmap.width() * 3/4, 0,
                        m_currentPixmap.width()/4, m_currentPixmap.height()/4);

    statusBar()->showMessage("正在去除水印...", 0);

    // 异步去除水印
    m_enhancementLoader->setFuture(
        m_imageProcessor->removeWatermarkAsync(m_currentPixmap, watermarkArea)
        );
}

void MainWindow::onImageListItemClicked(QListWidgetItem *item) {
    int index = ui->imageListWidget->row(item);
    if (index >= 0 && index < m_imagePaths.size()) {
        m_currentImageIndex = index;
        loadImageWithAnimation(m_imagePaths[index], 1);
    }
}

void MainWindow::onCheckUpdates() {
    QMessageBox::information(this, "检查更新", "当前已是最新版本");
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "关于",
                       "高级图片浏览器 v1.0.0\n\n"
                       "一个功能丰富的图片浏览工具，支持多种视图模式和动画效果。\n\n"
                       "功能特点:\n"
                       "- 多种图片切换动画效果\n"
                       "- 单图、网格、幻灯片三种视图模式\n"
                       "- 图片滤镜处理\n"
                       "- 图片缩放和旋转\n"
                       "- 幻灯片自动播放\n"
                       "- 背景音乐和音效\n"
                       "- 图片增强和去水印功能");
}

// ------------------ Slideshow ------------------
void MainWindow::updateSlideshow() {
    if (m_imagePaths.isEmpty()) return;

    if (m_randomOrder) {
        static int orderIndex = 0;
        orderIndex = (orderIndex + 1) % m_slideshowOrder.size();
        m_currentImageIndex = m_slideshowOrder[orderIndex];
    } else {
        m_currentImageIndex = (m_currentImageIndex + 1) % m_imagePaths.size();

        // 检查是否到达最后一张且不循环
        if (!m_loopSlideshow && m_currentImageIndex == 0) {
            m_slideShowTimer->stop();
            ui->slideShowBtn->setText("开始播放");
            statusBar()->showMessage("幻灯片播放已完成", 3000);
            return;
        }
    }

    loadImageWithAnimation(m_imagePaths[m_currentImageIndex], 1);

    // 更新列表选择
    ui->imageListWidget->setCurrentRow(m_currentImageIndex);
}

void MainWindow::updateSlideshowOrder() {
    m_slideshowOrder.resize(m_imagePaths.size());
    for (int i = 0; i < m_imagePaths.size(); ++i) {
        m_slideshowOrder[i] = i;
    }

    if (m_randomOrder) {
        // 随机打乱顺序
        for (int i = m_slideshowOrder.size() - 1; i > 0; --i) {
            int j = QRandomGenerator::global()->bounded(i + 1);
            qSwap(m_slideshowOrder[i], m_slideshowOrder[j]);
        }
    }
}

// ------------------ Helpers ------------------
void MainWindow::loadImage(const QString &path){
    // 检查缓存
    if (m_pixmapCache.contains(path)) {
        m_currentPixmap = *m_pixmapCache.object(path);
    } else {
        QPixmap p(path);
        if (p.isNull()) {
            QMessageBox::warning(this, "错误", "无法加载图片: " + path);
            return;
        }

        m_currentPixmap = m_imageProcessor->optimizePixmapSize(p);
        // 缓存图片
        m_pixmapCache.insert(path, new QPixmap(m_currentPixmap));
    }

    m_currentScale = 1.0;
    m_currentRotation = 0;
    updateImageDisplay();
    updateStatusInfo();
}

void MainWindow::loadImageWithAnimation(const QString &path, int direction){
    // 检查缓存
    QPixmap newPixmap;
    if (m_pixmapCache.contains(path)) {
        newPixmap = *m_pixmapCache.object(path);
    } else {
        QPixmap p(path);
        if (p.isNull()) {
            QMessageBox::warning(this, "错误", "无法加载图片: " + path);
            return;
        }
        newPixmap = m_imageProcessor->optimizePixmapSize(p);
        // 缓存图片
        m_pixmapCache.insert(path, new QPixmap(newPixmap));
    }

    m_currentPixmap = newPixmap;
    int type = ui->animationTypeCombo->currentData().toInt();
    QPixmap scaled = getScaledPixmap();
    m_animationManager->applyAnimation(ui->imageLabel, scaled,
                                       static_cast<AnimationManager::AnimationType>(type), direction);
}

void MainWindow::updateImageDisplay(){
    if (m_currentPixmap.isNull()) {
        ui->imageLabel->setText("没有图片可显示");
        return;
    }

    QPixmap scaled = getScaledPixmap();
    ui->imageLabel->setPixmap(scaled);
    updateStatusInfo();
}

QPixmap MainWindow::getScaledPixmap() const {
    return m_imageProcessor->scalePixmap(m_currentPixmap, ui->imageLabel->size(),
                                         m_currentScale, m_currentRotation);
}

void MainWindow::updateStatusInfo() {
    if (m_imagePaths.isEmpty()) {
        ui->statusLabel->setText("就绪");
        return;
    }

    QString status = QString("图片: %1/%2 | 缩放: %3% | 旋转: %4° | 尺寸: %5x%6")
                         .arg(m_currentImageIndex + 1)
                         .arg(m_imagePaths.size())
                         .arg(static_cast<int>(m_currentScale * 100))
                         .arg(m_currentRotation)
                         .arg(m_currentPixmap.width())
                         .arg(m_currentPixmap.height());

    ui->statusLabel->setText(status);
}

void MainWindow::showStatusMessage(const QString &message, int timeout) {
    statusBar()->showMessage(message, timeout);
}

void MainWindow::updateButtonStates() {
    bool hasImages = !m_imagePaths.isEmpty();
    bool hasMultipleImages = hasImages && m_imagePaths.size() > 1;

    ui->zoomInBtn->setEnabled(hasImages);
    ui->zoomOutBtn->setEnabled(hasImages);
    ui->rotateLeftBtn->setEnabled(hasImages);
    ui->rotateRightBtn->setEnabled(hasImages);
    ui->slideShowBtn->setEnabled(hasMultipleImages);
    ui->prevBtn->setEnabled(hasMultipleImages);
    ui->nextBtn->setEnabled(hasMultipleImages);
    ui->applyFilterBtn->setEnabled(hasImages);
}

// ------------------ Grid View ------------------
void MainWindow::updateGridView(bool force) {
    if (force) {
        delayedUpdateGridView();
    } else {
        m_gridUpdateTimer->start(100);
    }
}

void MainWindow::delayedUpdateGridView() {
    if (m_viewMode != GridView || m_imagePaths.isEmpty() || m_isSwitchingView) return;

    // 清除现有的网格内容
    QLayoutItem *child;
    while ((child = ui->gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    // 计算网格大小
    int containerWidth = ui->scrollArea->width() - 20; // 考虑滚动条宽度
    int itemWidth = containerWidth / m_gridColumns - 10;
    int itemHeight = itemWidth * 3 / 4; // 保持4:3的宽高比

    // 添加占位符到网格
    for (int i = 0; i < m_imagePaths.size(); ++i) {
        ClickableLabel *placeholderLabel = new ClickableLabel();
        placeholderLabel->setFixedSize(itemWidth, itemHeight);
        placeholderLabel->setText("加载中...");
        placeholderLabel->setProperty("imageIndex", i);

        int row = i / m_gridColumns;
        int col = i % m_gridColumns;
        ui->gridLayout->addWidget(placeholderLabel, row, col);

        // 连接点击事件
        connect(placeholderLabel, &ClickableLabel::clicked, this, [this, i]() {
            m_currentImageIndex = i;
            ui->viewModeCombo->setCurrentIndex(0); // 切换到单图视图
            loadImageWithAnimation(m_imagePaths[m_currentImageIndex], 1);
        });

        // 异步加载图片 - 使用互斥锁保护共享资源
        if (!m_gridPixmaps.contains(i)) {
            QMutexLocker locker(&m_gridMutex);
            QFuture<QPixmap> future = QtConcurrent::run([this, i, itemWidth, itemHeight]() {
                return loadPixmapForGrid(m_imagePaths[i], itemWidth, itemHeight);
            });
            m_imageLoader->setFuture(future);
        } else {
            // 如果已有缓存的图片，直接更新
            QMetaObject::invokeMethod(this, "onImageLoaded", Qt::QueuedConnection,
                                      Q_ARG(int, i), Q_ARG(QPixmap, m_gridPixmaps[i]));
        }
    }
}

void MainWindow::onImageLoaded(int index, QPixmap pixmap) {
    QMutexLocker locker(&m_gridMutex);

    if (index < 0 || index >= m_imagePaths.size() || m_viewMode != GridView || m_isSwitchingView) return;

    // 缓存图片
    m_gridPixmaps.insert(index, pixmap);

    // 更新对应的网格项
    int row = index / m_gridColumns;
    int col = index % m_gridColumns;

    QLayoutItem *item = ui->gridLayout->itemAtPosition(row, col);
    if (!item || !item->widget()) return;

    // 获取占位符标签
    ClickableLabel *label = qobject_cast<ClickableLabel*>(item->widget());
    if (!label) return;

    // 更新标签内容
    label->setPixmap(pixmap.scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    label->setText("");
}

QPixmap MainWindow::loadPixmapForGrid(const QString &path, int width, int height) {
    // 检查缓存
    if (m_pixmapCache.contains(path)) {
        return *m_pixmapCache.object(path);
    }

    // 加载图片
    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        return QPixmap();
    }

    // 缩放图片以适应网格
    QPixmap scaledPixmap = pixmap.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 缓存图片
    m_pixmapCache.insert(path, new QPixmap(scaledPixmap));

    return scaledPixmap;
}

void MainWindow::onScrollBarValueChanged() {
    if (m_viewMode == GridView) {
        updateVisibleGridItems();
    }
}

void MainWindow::updateVisibleGridItems() {
    if (m_viewMode != GridView) return;

    // 获取可见区域
    QRect visibleRect = ui->scrollArea->viewport()->rect();
    visibleRect.translate(ui->scrollArea->horizontalScrollBar()->value(),
                          ui->scrollArea->verticalScrollBar()->value());

    // 计算可见的行和列范围
    int firstVisibleRow = qMax(0, visibleRect.y() / (ui->scrollArea->widget()->height() / ((m_imagePaths.size() + m_gridColumns - 1) / m_gridColumns)));
    int lastVisibleRow = qMin((m_imagePaths.size() + m_gridColumns - 1) / m_gridColumns,
                              (visibleRect.bottom() / (ui->scrollArea->widget()->height() / ((m_imagePaths.size() + m_gridColumns - 1) / m_gridColumns))) + 2);

    // 如果可见区域没有变化，则跳过
    if (firstVisibleRow == m_lastVisibleRow && lastVisibleRow == m_lastVisibleRow) return;

    m_lastVisibleRow = firstVisibleRow;
    m_lastVisibleColumn = lastVisibleRow;

    // 只加载可见区域的图片
    for (int i = firstVisibleRow * m_gridColumns; i < qMin(lastVisibleRow * m_gridColumns, m_imagePaths.size()); ++i) {
        if (!m_gridPixmaps.contains(i)) {
            int itemWidth = ui->scrollArea->width() / m_gridColumns - 10;
            int itemHeight = itemWidth * 3 / 4;

            QFuture<QPixmap> future = QtConcurrent::run([this, i, itemWidth, itemHeight]() {
                return loadPixmapForGrid(m_imagePaths[i], itemWidth, itemHeight);
            });
            // 存储future以避免警告
            m_imageLoader->setFuture(future);
        }
    }
}

// ------------------ Style ------------------
void MainWindow::applyModernStyle() {
    // 设置应用程序样式
    QString styleSheet = R"(
        QMainWindow {
            background: #f5f7fa;
        }
        QGroupBox {
            font-weight: bold;
            border: 2px solid #dcdfe6;
            border-radius: 8px;
            margin-top: 1ex;
            padding-top: 10px;
            background: white;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
            color: #409eff;
        }
        QPushButton {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                          stop: 0 #409eff, stop: 1 #337ecc);
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 6px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #66b1ff;
        }
        QPushButton:pressed {
            background: #337ecc;
        }
        QPushButton:disabled {
            background: #c0c4cc;
            color: #909399;
        }
        QLabel#statusLabel {
            color: #606266;
            font-size: 12px;
            padding: 4px;
            background: rgba(255, 255, 255, 0.8);
            border-radius: 4px;
            border: 1px solid #ebeef5;
        }
        QScrollArea {
            border: 1px solid #dcdfe6;
            border-radius: 4px;
            background: white;
        }
        QListWidget {
            border: 1px solid #dcdfe6;
            border-radius: 4px;
            background: white;
        }
        QListWidget::item {
            padding: 5px;
            border-bottom: 1px solid #ebeef5;
        }
        QListWidget::item:selected {
            background: #ecf5ff;
            color: #409eff;
        }
        QListWidget::item:hover {
            background: #f5f7fa;
        }
        QTabWidget::pane {
            border: 1px solid #dcdfe6;
            border-radius: 4px;
            background: white;
        }
        QTabBar::tab {
            background: #f5f7fa;
            border: 1px solid #dcdfe6;
            padding: 8px 15px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background: white;
            border-bottom-color: white;
        }
        QTabBar::tab:hover:!selected {
            background: #ebeef5;
        }
        QSlider::groove:horizontal {
            border: 1px solid #dcdfe6;
            height: 6px;
            background: #ebeef5;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #409eff;
            border: 1px solid #409eff;
            width: 18px;
            margin: -6px 0;
            border-radius: 9px;
        }
        QSlider::sub-page:horizontal {
            background: #409eff;
            border-radius: 3px;
        }
        QSpinBox {
            border: 1px solid #dcdfe6;
            border-radius: 4px;
            padding: 3px;
            background: white;
        }
        QComboBox {
            border: 1px solid #dcdfe6;
            border-radius: 4px;
            padding: 3px;
            background: white;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left-width: 1px;
            border-left-color: #dcdfe6;
            border-left-style: solid;
            border-top-right-radius: 4px;
            border-bottom-right-radius: 4px;
        }
        QComboBox::down-arrow {
            image: url(down_arrow.png);
            width: 10px;
            height: 10px;
        }
    )";

    this->setStyleSheet(styleSheet);
}

void MainWindow::onEnhancementFinished(const QPixmap &result) {
    if (!result.isNull()) {
        m_currentPixmap = result;
        updateImageDisplay();
        statusBar()->showMessage("图片增强完成", 3000);
    }
}
