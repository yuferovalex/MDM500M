#pragma once

#include <QVariantMap>

class Device;
class Module;
class DM500;
class DM500M;
class DM500FM;
class QIODevice;
class QXmlStreamWriter;
class QXmlStreamReader;

namespace Interfaces {

class SettingsSerializer
{
public:
    virtual ~SettingsSerializer() = default;
    virtual QString fileExtension() const = 0;
    virtual void serialize(QIODevice &out, Device &device) = 0;
    virtual bool deserialize(QIODevice &in, Device &device, QString &errors) = 0;
};

}

class XmlSerializer : public Interfaces::SettingsSerializer
{
public:
    QString fileExtension() const override;
    void serialize(QIODevice &out, Device &device) override;
    bool deserialize(QIODevice &in, Device &device, QString &errors) override;

private:
    void writeModuleData(QXmlStreamWriter &xml, const QVariantMap &data);
    void readModuleData(QXmlStreamReader &xml, QVariantMap &data);
};
