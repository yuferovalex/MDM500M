#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QHash>

#include "NameRepository.h"

NameRepository::NameRepository(QIODevice *device)
    : m_device(device)
{
    read();
}

NameRepository::~NameRepository()
{
    save();
}

QString NameRepository::getName(QString serialNumber)
{
    auto i = m_dictionary.find(serialNumber);
    if (i == m_dictionary.end()) {
        auto defaultName = QObject::tr("Новое устройство #%1").arg(m_index++);
        m_dictionary[serialNumber] = defaultName;
        m_hasChanges = true;
        return defaultName;
    }
    return *i;
}

void NameRepository::setName(QString serialNumber, QString name)
{
    auto i = m_dictionary.find(serialNumber);
    if (i != m_dictionary.end() && *i == name) {
        return ;
    }
    m_dictionary[serialNumber] = name;
    m_hasChanges = true;
}

void NameRepository::read()
{
    if (m_device == nullptr || !m_device->open(QIODevice::ReadOnly)) {
        return ;
    }
    QXmlStreamReader xml(m_device.get());
    while (!xml.atEnd()) {
        auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "device") {
                auto attributes = xml.attributes();
                auto serialNumber = attributes.value("serial").toString();
                auto name = xml.readElementText();
                if (!serialNumber.isEmpty() && !name.isEmpty()) {
                    m_dictionary[serialNumber] = name;
                }
            }
        }
    }
    m_device->close();
}

void NameRepository::save()
{
    if (m_device == nullptr || !m_hasChanges || !m_device->open(QIODevice::WriteOnly)) {
        return ;
    }
    m_hasChanges = false;

    QXmlStreamWriter xml(m_device.get());
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("devices");

    for (auto i = m_dictionary.begin(); i != m_dictionary.end(); ++i) {
        auto serial = i.key();
        auto name = i.value();

        xml.writeStartElement("device");
        xml.writeAttribute("serial", serial);
        xml.writeAttribute("type", "МДМ-500М"); // Обратная совместимость с версией 3.0
        xml.writeCharacters(name);
        xml.writeEndElement();
    }

    xml.writeEndElement();
    m_device->close();
}
