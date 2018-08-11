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

class XmlSerializer
{
public:
    void serialize(QIODevice &out, Device &device);
    bool deserialize(QIODevice &in, Device &device, QString &errors);

private:
    void writeModuleData(QXmlStreamWriter &xml, const QVariantMap &data);
    void readModuleData(QXmlStreamReader &xml, QVariantMap &data);
};
