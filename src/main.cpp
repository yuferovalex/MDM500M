#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    qSetMessagePattern("[%{time}] %{type}: %{message}");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setApplicationVersion(VERSION_STR);
    app.setApplicationDisplayName(QObject::tr("Демодуляторы МДМ-500 и МДМ-500М"));
    app.setApplicationName(QObject::tr("Демодуляторы МДМ-500 и МДМ-500М"));
    MainWindow window;
    window.show();
    return app.exec();
}
