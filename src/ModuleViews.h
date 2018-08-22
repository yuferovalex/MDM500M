#pragma once

#include <memory>

#include <QWidget>

#include "SettingsView.h"

namespace Ui {
class DM500View;
class DM500MView;
class DM500FMView;
}
class QLabel;
class Module;
class DM500;
class DM500M;
class DM500FM;

class ModuleView : public QWidget
{
    Q_OBJECT

public:
    ModuleView(Module &module, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;

protected slots:
    void setup();
    void setupThresholdLevels();
    void onFrequencyEditingfinished();
    void onDiagnosticCurrentindexchanged(int value);
    void onChannelCurrentindexchanged(QString value);
    void onThresholdlevelCurrentindexchanged(int value);

signals:
    void back();
    void settingsChanged();
    void thresholdLevelChanged();

private:
    Module &m_module;
};

class DM500View : public ModuleView
{
    Q_OBJECT

public:
    DM500View(DM500 &module, QWidget *parent);

private:
    std::unique_ptr<Ui::DM500View> ui;
    DM500 &m_module;
};

class DM500MView : public ModuleView
{
    Q_OBJECT

public:
    DM500MView(DM500M &module, QWidget *parent);

private slots:
    void on_soundStandart_currentIndexChanged(int value);
    void on_videoStandart_currentIndexChanged(int value);

private:
    std::unique_ptr<Ui::DM500MView> ui;
    DM500M &m_module;
};

class DM500FMView : public ModuleView
{
    Q_OBJECT

public:
    DM500FMView(DM500FM &module, QWidget *parent);

private slots:
    void onVolumeEditingFinished();
    void onRdsChanged(bool state);
    void onStereoChanged(bool state);

private:
    void setColor(QLabel *label, bool on);

private:
    std::unique_ptr<Ui::DM500FMView> ui;
    DM500FM &m_module;
};

class ModuleViewFabric : public Interfaces::ModuleViewFabric
{
public:
    ModuleView *produce(Module *module, QWidget *parent = nullptr) override;
};
