#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setApplicationDisplayName(QObject::tr("Демодулятор МДМ-500М"));
    app.setApplicationName(QObject::tr("Демодулятор МДМ-500М"));
    MainWindow window;
    window.show();
    return app.exec();
}
