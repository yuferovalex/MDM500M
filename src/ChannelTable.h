#pragma once

#include <memory>

#include <QString>
#include <QStringList>

#include "Frequency.h"

namespace Interfaces {

class ChannelTable
{
public:
    virtual ~ChannelTable() = default;
    virtual QStringList allChannels() const = 0;
    virtual QString channel(KiloHertz khz) const = 0;
    virtual KiloHertz frequency(QString channel) const = 0;
};

} // namespace Interfaces

class WrongChannelName : public std::exception {};

class NullChannelTable : public Interfaces::ChannelTable
{
public:
    static std::shared_ptr<NullChannelTable> globalInstance();
    
    KiloHertz frequency(QString channel) const override;
    QString channel(KiloHertz frequency) const override;
    QStringList allChannels() const override;
};

class TvChannelTable : public Interfaces::ChannelTable
{
public:
    static std::shared_ptr<TvChannelTable> globalInstance();
    
    KiloHertz frequency(QString channel) const override;
    QString channel(KiloHertz frequency) const override;
    QStringList allChannels() const override;

private:
    static const KiloHertz m_frequencies[99];
    static const QStringList m_names;
};
