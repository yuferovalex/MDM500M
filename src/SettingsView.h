#pragma once

#include <memory>

#include <QAbstractTableModel>
#include <QWidget>

#include "Device.h"

class EventLog;
class Firmware;
class ModuleView;
class NameRepository;
class SettingsView;
class TransactionInvoker;

namespace Interfaces {

class SettingsSerializer;
class TransactionFabric;

class ModuleViewFabric
{
public:
    virtual ~ModuleViewFabric() = default;
    virtual ModuleView *produce(Module *module, QWidget *parent = nullptr) = 0;
};

} // namespace Interfaces

namespace Ui {
class SettingsView;
}

struct SettingsViewBuilder
{
    std::shared_ptr<TransactionInvoker> invoker;
    std::shared_ptr<Interfaces::SettingsSerializer> settingsSerializer;
    std::shared_ptr<Interfaces::ModuleFabric> moduleFabric;
    std::shared_ptr<Interfaces::ModuleViewFabric> moduleViewFabric;
    std::shared_ptr<Interfaces::TransactionFabric> transactionFabric;
    std::shared_ptr<NameRepository> nameRepo;
    DeviceType type;

    SettingsView *build() const;
};

class SettingsView : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString type READ type CONSTANT)

public:
    SettingsView(const SettingsViewBuilder &builder);

    QString type() const;
    Device *device() const;
    void setControlModule(int slot);

signals:
    void disconnected();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void on_saveChangesBtn_clicked();
    void on_updateFirmwareBtn_clicked();
    void on_createBackupBtn_clicked();
    void on_restoreBackupBtn_clicked();
    void on_configTable_clicked(const QModelIndex &index);
    void on_name_editingFinished();

private:
    void initModel();
    void updateModel();
    void updateFirmware(const Firmware &firmware);
    void setThresholdLevels();
    void setModuleConfig(int slot);
    void setInterfaceEnabled(bool enabled);
    void updateMainInfo();

    Device m_device;
    std::shared_ptr<Interfaces::ModuleViewFabric> m_moduleViewFabric;
    std::shared_ptr<Interfaces::TransactionFabric> m_transactionFabric;
    std::shared_ptr<NameRepository> m_nameRepo;
    std::shared_ptr<TransactionInvoker> m_invoker;
    std::shared_ptr<Interfaces::SettingsSerializer> m_settingsSerializer;
    std::unique_ptr<Ui::SettingsView> ui;
    EventLog *m_log;
    QTimer *m_updateTimer;
};

class ConfigViewModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns { Type, Status, Frequency, Channel, Diagnostic, ScaleLevel, SignalLevel, ColumnsCount };

    ConfigViewModel(Device &device, QObject *parent);
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void showSignalLevelColumn(bool show);

private:
    void onModuleReplaced(Module *module);

    Device &m_device;
    bool m_showSignalLevelColumn = true;
};
