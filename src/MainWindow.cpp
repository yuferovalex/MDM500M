#include <QFile>
#include <QSettings>
#include <QWindowStateChangeEvent>
#include <QDesktopWidget>

#include "Device.h"
#include "Modules.h"
#include "MainWindow.h"
#include "MiniView.h"
#include "SettingsSerializers.h"
#include "ModuleViews.h"
#include "NameRepository.h"
#include "SettingsView.h"
#include "Transactions.h"
#include "TransactionInvoker.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow()
    : ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    ui->tabs->hide();
    ui->mainWindowEmptyLbl->show();
    ui->version->setText(QString("v%1").arg(QApplication::applicationVersion()));
    onCurrentTabChanged(-1);
    connect(ui->tabs, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);
    readSettings();
    createBuilders();
    searchDevice();
}

MainWindow::~MainWindow()
{
    clearTabs();
    writeSettings();
}

void MainWindow::createBuilders()
{
    SettingsViewBuilder builder;

    builder.type = DeviceType::MDM500M;
    builder.moduleFabric = std::make_shared<ModuleFabric>();
    builder.moduleViewFabric = std::make_shared<ModuleViewFabric>();
    builder.nameRepo = std::make_shared<NameRepository>(new QFile("devices.xml"));
    builder.settingsSerializer = std::make_shared<XmlSerializer>();
    builder.transactionFabric = std::make_shared<MDM500M::TransactionFabric>();
    m_builders[DeviceType::MDM500M] = builder;

    builder.type = DeviceType::MDM500;
    builder.settingsSerializer = std::make_shared<CsvSerializer>();
    builder.transactionFabric = std::make_shared<MDM500::TransactionFabric>();
    m_builders[DeviceType::MDM500] = builder;
}

void MainWindow::searchDevice()
{
    m_invoker = std::make_unique<TransactionInvoker>();
    auto transaction = new SearchDevice;
    connect(transaction, &SearchDevice::found, this, [=](auto type)
    {
        // Строитель такого типа зарегистрирован
        Q_ASSERT(m_builders.find(type) != m_builders.end());

        // Создание новой вкладки
        auto builder = m_builders[type];
        builder.invoker = std::move(m_invoker);
        auto settingsView = builder.build();
        auto miniView = new MiniView(settingsView->device());
        connect(miniView, &MiniView::controlModuleChanged,
                settingsView, &SettingsView::setControlModule);
        addTab(miniView, settingsView);

        // При отключении устройства удалить вкладку
        connect(settingsView, &SettingsView::disconnected, this, [=]
        {
            // Если приостанавливали поиск, потому что достигли ограничения, то
            // возобновляем его
            if (ui->tabs->count() == maxDeviceCount) {
                searchDevice();
            }
            removeTab(settingsView);
        });
        // Если вкладок меньше максимального значения, то продолжаем поиск
        if (ui->tabs->count() < maxDeviceCount) {
            searchDevice();
        }
    });
    m_invoker->exec(transaction);
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
    miniView->deleteLater();
    settingsView->deleteLater();
    ui->tabs->setVisible(ui->tabs->count() > 0);
    ui->mainWindowEmptyLbl->setVisible(ui->tabs->count() == 0);
}

void MainWindow::onCurrentTabChanged(int index)
{
    if (index == -1) {
        ui->title->setText(tr("Демодуляторы МДМ-500 и МДМ-500М"));
        return ;
    }
    QString type = ui->tabs->widget(index)->property("type").toString();
    ui->title->setText(tr("Демодулятор %1").arg(type));
}

void MainWindow::clearTabs()
{
    while (ui->tabs->count() > 0) {
        auto miniView = ui->tabs->tabBar()->tabButton(0, QTabBar::ButtonPosition::LeftSide);
        auto settingsView = ui->tabs->widget(0);
        ui->tabs->removeTab(0);
        miniView->deleteLater();
        settingsView->deleteLater();
    }
    ui->tabs->hide();
    ui->mainWindowEmptyLbl->show();
}

void MainWindow::readSettings()
{
    QSettings settings("settings.ini", QSettings::Format::IniFormat);
    auto geometry = settings.value("geometry");
    if (geometry.isValid()) {
        restoreGeometry(geometry.toByteArray());
    }
    else {
        setGeometry(
            QStyle::alignedRect(
                Qt::LeftToRight,
                Qt::AlignCenter,
                size(),
                qApp->desktop()->availableGeometry()
            )
        );
    }
}

void MainWindow::writeSettings()
{
    QSettings settings("settings.ini", QSettings::Format::IniFormat);
    settings.setValue("geometry", saveGeometry());
}
