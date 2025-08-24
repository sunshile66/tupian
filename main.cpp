#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("高级图片浏览器");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("YourCompany");
    app.setOrganizationDomain("yourcompany.com");

    MainWindow window;
    window.show();

    return app.exec();
}
