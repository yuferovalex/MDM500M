#include <typeindex>
#include <unordered_map>

#include <QFile>
#include <QFileDialog>
#include <QTimer>

#include "Commands.h"
#include "ChannelTable.h"
#include "Driver.h"
#include "EventLog.h"
#include "ModuleViews.h"
#include "Modules.h"
#include "NameRepository.h"
#include "SettingsView.h"
#include "ui_SettingsView.h"
#include "XmlSerializer.h"

SettingsView::SettingsView(std::unique_ptr<Driver> &&driver,
                           std::shared_ptr<ModuleViewFabric> moduleViewFabric,
                           std::shared_ptr<NameRepository> nameRepo)
    : m_device(new ModuleFabricImpl())
    , m_moduleViewFabric(moduleViewFabric)
    , m_nameRepo(nameRepo)
    , ui(std::make_unique<Ui::SettingsView>())
    , m_driver(std::move(driver))
    , m_log(new EventLog(m_device, this))
{
    // UI setup
    ui->setupUi(this);
    setInterfaceEnabled(false);
    ui->configTable->setModel(new ConfigViewModel(m_device, ui->configTable));
    ui->configTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->configTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    initModel();
}

void SettingsView::mousePressEvent(QMouseEvent *event)
{
    if (auto focused = focusWidget()) {
        focused->clearFocus();
    }
    QWidget::mousePressEvent(event);
}

Device *SettingsView::device() const
{
    return const_cast<Device *>(&m_device);
}

void SettingsView::setInterfaceEnabled(bool enabled)
{
    ui->configTable->setEnabled(enabled);
    ui->name->setEnabled(enabled);
    ui->createBackupBtn->setEnabled(enabled);
    ui->restoreBackupBtn->setEnabled(enabled);
    ui->saveChangesBtn->setEnabled(enabled);
    ui->updateFirmwareBtn->setEnabled(enabled);
    ui->serialNumber->setEnabled(enabled);
    ui->softwareVersion->setEnabled(enabled);
}

void SettingsView::updateMainInfo()
{
    ui->name->setText(m_device.name());
    ui->serialNumber->setText(m_device.serialNumber().toString());
    ui->softwareVersion->setText(m_device.softwareVersion().toString());
}

void SettingsView::initModel()
{
    qDebug("начата инициализация модели");
    auto cmd = new GetAllDeviceInfo;
    connect(cmd, &GetAllDeviceInfo::success, this, [=](auto &&response)
    {
        qDebug("инициализация модели завершена");

        // Fill model
        m_device.setInfo(response.info);
        m_device.setName(m_nameRepo->getName(m_device.serialNumber().toString()));
        m_device.setConfig(response.config);
        m_device.setThresholdLevels(response.thresholdLevels);
        m_device.setSignalLevels(response.signalLevels);

        // Update GUI
        m_log->initialMessage(response.log);
        setInterfaceEnabled(true);
        updateMainInfo();
        // Start update loop
        QTimer::singleShot(800, this, &SettingsView::updateModel);
    });
    connect(cmd, &GetAllDeviceInfo::failure, this, [=]
    {
        qDebug("произошло отключение во время инициализации модели");
        emit disconnected();
    });
    m_driver->exec(cmd);
}

void SettingsView::updateModel()
{
    qDebug("начато обновление модели");
    auto cmd = new UpdateDeviceInfo;
    connect(cmd, &UpdateDeviceInfo::success, this, [=](auto &&response)
    {
        qDebug("обновление завершено");
        m_device.setErrors(response.errors);
        m_device.setSignalLevels(response.signalLevels);
        m_device.setModuleStates(response.states);
        QTimer::singleShot(800, this, &SettingsView::updateModel);
    });
    connect(cmd, &UpdateDeviceInfo::failure, this, [=]
    {
        qDebug("произошло отключение во время обновления модели");
        emit disconnected();
    });
    m_driver->exec(cmd);
}

void SettingsView::setControlModule(int slot)
{
    qDebug("запрошено переключение контрольного канала");
    auto cmd = new SetControlModule(slot);
    m_device.setControlModule(slot);
    connect(cmd, &SetControlModule::success, this, [=]
    {
        qDebug("контрольный канал переключён");
    });
    connect(cmd, &SetControlModule::failure, this, [=]
    {
        qDebug("произошло отключение во время переключения контрольного канала");
        emit disconnected();
    });
    m_driver->exec(cmd);
}

void SettingsView::setModuleConfig(int slot)
{
    qDebug("запрошено сохранение новых настроек модуля");
    auto config = m_device.data().config.modules[slot];
    auto cmd = new SetModuleConfig(slot, config);
    connect(cmd, &SetModuleConfig::success, this, [=]
    {
        qDebug("параметры модуля применены");
    });
    connect(cmd, &SetModuleConfig::wrongParametersDetected, this, []
    {
        qDebug("устройство сообщило о неверных значениях параметров (1)");
        // TODO: ShowMessage("blah-blah-blah");
    });
    connect(cmd, &SetModuleConfig::failure, this, [=]
    {
        qDebug("произошло отключение во время применения параметров модуля");
        emit disconnected();
    });
    m_driver->exec(cmd);
}

void SettingsView::setThresholdLevels()
{
    qDebug("запрошено сохранение пороговых уровней");
    auto lvls = m_device.data().thresholdLevels;
    auto cmd = new SetThresholdLevels(lvls);
    connect(cmd, &SetThresholdLevels::success, this, [=]
    {
        qDebug("пороговые уровни установлены");
    });
    connect(cmd, &SetThresholdLevels::failure, this, [=]
    {
        qDebug("произошло отключение во время применения пороговых уровней");
        emit disconnected();
    });
    m_driver->exec(cmd);
}

void SettingsView::on_saveChangesBtn_clicked()
{
    qDebug("запрошено сохранение параметров в постоянную память");
    ui->saveChangesBtn->setEnabled(false);
    auto cmd = new SaveConfigToEprom(m_device.data().config);
    connect(cmd, &SaveConfigToEprom::success, this, [=]
    {
        qDebug("параметры сохранены в постоянную память");
        ui->saveChangesBtn->setEnabled(true);
    });
    connect(cmd, &SaveConfigToEprom::wrongParametersDetected, this, [=]
    {
        qDebug("устройство сообщило о неверных значениях параметров (2)");
        // TODO: ShowMessage(blah-blah-blah);
        ui->saveChangesBtn->setEnabled(true);
    });
    connect(cmd, &SaveConfigToEprom::deviceCorruptionDetected, this, [=]
    {
        qDebug("устройство сообщило о повреждении памяти");
        // TODO: ShowMessage(blah-blah-blah);
        ui->saveChangesBtn->setEnabled(true);
    });
    connect(cmd, &SaveConfigToEprom::failure, this, [=]
    {
        qDebug("произошло отключение во время сохранения параметров в постоянную память");
        emit disconnected();
    });
    m_driver->exec(cmd);
}

#include <QMessageBox>
#define UNRELEASED() \
    do { \
        QMessageBox::warning(this, QString(), "Эта фукция не реализована"); \
        return ; \
    } while(false)

void SettingsView::on_updateFirmwareBtn_clicked()
{
    UNRELEASED();
}

void SettingsView::on_createBackupBtn_clicked()
{
    auto filename = QFileDialog::getSaveFileName(this, QString(), QString(), "XML (*.xml)");
    if (filename.isEmpty()) return;
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, QString(), tr("Не удалось открыть файл для записи: %1")
                             .arg(file.errorString()));
        return ;
    }

    XmlSerializer serializer;
    serializer.serialize(file, m_device);
    QMessageBox::information(this, QString(), tr("Настройки успешно сохранены"));
}

void SettingsView::on_restoreBackupBtn_clicked()
{
    auto filename = QFileDialog::getOpenFileName(this, QString(), QString(), "XML (*.xml)");
    if (filename.isEmpty()) return;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QString(), tr("Не удалось открыть файл для чтения: %1")
                             .arg(file.errorString()));
        return ;
    }

    XmlSerializer serializer;
    QString errors;
    if (!serializer.deserialize(file, m_device, errors)) {
        QMessageBox::warning(this, QString(), errors);
        return;
    }
    on_saveChangesBtn_clicked();
    QMessageBox::information(this, QString(), errors);
}

void SettingsView::on_configTable_clicked(const QModelIndex &index)
{
    ui->configTable->clearSelection();
    int slot = index.row();
    auto module = m_device.module(slot);
    if (module->isEmpty()) {
        return ;
    }
    auto page = m_moduleViewFabric->produce(module);
    connect(page, &ModuleView::back, this, [=]
    {
        ui->stackedWidget->setCurrentWidget(ui->mainPage);
        page->deleteLater();
    });
    connect(page, &ModuleView::thresholdLevelChanged, this, [=]
    {
        setThresholdLevels();
    });
    connect(page, &ModuleView::settingsChanged, this, [=]
    {
        setModuleConfig(slot);
    });
    ui->stackedWidget->addWidget(page);
    ui->stackedWidget->setCurrentWidget(page);
}

void SettingsView::on_name_editingFinished()
{
    auto name = ui->name->text();
    m_device.setName(name);
    m_nameRepo->setName(m_device.serialNumber().toString(), name);
}

ConfigViewModel::ConfigViewModel(Device &device, QObject *parent)
    : QAbstractTableModel(parent)
    , m_device(device)
{
    connect(&m_device, &Device::moduleReplaced, this, &ConfigViewModel::onModuleReplaced);
}

int ConfigViewModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : kMDM500MSlotCount;
}

int ConfigViewModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnsCount;
}

QVariant ConfigViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter;
    }
    auto &module = *m_device.module(index.row());
    if (index.column() == Type && role == Qt::DisplayRole) {
        return module.type();
    }
    if (index.column() == Status) {
        switch (role) {
        case Qt::DisplayRole:
            return module.error().toString();
        case Qt::ForegroundRole:
            return QBrush(Qt::white);
        case Qt::FontRole: {
            QFont font;
            font.setBold(true);
            return font;
        }
        case Qt::BackgroundRole:
            return module.isError() ? QBrush("#CC0000") : QBrush("#09A504");
        default:
            break;
        }
    }
    if (module.isEmpty()) {
        return QVariant();
    }
    if (index.column() == Channel && role == Qt::DisplayRole) {
        auto c = module.channel();
        auto f = QString::number(frequency_cast<MegaHertzReal>(module.frequency()).count(), 'f', 2);
        return c.isEmpty()
             ? QString("%1 МГц").arg(f)
             : QString("CH %1 (%2 МГц)").arg(c).arg(f);
    }
    if (index.column() == ScaleLevel || index.column() == SignalLevel) {
        switch (role) {
        case Qt::DisplayRole:
            return index.column() == ScaleLevel
                 ? module.scaleLevel().toString()
                 : module.isSupportSignalLevel() ? QString::number(module.signalLevel())
                                                 : QString("-");
        case Qt::ForegroundRole:
            return QBrush(Qt::blue);
        case Qt::FontRole: {
            QFont font;
            font.setBold(true);
            return font;
        }
        default:
            break;
        }
    }
    return QVariant();
}

QVariant ConfigViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        if (role == Qt::TextAlignmentRole) {
            return Qt::AlignCenter;
        }
        if (role == Qt::DisplayRole) {
            if (section > 9) {
                return QString("%1/%2").arg(section).arg(section, 1, 16).toUpper();
            }
            return QString::number(section);
        }
    }
    else {
        if (role == Qt::TextAlignmentRole) {
            return Qt::AlignCenter;
        }
        if (role == Qt::DisplayRole) {
            switch (section) {
            case Type        : return tr("Тип");
            case Status      : return tr("Состояние");
            case Channel     : return tr("Канал");
            case ScaleLevel  : return tr("Оценка уровня\nсигнала");
            case SignalLevel : return tr("Уровень сигнала,\nдБмкВ");
            }
        }
    }
    return QVariant();
}

void ConfigViewModel::onModuleReplaced(Module *module)
{
    int slot = module->slot();
    connect(module, &Module::frequencyChanged, this, [=]
    {
        emit dataChanged(index(slot, Channel), index(slot, Channel),
                         QVector<int>() << Qt::DisplayRole);
    });
    connect(module, &Module::channelChanged, this, [=]
    {
        emit dataChanged(index(slot, Channel), index(slot, Channel),
                         QVector<int>() << Qt::DisplayRole);
    });
    connect(module, &Module::signalLevelChanged, this, [=]
    {
        emit dataChanged(index(slot, ScaleLevel), index(slot, SignalLevel),
                         QVector<int>() << Qt::DisplayRole);
    });
    connect(module, &Module::errorsChanged, this, [=]
    {
        emit dataChanged(index(slot, Status), index(slot, Status),
                         QVector<int>() << Qt::DisplayRole << Qt::BackgroundRole);
    });

    emit dataChanged(index(slot, 0), index(slot, ColumnsCount), QVector<int>() << Qt::DisplayRole);
    emit dataChanged(index(slot, Status), index(slot, Status), QVector<int>() << Qt::BackgroundRole);
}
