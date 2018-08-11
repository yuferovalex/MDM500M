#pragma once

#include <memory>

#include <QAbstractTableModel>
#include <QWidget>

#include "Device.h"

namespace Ui {
class SettingsView;
}
class Driver;
class SettingsView;
class ModuleView;
class NameRepository;
class EventLog;

class ModuleViewFabric
{
public:
    virtual ~ModuleViewFabric() = default;
    virtual ModuleView *produce(Module *module, QWidget *parent = nullptr) = 0;
};

class SettingsView : public QWidget
{
    Q_OBJECT

public:
    SettingsView(std::unique_ptr<Driver> &&driver,
                 std::shared_ptr<ModuleViewFabric> moduleViewFabric,
                 std::shared_ptr<NameRepository> nameRepo);

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
    void setThresholdLevels();
    void setModuleConfig(int slot);

    void setInterfaceEnabled(bool enabled);
    void updateMainInfo();

    Device m_device;
    std::shared_ptr<ModuleViewFabric> m_moduleViewFabric;
    std::shared_ptr<NameRepository> m_nameRepo;
    std::unique_ptr<Ui::SettingsView> ui;
    std::unique_ptr<Driver> m_driver;
    EventLog *m_log;
};

class ConfigViewModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns { Type, Status, Channel, ScaleLevel, SignalLevel, ColumnsCount };

    ConfigViewModel(Device &device, QObject *parent);
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    void onModuleReplaced(Module *module);

    Device &m_device;
};
