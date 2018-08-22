#pragma once

#include <memory>

#include <QWidget>

namespace Ui {
class MiniView;
}
class Device;
class Module;
class QButtonGroup;

class MiniView : public QWidget
{
    Q_OBJECT

public:
    MiniView(Device *device);

signals:
    void controlModuleChanged(int slot);

private slots:
    void onCurrentButtonChanged(int id);
    void onModuleReplaced(Module *module);
    void onControlModuleChanged(int slot);
    void onErrorsChanged(bool isError);

private:
    std::unique_ptr<Ui::MiniView> ui;
    QButtonGroup *m_group;
    Device *m_device;
};
