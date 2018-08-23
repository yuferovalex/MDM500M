#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>

#include "Modules.h"
#include "ModuleViews.h"
#include "ui_DM500View.h"
#include "ui_DM500MView.h"
#include "ui_DM500FMView.h"

ModuleView::ModuleView(Module &module, QWidget *parent)
    : QWidget (parent)
    , m_module(module)
{
}

void ModuleView::mousePressEvent(QMouseEvent *event)
{
    if (auto focused = focusWidget()) {
        focused->clearFocus();
    }
    QWidget::mousePressEvent(event);
}

void ModuleView::setup()
{
    setStyleSheet
    (
        "#scaleLevel, #moduleStatus, #signalLevel, #header, #moduleConfigHeader,"
        " #stereoStatus, #rdsStatus {\n"
        "	color: #3A6AB4;\n"
        "   font-weight: 900;\n"
        "   font-size: 12pt;\n"
        "}\n"
        "#header, #moduleConfigHeader {\n"
        "   font-size: 14pt;\n"
        "}"
    );

    // Заголовок
    auto header = findChild<QLabel *>("header");
    Q_ASSERT(header != nullptr);
    header->setText(tr("Модуль %1, %2").arg(m_module.slot()).arg(m_module.type()));

    // Оценка уровня сигнала и уровень сигнала
    auto scaleLevel = findChild<QLabel *>("scaleLevel");
    auto signalLevel = findChild<QLabel *>("signalLevel");
    Q_ASSERT(scaleLevel != nullptr);
    // v-- Если модуль поддерживает, то и надпись найдется --v
    Q_ASSERT(m_module.isSupportSignalLevel() == (signalLevel != nullptr));
    auto onSignalLevelChanged = [=]
    {
        scaleLevel->setText(m_module.scaleLevel().toString());
        if (signalLevel != nullptr) {
            signalLevel->setText(QString("%1 дБмкВ").arg(m_module.signalLevel()));
        }
    };
    onSignalLevelChanged();
    connect(&m_module, &Module::signalLevelChanged, this, onSignalLevelChanged);

    // Состояние
    auto moduleStatus = findChild<QLabel *>("moduleStatus");
    Q_ASSERT(moduleStatus != nullptr);
    auto onErrorsChanged = [=]
    {
        moduleStatus->setText(m_module.error().toString());
    };
    onErrorsChanged();
    connect(&m_module, &Module::errorsChanged, this, onErrorsChanged);

    // Кнопка "Назад"
    auto backBtn = findChild<QPushButton *>("backBtn");
    Q_ASSERT(backBtn != nullptr);
    connect(backBtn, &QPushButton::clicked, this, &ModuleView::back);

    // Частота
    auto frequency = findChild<QDoubleSpinBox *>("frequency");
    Q_ASSERT(frequency != nullptr);
    frequency->setValue(MegaHertzReal(m_module.frequency()).count());
    connect(frequency, &QDoubleSpinBox::editingFinished,
            this, &ModuleView::onFrequencyEditingfinished);
    connect(&m_module, &Module::frequencyChanged, this, [=]
    {
        frequency->setValue(MegaHertzReal(m_module.frequency()).count());
    });

    // Канал
    if (auto channel = findChild<QComboBox *>("channel")) {
        channel->addItems(m_module.allChannels());
        channel->setCurrentText(m_module.channel());
        channel->lineEdit()->setReadOnly(true);
        connect(channel, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
                this, &ModuleView::onChannelCurrentindexchanged);
        connect(&m_module, &Module::channelChanged, channel, &QComboBox::setCurrentText);
    }

    // Диагностика
    auto diagnostic = findChild<QComboBox *>("diagnostic");
    Q_ASSERT(diagnostic != nullptr);
    diagnostic->addItem(tr("Включена"), true);
    diagnostic->addItem(tr("Выключена"), false);
    int index = diagnostic->findData(m_module.isDiagnosticEnabled());
    Q_ASSERT(index != -1);
    diagnostic->setCurrentIndex(index);
    connect(diagnostic, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ModuleView::onDiagnosticCurrentindexchanged);

    // Пороговый уровень
    auto thresholdLevel = findChild<QComboBox *>("thresholdLevel");
    Q_ASSERT(thresholdLevel != nullptr);
    int lvl = m_module.thresholdLevel();
    index = thresholdLevel->findData(lvl);
    Q_ASSERT(index != -1);
    thresholdLevel->setCurrentIndex(index);
    connect(thresholdLevel, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ModuleView::onThresholdlevelCurrentindexchanged);
}

void ModuleView::setupThresholdLevels()
{
    auto thresholdLevel = findChild<QComboBox *>("thresholdLevel");
    Q_ASSERT(thresholdLevel != nullptr);
    for (int scaleLevel = 1; scaleLevel <= 9; ++scaleLevel) {
        thresholdLevel->addItem(tr("%1 / %2 дБмкВ")
                .arg(scaleLevel)
                .arg(m_module.convertScaleLevelToSignalLevel(scaleLevel)), scaleLevel);
    }
}

void ModuleView::onFrequencyEditingfinished()
{
    auto frequency = findChild<QDoubleSpinBox *>("frequency");
    if (!m_module.setFrequency(MegaHertzReal(frequency->value()))) {
        QMessageBox::warning(this, QString(), tr("Частота введена неверно"));
        frequency->setValue(frequency_cast<MegaHertzReal>(m_module.frequency()).count());
    }
    else {
        emit settingsChanged();
    }
}

void ModuleView::onDiagnosticCurrentindexchanged(int value)
{
    auto diagnostic = findChild<QComboBox *>("diagnostic");
    Q_ASSERT(diagnostic != nullptr);
    m_module.setDiagnostic(diagnostic->itemData(value).toBool());
    emit settingsChanged();
}

void ModuleView::onChannelCurrentindexchanged(QString value)
{
    m_module.setChannel(value);
    emit settingsChanged();
}

void ModuleView::onThresholdlevelCurrentindexchanged(int index)
{
    auto thresholdLevel = findChild<QComboBox *>("thresholdLevel");
    Q_ASSERT(thresholdLevel != nullptr);
    m_module.setThresholdLevel(thresholdLevel->itemData(index).toInt());
    emit thresholdLevelChanged();
}

DM500View::DM500View(DM500 &module, QWidget *parent)
    : ModuleView(module, parent)
    , ui(std::make_unique<Ui::DM500View>())
    , m_module(module)
{
    ui->setupUi(this);
    bool isThresholdLevelsSupported = module.isThresholdLevelsSupported();
    ui->thresholdLevel->setVisible(isThresholdLevelsSupported);
    ui->moduleThresholdLevelLabel->setVisible(isThresholdLevelsSupported);
    ui->frequency->setMinimum(MegaHertzReal(ModuleInfo<DM500>::minFrequency()).count());
    ui->frequency->setMaximum(MegaHertzReal(ModuleInfo<DM500>::maxFrequency()).count());
    for (int scaleLevel = 1; scaleLevel <= 9; scaleLevel += 2) {
        ui->thresholdLevel->addItem(QString::number(scaleLevel), scaleLevel);
    }
    ModuleView::setup();
}

DM500MView::DM500MView(DM500M &module, QWidget *parent)
    : ModuleView(module, parent)
    , ui(std::make_unique<Ui::DM500MView>())
    , m_module(module)
{
    ui->setupUi(this);
    ui->frequency->setMinimum(MegaHertzReal(ModuleInfo<DM500M>::minFrequency()).count());
    ui->frequency->setMaximum(MegaHertzReal(ModuleInfo<DM500M>::maxFrequency()).count());
    ModuleView::setupThresholdLevels();
    ModuleView::setup();
    ui->soundStandart->setCurrentIndex(m_module.soundStandart() == DM500M::NICAM ? 0 : 1);
    ui->videoStandart->setCurrentIndex(m_module.videoStandart() == DM500M::DK    ? 0 : 1);
}

void DM500MView::on_soundStandart_currentIndexChanged(int value)
{
    m_module.setSoundStandart(value == 0 ? DM500M::NICAM : DM500M::A2);
    emit settingsChanged();
}

void DM500MView::on_videoStandart_currentIndexChanged(int value)
{
    m_module.setVideoStandart(value == 0 ? DM500M::DK : DM500M::BG);
    emit settingsChanged();
}

DM500FMView::DM500FMView(DM500FM &module, QWidget *parent)
    : ModuleView(module, parent)
    , ui(std::make_unique<Ui::DM500FMView>())
    , m_module(module)
{
    ui->setupUi(this);
    ui->frequency->setMinimum(MegaHertzReal(ModuleInfo<DM500FM>::minFrequency()).count());
    ui->frequency->setMaximum(MegaHertzReal(ModuleInfo<DM500FM>::maxFrequency()).count());
    ModuleView::setupThresholdLevels();
    ModuleView::setup();
    // Volume
    ui->volume->setMinimum(ModuleInfo<DM500FM>::minVolume());
    ui->volume->setMaximum(ModuleInfo<DM500FM>::maxVolume());
    ui->volume->setValue(m_module.volume());
    connect(ui->volume, &QSpinBox::editingFinished, this, &DM500FMView::onVolumeEditingFinished);
    // RDS
    onRdsChanged(m_module.isRds());
    connect(&m_module, &DM500FM::rdsChanged, this, &DM500FMView::onRdsChanged);
    // Stereo
    onStereoChanged(m_module.isStereo());
    connect(&m_module, &DM500FM::stereoChanged, this, &DM500FMView::onStereoChanged);
}

void DM500FMView::onVolumeEditingFinished()
{
    m_module.setVolume(ui->volume->value());
    emit settingsChanged();
}

void DM500FMView::onRdsChanged(bool on)
{
    QString text = on
            ? tr("Есть")
            : tr("Нет");
    ui->rdsStatus->setText(text);
}

void DM500FMView::onStereoChanged(bool on)
{
    QString text = on
            ? tr("Стерео")
            : tr("Моно");
    ui->stereoStatus->setText(text);
}

ModuleView *ModuleViewFabric::produce(Module *module, QWidget *parent)
{
    if (auto m = dynamic_cast<DM500 *>(module)) {
        return new DM500View(*m, parent);
    }
    if (auto m = dynamic_cast<DM500M *>(module)) {
        return new DM500MView(*m, parent);
    }
    if (auto m = dynamic_cast<DM500FM *>(module)) {
        return new DM500FMView(*m, parent);
    }
    Q_ASSERT(false);
    return nullptr;
}
