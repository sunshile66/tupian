QT += core gui network concurrent multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# 禁用过时API
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    imageprocessor.cpp \
    animationmanager.cpp

HEADERS += \
    mainwindow.h \
    imageprocessor.h \
    animationmanager.h

FORMS += \
    mainwindow.ui

# 添加资源文件
RESOURCES += \
    resources.qrc

# 发布配置
win32 {
    target.path = $$PWD/bin
    INSTALLS += target
}

unix:!android {
    target.path = /opt/$${TARGET}/bin
    INSTALLS += target
}
