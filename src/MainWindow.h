#pragma once

#include <memory>
#include <unordered_map>

#include <QWidget>

#include "SettingsView.h"

namespace Ui {
class MainWindow;
}
class TransactionInvoker;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

private:
    static constexpr int maxDeviceCount = 4;

    void createBuilders();
    void searchDevice();
    void addTab(QWidget *miniView, QWidget *settingsView);
    void removeTab(QWidget *settingsView);
    void onCurrentTabChanged(int index);
    void clearTabs();

    std::unordered_map<DeviceType, SettingsViewBuilder> m_builders;
    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<TransactionInvoker> m_invoker;
};
