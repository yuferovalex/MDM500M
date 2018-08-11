#pragma once

#include <memory>
#include <QString>
#include <QHash>

class QIODevice;

class NameRepository
{
public:
    NameRepository(QIODevice *device);
    ~NameRepository();
    QString getName(QString serialNumber);
    void setName(QString serialNumber, QString name);

private:
    void read();
    void save();

    bool m_hasChanges = false;
    int m_index = 1;
    const std::unique_ptr<QIODevice> m_device;
    QHash<QString, QString> m_dictionary;
};
