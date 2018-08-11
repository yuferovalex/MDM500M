#pragma once

#include <memory>

#include <QWidget>

namespace Ui {
class MainWindow;
}
class Driver;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

private:
    static constexpr int maxDeviceCount = 4;

    void searchDevice();
    void addTab(QWidget *miniView, QWidget *settingsView);
    void removeTab(QWidget *settingsView);
    void clearTabs();

    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<Driver> m_driver;
};
