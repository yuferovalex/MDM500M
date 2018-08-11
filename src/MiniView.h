#pragma once

#include <memory>

#include <QWidget>

namespace Ui {
class MiniView;
}
class Device;
class Module;
class SettingsView;
class QButtonGroup;

class MiniView : public QWidget
{
    Q_OBJECT

public:
    MiniView(SettingsView *settingsView);
    ~MiniView();

private slots:
    void onCurrentButtonChanged(int id);
    void onModuleReplaced(Module *module);
    void onControlModuleChanged(int slot);
    void onErrorsChanged(bool isError);

private:
    std::unique_ptr<Ui::MiniView> ui;
    SettingsView *m_settingsView;
    QButtonGroup *m_group;
    Device *m_device;
};
