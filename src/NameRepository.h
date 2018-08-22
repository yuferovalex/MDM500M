#pragma once

#include <memory>
#include <future>

#include <QObject>
#include <QString>
#include <QHash>

class QIODevice;
class QTimer;

class NameRepository : public QObject
{
public:
    NameRepository(QIODevice *device);
    ~NameRepository();
    QString getName(QString type, QString serialNumber);
    void setName(QString type, QString serialNumber, QString name);

private:
    static constexpr std::chrono::milliseconds m_saveTimeout { 5000 };
    typedef QHash<QString, QHash<QString, QString>> Dictionary;
    static Dictionary read(QIODevice *device);
    static void save(QIODevice *device, const Dictionary &dictionary);
    QString getDefaultName(QString type, QString serialNumber);
    void initDictionary();
    void saveChanges();

    std::future<Dictionary> m_readFuture;
    std::future<void> m_saveFuture;
    QTimer *m_timer;
    QIODevice *m_device;
    Dictionary m_dictionary;
    int m_index = 1;
    bool m_isDictionaryInit = false;
    bool m_hasChanges = false;
};
