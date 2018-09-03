#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QTextStream>

#include "Device.h"
#include "Modules.h"
#include "SettingsSerializers.h"

namespace {
struct Get : Interfaces::ModuleVisitor
{
    void visit(Module &) override;
    void visit(DM500 &) override;
    void visit(DM500M &) override;
    void visit(DM500FM &) override;
    void visit(EmptyModule &) override {}
    void visit(UnknownModule &) override {}

    QVariantMap data;
};

struct Set : Interfaces::ModuleVisitor
{
    void visit(Module &) override;
    void visit(DM500 &) override;
    void visit(DM500M &) override;
    void visit(DM500FM &) override;
    void visit(EmptyModule &) override;
    void visit(UnknownModule &) override;
    template <typename T, typename Setter>
    void setParam(QString key, int slot, Setter &&setter);

    QVariantMap data;
    QString errors;
    QTextStream stream { &errors };
};
}

QString XmlSerializer::fileExtension() const
{
    return QString("XML (*.xml)");
}

void XmlSerializer::serialize(QIODevice &out, Device &device)
{
    QXmlStreamWriter xml(&out);

    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("config");
    Get visitor;

    for (int slot = 0; slot < device.moduleCount(); ++slot) {
        Module &module = *device.module(slot);
        if (!module.isEmpty()) {
            xml.writeStartElement("module");
            xml.writeAttribute("id", QString::number(module.slot()));
            xml.writeAttribute("type", module.metaObject()->className());
            visitor.data.clear();
            device.module(slot)->accept(visitor);
            writeModuleData(xml, visitor.data);
            xml.writeEndElement();
        }
    }

    xml.writeEndElement();
}

void XmlSerializer::writeModuleData(QXmlStreamWriter &xml, const QVariantMap &data)
{
    for (auto param = data.begin(); param != data.end(); ++param) {
        xml.writeTextElement(param.key(), param.value().toString());
    }
}

void XmlSerializer::readModuleData(QXmlStreamReader &xml, QVariantMap &data)
{
    while (!xml.atEnd()) {
        auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            data[xml.name().toString().toLower()] = xml.readElementText();
        }
        if (token == QXmlStreamReader::EndElement && xml.name() == "module") {
            break ;
        }
    }
}

bool XmlSerializer::deserialize(QIODevice &in, Device &device, QString &errors)
{
    QXmlStreamReader xml(&in);
    Set visitor;
    std::array<QVariantMap, MDM500M::kSlotCount> data;
    bool isConfig = false;
    visitor.stream << QObject::tr("Отчет о восстановлении настроек:") << endl;
    // Чтение и разбор данных
    while (!xml.atEnd() && !xml.hasError()) {
        auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name().toString().toLower() == QLatin1String("config")) {
                isConfig = true;
            }
            if (xml.name().toString().toLower() == QLatin1String("module")) {
                auto attributes = xml.attributes();
                bool slotOk;
                int slot = attributes.value("id").toInt(&slotOk);
                QString type = attributes.value("type").toString();

                if (!slotOk || slot < 0 || slot > 15) {
                    visitor.stream << QObject::tr("Ошибка: неверный атрибут id (строка файла %1). "
                                    "Пропуск элемента.").arg(xml.lineNumber()) << endl;
                    xml.skipCurrentElement();
                    continue ;
                }

                data[slot]["type"] = type;
                readModuleData(xml, data[slot]);
            }
        }
    }
    if (xml.hasError()) {
        errors = QObject::tr("Ошибка разметки XML-файла: %1 (строка файла %2)")
                         .arg(xml.errorString())
                         .arg(xml.lineNumber());
        return false;
    }
    if (!isConfig) {
        errors = QObject::tr("Файл не является файлом резервной копии настроек");
        return false;
    }
    for (int slot = 0; slot < device.moduleCount(); ++slot) {
        visitor.data = data[slot];
        device.module(slot)->accept(visitor);
    }
    errors = visitor.errors;
    return true;
}

void Get::visit(Module &module)
{
    data["frequency"]      = QString::number(MegaHertzReal(module.frequency()).count(), 'f', 2);
    data["diagnostic"]     = module.isDiagnosticEnabled();
    data["thresholdLevel"] = QString::number(module.thresholdLevel());
}

void Get::visit(DM500 &module)
{
    visit(static_cast<Module &>(module));
}

void Get::visit(DM500M &module)
{
    visit(static_cast<Module &>(module));
    data["videoStandart"] = QVariant::fromValue(module.videoStandart());
    data["soundStandart"] = QVariant::fromValue(module.soundStandart());
}

void Get::visit(DM500FM &module)
{
    visit(static_cast<Module &>(module));
    data["volume"] = QString::number(module.volume());
}

template <typename T, typename Setter>
void Set::setParam(QString key, int slot, Setter &&setter)
{
    auto iter = data.find(key.toLower());
    if (iter == data.end()) {
        stream << QObject::tr("Модуль %1: параметр \"%2\" не найден.")
                             .arg(slot).arg(key)
              << endl;
        return ;
    }
    if (iter->canConvert<T>()) {
        setter(iter->value<T>());
    }
    else {
        stream << QObject::tr("Модуль %1: неверное значение параметра \"%2\".")
                             .arg(slot).arg(key)
               << endl;
    }
}

#define CHECK_TYPE() \
    do { \
        if (data["type"].toString() != module.metaObject()->className()) { \
            stream << QObject::tr("Модуль %1: типы модулей не совпадают") \
                      .arg(module.slot()) \
                   << endl; \
            return ; \
        } \
    } while (false)

#define SET_PARAM(type, key)                                \
    setParam<type>(#key, module.slot(), [&](auto value) {   \
        module.set ## key (value);                          \
    });

#define CHECKED_SET_PARAM(type, key, check)                 \
    setParam<type>(#key, module.slot(), [&](type value) {   \
        if (!(check)) {                                     \
            stream << QObject::tr("Модуль %1: неверное значение параметра \"%2\".") \
                         .arg(module.slot()).arg(#key)      \
            << endl;                                        \
            return ;                                        \
        }                                                   \
        module.set ## key (value);                          \
    });

#define CHECKED_SET_FREQUENCY(check)                                                \
    setParam<double>("Frequency", module.slot(), [&](double v) {                    \
        auto value = MegaHertzReal(v);                                              \
        if (!(check)) {                                                             \
            stream << QObject::tr("Модуль %1: неверное значение параметра \"%2\".") \
                         .arg(module.slot()).arg("Frequency")                       \
            << endl;                                                                \
            return ;                                                                \
        }                                                                           \
        module.setFrequency(value);                                                 \
    });

void Set::visit(Module &module)
{
    SET_PARAM(bool, Diagnostic);
    stream << QObject::tr("Модуль %1: восстановление завершено").arg(module.slot()) << endl;
}

void Set::visit(DM500 &module)
{
    CHECK_TYPE();
    CHECKED_SET_FREQUENCY(ModuleInfo<DM500>::validateFrequency(value));
    CHECKED_SET_PARAM(int, ThresholdLevel, ModuleInfo<DM500>::validateThresholdLevel(value));
    visit(static_cast<Module &>(module));
}

void Set::visit(DM500M &module)
{
    CHECK_TYPE();
    CHECKED_SET_FREQUENCY(ModuleInfo<DM500M>::validateFrequency(value));
    CHECKED_SET_PARAM(int, ThresholdLevel, ModuleInfo<DM500M>::validateThresholdLevel(value));
    SET_PARAM(DM500M::SoundStandart, SoundStandart);
    SET_PARAM(DM500M::VideoStandart, VideoStandart);
    visit(static_cast<Module &>(module));
}

void Set::visit(DM500FM &module)
{
    CHECK_TYPE();
    CHECKED_SET_FREQUENCY(ModuleInfo<DM500FM>::validateFrequency(value));
    CHECKED_SET_PARAM(int, ThresholdLevel, ModuleInfo<DM500FM>::validateThresholdLevel(value));
    CHECKED_SET_PARAM(int, Volume, ModuleInfo<DM500FM>::validateVolume(value));
    visit(static_cast<Module &>(module));
}

void Set::visit(EmptyModule &module)
{
    if (data.isEmpty()) return;
    CHECK_TYPE();
}

void Set::visit(UnknownModule &module)
{
    if (data.isEmpty()) return;
    CHECK_TYPE();
}

QString CsvSerializer::fileExtension() const
{
    return "CSV (*.csv)";
}

void CsvSerializer::serialize(QIODevice &out, Device &device)
{
    QTextStream text(&out);

    for (int slot = 0; slot < kSlotCount; ++slot) {
        auto &module = *device.module(slot);
        text << QString("%1;%2;\n")
                .arg(MegaHertzReal(module.frequency()).count(), 0, 'f', 2)
                .arg(module.isDiagnosticEnabled());
    }
    text << device.controlModule() << ";;" << endl;
}

bool CsvSerializer::deserialize(QIODevice &in, Device &device, QString &errors)
{
    Record data[kSlotCount];
    int control = 0;

    bool read = readModuleParams(in, errors, data)
             && readControlSlot(in, control, errors);
    if (!read) {
        return false;
    }
    QTextStream e(&errors);
    e << QObject::tr("Отчет о восстановлении настроек:") << endl;

    for (int slot = 0; slot < kSlotCount; ++slot) {
        auto &module = *device.module(slot);
        if (!module.isEmpty()) {
            if (!module.setFrequency(data[slot].frequency)) {
                e << QObject::tr("Модуль %1: неверное значение частоты").arg(slot) << endl;
            }
            module.setDiagnostic(data[slot].diagnostic);
            e << QObject::tr("Модуль %1: восстановление завершено").arg(slot) << endl;
        }
    }
    device.setControlModule(control);
    return true;
}

bool CsvSerializer::readModuleParams(QIODevice &in, QString &errors, Record *data)
{
    for (size_t lineNumber = 0; !in.atEnd() && lineNumber < kSlotCount; ++lineNumber) {
        bool ok;
        auto line = in.readLine().split(';');

        if (line.size() < 2) {
            errors = QObject::tr("нарушена разметка файла (строка %1)")
                    .arg(lineNumber + 1);
            return false;
        }

        auto frequency = MegaHertzReal(line[0].toDouble(&ok));
        if (!ok) {
            errors = QObject::tr("неверное значение частоты (1-ый столбец, %1 строка)")
                    .arg(lineNumber + 1);
            return false;
        }
        data[lineNumber].frequency = frequency;

        bool diagnostic = line[1].toInt(&ok);
        if (!ok) {
            errors = QObject::tr("неверное значение диагностики (2-ой столбец, %1 строка)")
                    .arg(lineNumber + 1);
            return false;
        }
        data[lineNumber].diagnostic = diagnostic;
    }
    return true;
}

bool CsvSerializer::readControlSlot(QIODevice &in, int &control, QString &errors)
{
    bool ok;
    auto line = in.readLine().split(';');

    if (line.size() < 1) {
        errors = QObject::tr("не найдено значение контрольного канала (строка 17)");
        return false;
    }

    control = line[0].toInt(&ok);
    if (!ok || control < 0 || control >= kSlotCount) {
        errors = QObject::tr("неверное значение контрольного канала (строка 17)");
        return false;
    }
    return true;
}
