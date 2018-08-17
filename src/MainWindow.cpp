#include <QFile>

#include "Commands.h"
#include "Device.h"
#include "Driver.h"
#include "MainWindow.h"
#include "MiniView.h"
#include "SettingsView.h"
#include "ModuleViews.h"
#include "ui_MainWindow.h"
#include "NameRepository.h"

MainWindow::MainWindow()
    : ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    ui->tabs->hide();
    ui->mainWindowEmptyLbl->show();
    ui->version->setText(QString("v. %1").arg(QApplication::applicationVersion()));
    searchDevice();
}

MainWindow::~MainWindow()
{
    clearTabs();
}

void MainWindow::searchDevice()
{
    m_driver = std::make_unique<Driver>();
    auto cmd = new SearchDevice;
    connect(cmd, &SearchDevice::found, this, [=]
    {
        static auto moduleViewFabric = std::make_shared<ModuleViewFabricImpl>();
        static auto nameRepo = std::make_shared<NameRepository>(new QFile("devices.xml"));

        // Создание новой вкладки
        auto settingsView = new SettingsView(std::move(m_driver), moduleViewFabric, nameRepo);
        auto miniView = new MiniView(settingsView);
        addTab(miniView, settingsView);

        // При отключении устройства удалить вкладку
        connect(settingsView, &SettingsView::disconnected, this, [=]
        {
            if (ui->tabs->count() == maxDeviceCount) {
                searchDevice();
            }
            removeTab(settingsView);
        });

        if (ui->tabs->count() < maxDeviceCount) {
            searchDevice();
        }
    });
    m_driver->exec(cmd);
}

void MainWindow::addTab(QWidget *miniView, QWidget *settingsView)
{
    int index = ui->tabs->addTab(settingsView, QString());
    ui->tabs->tabBar()->setTabButton(index, QTabBar::ButtonPosition::LeftSide, miniView);
    ui->tabs->show();
    ui->mainWindowEmptyLbl->hide();
}

void MainWindow::removeTab(QWidget *settingsView)
{
    int index = ui->tabs->indexOf(settingsView);
    auto miniView = ui->tabs->tabBar()->tabButton(index, QTabBar::ButtonPosition::LeftSide);
    ui->tabs->removeTab(index);
    delete miniView;
    delete settingsView;
    ui->tabs->setVisible(ui->tabs->count() > 0);
    ui->mainWindowEmptyLbl->setVisible(ui->tabs->count() == 0);
}

void MainWindow::clearTabs()
{
    while (ui->tabs->count() > 0) {
        auto miniView = ui->tabs->tabBar()->tabButton(0, QTabBar::ButtonPosition::LeftSide);
        auto settingsView = ui->tabs->widget(0);
        ui->tabs->removeTab(0);
        delete miniView;
        delete settingsView;
    }
    ui->tabs->hide();
    ui->mainWindowEmptyLbl->show();
}
