#include <typeindex>
#include <unordered_map>

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTimer>

#include "ChannelTable.h"
#include "EventLog.h"
#include "Firmware.h"
#include "ModuleViews.h"
#include "Modules.h"
#include "NameRepository.h"
#include "SettingsView.h"
#include "Transactions.h"
#include "TransactionInvoker.h"
#include "SettingsSerializers.h"
#include "ui_SettingsView.h"

SettingsView::SettingsView(const SettingsViewBuilder &builder)
    : m_device(builder.moduleFabric, builder.type)
    , m_moduleViewFabric(builder.moduleViewFabric)
    , m_transactionFabric(builder.transactionFabric)
    , m_nameRepo(builder.nameRepo)
    , m_invoker(builder.invoker)
    , m_settingsSerializer(builder.settingsSerializer)
    , ui(std::make_unique<Ui::SettingsView>())
    , m_log(new EventLog(m_device, this))
    , m_updateTimer(new QTimer(this))
{
    m_updateTimer->setInterval(800);
    m_updateTimer->setSingleShot(true);
    connect(m_updateTimer, &QTimer::timeout, this, &SettingsView::updateModel);

    // UI setup
    ui->setupUi(this);
    setInterfaceEnabled(false);
    auto model = new ConfigViewModel(m_device, ui->configTable);
    ui->configTable->setModel(model);
    ui->configTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->configTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Настраиваем вид в зависимости от типа устройства
    bool show = builder.type == DeviceType::MDM500M;
    model->showSignalLevelColumn(show);
    ui->deviceSoftwareVersionLabel->setVisible(show);
    ui->softVerWrapper->setVisible(show);

    initModel();
}

QString SettingsView::type() const
{
    return m_device.type();
}

void SettingsView::mousePressEvent(QMouseEvent *event)
{
    // Очистка фокуса, если есть
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
    ui->serialNumber->setText(m_device.serialNumber());
    ui->softwareVersion->setText(m_device.softwareVersion().toString());
}

void SettingsView::onWrongParametersDetected()
{
    QMessageBox::critical(
                this,
                tr("Ошибка сохранения параметров на устройство"),
                tr("Во время передачи новых параметров, устройство сообщило, что "
                   "они не соответсвуют допустимым.\n"
                   "\n"
                   "За дополнительной информацией обратитесь к руководству "
                   "пользователя данной программы."));
}

void SettingsView::onDeviceCorruptionDetected()
{
    QMessageBox::critical(
                this,
                tr("Ошибка сохранения параметров на устройство"),
                tr("Во время передачи новых параметров, устройство сообщило, что "
                   "запись настроек невозможна из-за повреждения памяти.\n"
                   "\n"
                   "За дополнительной информацией обратитесь к руководству "
                   "пользователя данной программы."));
}

void SettingsView::initModel()
{
    using Interfaces::GetAllDeviceInfo;

    qDebug("начата инициализация модели");
    auto transaction = m_transactionFabric->getAllDeviceInfo();
    connect(transaction, &GetAllDeviceInfo::success, this, [=](auto &&response)
    {
        qDebug("инициализация модели завершена");

        // Заполняем модель данными
        m_device.setInfo(response.info);
        m_device.setName(m_nameRepo->getName(m_device.type(), m_device.serialNumber()));
        m_device.setConfig(response.config);
        m_device.setThresholdLevels(response.thresholdLevels);
        m_device.setSignalLevels(response.signalLevels);

        // Обновляем интерфейс пользователя
        m_log->initialMessage(response.log);
        setInterfaceEnabled(true);
        updateMainInfo();

        // Запускаем цикл обновления данных
        m_updateTimer->start();
    });
    connect(transaction, &GetAllDeviceInfo::failure, this, [=]
    {
        qDebug("произошло отключение во время инициализации модели");

        emit disconnected();
    });
    m_invoker->exec(transaction);
}

void SettingsView::updateModel()
{
    using Interfaces::UpdateDeviceInfo;

    qDebug("начато обновление модели");
    auto transaction = m_transactionFabric->updateDeviceInfo();
    connect(transaction, &UpdateDeviceInfo::success, this, [=](auto &&response)
    {
        qDebug("обновление завершено");
        // Обновляем модель
        m_device.setErrors(response.errors);
        m_device.setSignalLevels(response.signalLevels);
        m_device.setModuleStates(response.states);

        // Запускаем таймер по новой
        m_updateTimer->start();
    });
    connect(transaction, &UpdateDeviceInfo::failure, this, [=]
    {
        qDebug("произошло отключение во время обновления модели");
        emit disconnected();
    });
    m_invoker->exec(transaction);
}

void SettingsView::updateFirmware(const Firmware &firmware)
{
    using Interfaces::UpdateFirmware;

    auto transaction = m_transactionFabric->updateFirmware(firmware);
    auto dialog = new QProgressDialog(
                this,
                Qt::Window | Qt::WindowTitleHint); // Убераем кноки из заголовка

    // Настраиваем диалог
    dialog->setMinimum(0);
    dialog->setCancelButton(nullptr); // Запрещаем отмену
    dialog->setModal(true);
    dialog->setAutoClose(false);
    dialog->setAutoReset(false);
    dialog->setWindowTitle(tr("Прошивка"));

    connect(transaction, &UpdateFirmware::started, dialog, &QProgressDialog::show);
    connect(transaction, &UpdateFirmware::success, this, [=]
    {
        dialog->deleteLater();
        QMessageBox::information(this,
                                 tr("Обновление программного обеспечения"),
                                 tr("Программное обеспечение устройства "
                                    "успешно обновлено."));
        initModel();
    });
    connect(transaction, &UpdateFirmware::failure, this, [=]
    {
        dialog->deleteLater();
        initModel();
    });
    connect(transaction, &UpdateFirmware::error, this, [=](auto whileDoing)
    {
        QString text = tr("Произошла ошибка во время %1.\n\n%2");
        switch (whileDoing) {
        case UpdateFirmware::WaitForBoot:
            text = text.arg(tr("ожидания запуска прошивки"))
                       .arg(tr("Перезагрузите устройство вручную и "
                               "проверьте его работоспособность."));
            break;
        case UpdateFirmware::Reboot:
            text = text.arg(tr("попытки перезагрузить устройство"))
                       .arg(tr("За дополнительной информацией обратитесь к "
                               "руководству пользователя данной программы."));
            break;
        case UpdateFirmware::Flash:
            text = text.arg(tr("передачи прошивки на устройство"))
                       .arg(tr("За дополнительной информацией обратитесь к "
                               "руководству пользователя данной программы."));
            break;
        default:
            Q_ASSERT(false);
            return ;
        }
        QMessageBox::warning(this,
                             tr("Ошибка обновления программного обеспечения"),
                             text);
    });
    connect(transaction, &UpdateFirmware::statusChanged, dialog, [=](auto status)
    {
        QString text;
        switch (status) {
        case UpdateFirmware::WaitForBoot:
            text = tr("Ожидание запуска прошивки");
            break;
        case UpdateFirmware::Reboot:
            text = tr("Перезагрузка");
            break;
        case UpdateFirmware::Flash:
            text = tr("Передача прошивки на устройство");
            break;
        default:
            Q_ASSERT(false);
            return ;
        }
        dialog->setLabelText(text);
    });
    connect(transaction, &UpdateFirmware::progressChanged, dialog, &QProgressDialog::setValue);
    connect(transaction, &UpdateFirmware::progressMaxChanged, dialog, &QProgressDialog::setMaximum);

    // Останавливаем цикл обновлений модели
    if (m_updateTimer->isActive()) {
        m_updateTimer->stop();
    }
    m_invoker->exec(transaction);
}

void SettingsView::setControlModule(int slot)
{
    using Interfaces::SetControlModule;

    qDebug("запрошено переключение контрольного канала");
    auto transaction = m_transactionFabric->setControlModule(slot);
    m_device.setControlModule(slot);
    connect(transaction, &SetControlModule::success, this, [=]
    {
        qDebug("контрольный канал переключён");
    });
    connect(transaction, &SetControlModule::failure, this, [=]
    {
        qDebug("произошло отключение во время переключения контрольного канала");
        emit disconnected();
    });
    m_invoker->exec(transaction);
}

void SettingsView::setModuleConfig(int slot)
{
    using Interfaces::SetModuleConfig;

    qDebug("запрошено сохранение новых настроек модуля");
    auto config = m_device.data().config.modules[slot];
    auto transaction = m_transactionFabric->setModuleConfig(slot, config);
    connect(transaction, &SetModuleConfig::success, this, [=]
    {
        qDebug("параметры модуля применены");
    });
    connect(transaction, &SetModuleConfig::wrongParametersDetected, this, [=]
    {
        qDebug("устройство сообщило о неверных значениях параметров (1)");
        onWrongParametersDetected();
    });
    connect(transaction, &SetModuleConfig::failure, this, [=]
    {
        qDebug("произошло отключение во время применения параметров модуля");
        emit disconnected();
    });
    m_invoker->exec(transaction);
}

void SettingsView::setThresholdLevels()
{
    using Interfaces::SetThresholdLevels;

    qDebug("запрошено сохранение пороговых уровней");
    auto lvls = m_device.data().thresholdLevels;
    auto transaction = m_transactionFabric->setThresholdLevels(lvls);
    connect(transaction, &SetThresholdLevels::success, this, [=]
    {
        qDebug("пороговые уровни установлены");
    });
    connect(transaction, &SetThresholdLevels::failure, this, [=]
    {
        qDebug("произошло отключение во время применения пороговых уровней");
        emit disconnected();
    });
    m_invoker->exec(transaction);
}

void SettingsView::on_saveChangesBtn_clicked()
{
    using Interfaces::SaveConfigToEprom;

    qDebug("запрошено сохранение параметров в постоянную память");
    ui->saveChangesBtn->setEnabled(false);
    auto transaction = m_transactionFabric->saveConfigToEprom(m_device.data().config);
    connect(transaction, &SaveConfigToEprom::success, this, [=]
    {
        qDebug("параметры сохранены в постоянную память");
        ui->saveChangesBtn->setEnabled(true);
    });
    connect(transaction, &SaveConfigToEprom::wrongParametersDetected, this, [=]
    {
        qDebug("устройство сообщило о неверных значениях параметров (2)");
        onWrongParametersDetected();
        ui->saveChangesBtn->setEnabled(true);
    });
    connect(transaction, &SaveConfigToEprom::deviceCorruptionDetected, this, [=]
    {
        qDebug("устройство сообщило о повреждении памяти");
        onDeviceCorruptionDetected();
        ui->saveChangesBtn->setEnabled(true);
    });
    connect(transaction, &SaveConfigToEprom::failure, this, [=]
    {
        qDebug("произошло отключение во время сохранения параметров в постоянную память");
        emit disconnected();
    });
    m_invoker->exec(transaction);
}

void SettingsView::on_updateFirmwareBtn_clicked()
{
    // Начинаем диалог с пользователем
    auto filename = QFileDialog::getOpenFileName(
                this,
                tr("Выберите файл прошивки"),
                QDir::currentPath(),
                "BSK (*.bsk)");
    // Если нажал "Отмена" - выходим
    if (filename.isEmpty()) {
        return;
    }
    Firmware firmware(filename);
    // Проверяем файл на ошибки
    if (firmware.isError()) {
        QMessageBox::warning(
                    this,
                    tr("Ошибка"),
                    tr("Произошла ошибка при открытии файла прошивки: %1")
                    .arg(firmware.errorString()));
        return ;
    }
    // Проверяем прошивку на совместимость
    if (!firmware.isCompatible(MDM500M::kHardwareVersion)) {
        QMessageBox::warning(
                    this,
                    tr("Ошибка"),
                    tr("Прошивка в данном файле не предназначена для этого устройства."));
        return ;
    }
    // Если откат, то просим подтверждения
    if (firmware.softwareVersion() < m_device.softwareVersion()) {
        int answer = QMessageBox::question
        (
            this,
            QString(),
            tr("Вы пытаетесь понизить версию прошивки устройства.\n\n"
               "Продолжить?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (answer != QMessageBox::Yes) { return ; }
    }
    // Если перепрошивка, то просим подтверждения
    if (firmware.softwareVersion() == m_device.softwareVersion()) {
        int answer = QMessageBox::question
        (
            this,
            QString(),
            tr("Выбранная прошивка уже установлена на устройство.\n\n"
               "Продолжить?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (answer != QMessageBox::Yes) { return ; }
    }
    // Начинаем процедуру перепрошивки
    updateFirmware(firmware);
}

void SettingsView::on_createBackupBtn_clicked()
{
    // Начинаем диалог с пользователем
    auto filename = QFileDialog::getSaveFileName(
                this,
                tr("Выберите место для сохранения настроек"),
                QDir::currentPath(),
                m_settingsSerializer->fileExtension());
    // Если нажал "Отмена" - выходим
    if (filename.isEmpty()) {
        return;
    }
    // Открываем файл для записи, если не удалось, то выдаем предупреждение
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(
                    this,
                    tr("Ошибка создания резервной копии настроек"),
                    tr("Не удалось открыть файл для записи настроек: %1")
                    .arg(file.errorString()));
        return ;
    }
    // Сохраняем настройки
    m_settingsSerializer->serialize(file, m_device);
    QMessageBox::information(this, QString(), tr("Настройки успешно сохранены"));
}

void SettingsView::on_restoreBackupBtn_clicked()
{
    // Начинаем диалог с пользователем
    auto filename = QFileDialog::getOpenFileName(
                this,
                tr("Выберите файл настроек для восстановления"),
                QDir::currentPath(),
                m_settingsSerializer->fileExtension());
    // Если нажал "Отмена" - выходим
    if (filename.isEmpty()) {
        return;
    }
    // Открываем файл для чтения, если не удалось, то выдаем предупреждение
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(
                    this,
                    tr("Ошибка восстановления резервной копии настроек"),
                    tr("Не удалось открыть файл для чтения настроек: %1")
                    .arg(file.errorString()));
        return ;
    }
    // Читаем настройки
    QString errors;
    bool restored = m_settingsSerializer->deserialize(file, m_device, errors);
    // Если прочитать не удалось - выдаем предупреждение и выходим
    if (!restored) {
        QMessageBox::warning(
                    this,
                    tr("Ошибка восстановления резервной копии настроек"),
                    tr("Во время чтения файла настроек произошла ошибка: %1")
                    .arg(errors));
        return ;
    }
    // Сохраняем восстановленные настройки на прибор и выдаем отчет
    on_saveChangesBtn_clicked();
    QMessageBox::information(
                this,
                tr("Отчет о восстановлении"),
                errors);
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
    // Старое устройство не поддерживает временную передачу параметров%
    if (!m_device.isMDM500()) {
        connect(page, &ModuleView::thresholdLevelChanged, this, [=]
        {
            setThresholdLevels();
        });
        connect(page, &ModuleView::settingsChanged, this, [=]
        {
            setModuleConfig(slot);
        });
    }
    ui->stackedWidget->addWidget(page);
    ui->stackedWidget->setCurrentWidget(page);
}

void SettingsView::on_name_editingFinished()
{
    auto name = ui->name->text();
    m_device.setName(name);
    m_nameRepo->setName(m_device.type(), m_device.serialNumber(), name);
}

ConfigViewModel::ConfigViewModel(Device &device, QObject *parent)
    : QAbstractTableModel(parent)
    , m_device(device)
{
    connect(&m_device, &Device::moduleReplaced,
            this, &ConfigViewModel::onModuleReplaced);
}

int ConfigViewModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : MDM500M::kSlotCount;
}

int ConfigViewModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid()
            ? 0
            : m_showSignalLevelColumn
              ? ColumnsCount
              : ColumnsCount - 1;
}

QVariant ConfigViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (role == Qt::ToolTipRole) {
        role = Qt::DisplayRole;
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
    if (index.column() == Frequency && role == Qt::DisplayRole) {
        return QString::number(MegaHertzReal(module.frequency()).count(), 'f', 2);
    }
    if (index.column() == Channel && role == Qt::DisplayRole) {
        return module.channel().isEmpty()
             ? QString("-")
             : module.channel();
    }
    if (index.column() == Diagnostic && role == Qt::DisplayRole) {
        return module.isDiagnosticEnabled() ? tr("Вкл") : tr("Выкл");
    }
    if (index.column() == ScaleLevel || index.column() == SignalLevel) {
        switch (role) {
        case Qt::DisplayRole:
            return index.column() == ScaleLevel
                 ? module.scaleLevel().toString()
                 : module.isSupportSignalLevel()
                   ? QString::number(module.signalLevel())
                   : QString("-");
        case Qt::ForegroundRole:
            return QBrush("#3A6AB4");
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
            case Frequency   : return tr("Частота, МГц");
            case Channel     : return tr("Канал");
            case Diagnostic  : return tr("Диагностика");
            case ScaleLevel  : return tr("Оценка уровня\nсигнала");
            case SignalLevel : return tr("Уровень сигнала,\nдБмкВ");
            }
        }
    }
    return QVariant();
}

void ConfigViewModel::showSignalLevelColumn(bool show)
{
    if (show) {
        beginInsertColumns(QModelIndex(), SignalLevel, SignalLevel);
    }
    else {
        beginRemoveColumns(QModelIndex(), SignalLevel, SignalLevel);
    }
    m_showSignalLevelColumn = show;
    if (show) {
        endInsertColumns();
    }
    else {
        endRemoveColumns();
    }
}

void ConfigViewModel::onModuleReplaced(Module *module)
{
    int slot = module->slot();
    connect(module, &Module::frequencyChanged, this, [=]
    {
        emit dataChanged(index(slot, Channel), index(slot, Channel),
                         QVector<int>()
                         << Qt::DisplayRole
                         << Qt::ToolTipRole);
    });
    connect(module, &Module::channelChanged, this, [=]
    {
        emit dataChanged(index(slot, Channel), index(slot, Channel),
                         QVector<int>()
                         << Qt::DisplayRole
                         << Qt::ToolTipRole);
    });
    connect(module, &Module::signalLevelChanged, this, [=]
    {
        emit dataChanged(index(slot, ScaleLevel), index(slot, SignalLevel),
                         QVector<int>()
                         << Qt::DisplayRole
                         << Qt::ToolTipRole);
    });
    connect(module, &Module::errorsChanged, this, [=]
    {
        emit dataChanged(index(slot, Status), index(slot, Status),
                         QVector<int>()
                         << Qt::DisplayRole
                         << Qt::ToolTipRole
                         << Qt::BackgroundRole);
    });

    emit dataChanged(index(slot, 0), index(slot, ColumnsCount),
                     QVector<int>() << Qt::DisplayRole << Qt::ToolTipRole);
    emit dataChanged(index(slot, Status), index(slot, Status),
                     QVector<int>() << Qt::BackgroundRole);
}

SettingsView *SettingsViewBuilder::build() const
{
    return new SettingsView(*this);
}
