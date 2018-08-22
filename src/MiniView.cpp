#include <QButtonGroup>

#include "Modules.h"
#include "MiniView.h"
#include "ui_MiniView.h"

MiniView::MiniView(Device *device)
    : ui(new Ui::MiniView)
    , m_group(new QButtonGroup(this))
    , m_device(device)
{
    Q_ASSERT(device != nullptr);

    ui->setupUi(this);
    adjustSize();
    for (int slot = 0; slot < MDM500M::kSlotCount; ++slot) {
        auto ctrl  = findChild<QAbstractButton *>(QString("control_%1").arg(slot));
        m_group->addButton(ctrl, slot);
        onModuleReplaced(m_device->module(slot));
    }
    onErrorsChanged(m_device->isError());
    ui->deviceNameLbl->setText(m_device->name());
    connect(m_device, &Device::nameChanged, ui->deviceNameLbl, &QLabel::setText);
    connect(m_device, &Device::moduleReplaced, this, &MiniView::onModuleReplaced);
    connect(m_device, &Device::controlModuleChanged, this, &MiniView::onControlModuleChanged);
    connect(m_device, &Device::errorsChanged, this, &MiniView::onErrorsChanged);
    connect(m_group, SIGNAL(buttonClicked(int)), this, SLOT(onCurrentButtonChanged(int)));
}

void MiniView::onCurrentButtonChanged(int id)
{
    emit controlModuleChanged(id);
}

void MiniView::onModuleReplaced(Module *module)
{
    int slot = module->slot();
    auto ctrl  = findChild<QAbstractButton *>(QString("control_%1").arg(slot));
    auto scale = findChild<Scale  *>(QString("scale_%1").arg(slot));
    auto dbmv  = findChild<QLabel *>(QString("dbmv_%1").arg(slot));
    Q_ASSERT(scale != nullptr);
    Q_ASSERT(dbmv != nullptr);
    Q_ASSERT(ctrl != nullptr);

    bool isEmpty = module->isEmpty();
    bool isDbmv = module->isSupportSignalLevel();

    if (!isDbmv) {
        dbmv->setText("-");
    }
    ctrl->setDisabled(isEmpty);
    scale->setEmpty(isEmpty);
    connect(module, &Module::signalLevelChanged, [=](int scaleLevel, int signalLevel)
    {
        scale->setSignalLevel(scaleLevel);
        if (isDbmv) {
            dbmv->setText(QString::number(signalLevel));
        }
    });
}

void MiniView::onControlModuleChanged(int slot)
{
    m_group->button(slot)->setChecked(true);
}

void MiniView::onErrorsChanged(bool isError)
{
    ui->deviceStatus->setText(isError ? tr("Ошибка") : tr("Норма"));
    QString background = isError ? "#CC0000" : "#09A504";
    ui->deviceStatus->setStyleSheet("QLabel { background-color: " + background + "; }");
}
