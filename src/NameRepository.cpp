#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QHash>
#include <QTimer>

#include "NameRepository.h"

NameRepository::NameRepository(QIODevice *device)
    : m_timer(new QTimer(this))
    , m_device(device)
{
    Q_ASSERT(m_device != nullptr);

    m_device->setParent(this);
    m_timer->setInterval(m_saveTimeout);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &NameRepository::saveChanges);
    m_readFuture = std::async(&NameRepository::read, device);
}

NameRepository::~NameRepository()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    if (m_hasChanges) {
        save(m_device, m_dictionary);
    }
}

QString NameRepository::getName(QString type, QString serialNumber)
{
    initDictionary();
    auto i = m_dictionary.find(type);
    if (i == m_dictionary.end()) {
        return getDefaultName(type, serialNumber);
    }
    auto j = i->find(serialNumber);
    return j == i->end()
            ? getDefaultName(type, serialNumber)
            : *j;
}

void NameRepository::setName(QString type, QString serialNumber, QString newName)
{
    initDictionary();
    auto &currentName = m_dictionary[type][serialNumber];
    if (currentName == newName) {
        return ;
    }
    currentName = newName;
    m_hasChanges = true;
    m_timer->start();
}

NameRepository::Dictionary NameRepository::read(QIODevice *m_device)
{
    if (m_device == nullptr || !m_device->open(QIODevice::ReadOnly)) {
        return Dictionary {};
    }
    QXmlStreamReader xml(m_device);
    Dictionary m_dictionary;
    while (!xml.atEnd()) {
        auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "device") {
                auto attributes = xml.attributes();
                auto serialNumber = attributes.value("serial").toString();
                auto type = attributes.value("type").toString();
                auto name = xml.readElementText();
                if (!type.isEmpty() && !serialNumber.isEmpty() && !name.isEmpty()) {
                    m_dictionary[type][serialNumber] = name;
                }
            }
        }
    }
    m_device->close();
    return m_dictionary;
}

void NameRepository::save(QIODevice *m_device, const Dictionary &dictionary)
{
    if (m_device == nullptr || !m_device->open(QIODevice::WriteOnly)) {
        return ;
    }

    QXmlStreamWriter xml(m_device);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("devices");

    for (auto i = dictionary.begin(); i != dictionary.end(); ++i) {
        auto type = i.key();

        for (auto j = i->begin(); j != i->end(); ++j) {
            auto serial = j.key();
            auto name = j.value();

            xml.writeStartElement("device");
            xml.writeAttribute("type", type);
            xml.writeAttribute("serial", serial);
            xml.writeCharacters(name);
            xml.writeEndElement();
        }
    }

    xml.writeEndElement();
    m_device->close();
}

QString NameRepository::getDefaultName(QString type, QString serialNumber)
{
    auto defaultName = QObject::tr("Новое устройство #%1").arg(m_index++);
    m_dictionary[type][serialNumber] = defaultName;
    m_hasChanges = true;
    return defaultName;
}

void NameRepository::initDictionary()
{
    if (!m_isDictionaryInit) {
        m_dictionary = m_readFuture.get();
        m_isDictionaryInit = true;
    }
}

void NameRepository::saveChanges()
{
    m_saveFuture = std::async(&NameRepository::save, m_device, m_dictionary);
    m_hasChanges = false;
}
