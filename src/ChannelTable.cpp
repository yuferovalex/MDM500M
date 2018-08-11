#include <limits>

#include "ChannelTable.h"

std::shared_ptr<NullChannelTable> NullChannelTable::globalInstance()
{
    static auto instance = std::make_shared<NullChannelTable>();
    return instance;
}

KiloHertz NullChannelTable::frequency(QString) const
{
    return 0;
}

QString NullChannelTable::channel(KiloHertz) const
{
    return QString();
}

QStringList NullChannelTable::allChannels() const
{
    return QStringList();
}

std::shared_ptr<TvChannelTable> TvChannelTable::globalInstance()
{
    static auto instance = std::make_shared<TvChannelTable>();
    return instance;
}

KiloHertz TvChannelTable::frequency(QString channel) const
{
    int index = m_names.indexOf(channel);
    if (index == -1) {
        throw WrongChannelName();
    }
    return m_frequencies[index];
}

QString TvChannelTable::channel(KiloHertz frequency) const
{
    auto iterator = std::lower_bound(std::begin(m_frequencies), std::end(m_frequencies), frequency);
    int index = static_cast<int>(std::distance(std::begin(m_frequencies), iterator));
    auto firstDistance = KiloHertz::max();
    auto secondDistance = KiloHertz::max();
    if (iterator != std::begin(m_frequencies)) {
        firstDistance = frequency - *(iterator - 1);
    }
    if (iterator != std::end(m_frequencies)) {
        secondDistance = *iterator - frequency;
    }
    if (firstDistance < secondDistance) {
        return m_names[index - 1] + "+";
    }
    else if (secondDistance > KiloHertz::zero()) {
        return m_names[index] + "-";
    }
    else {
        return m_names[index];
    }
}

QStringList TvChannelTable::allChannels() const
{
    return m_names;
}

const KiloHertz TvChannelTable::m_frequencies[99] {
     49750,     59250,     77250,     85250,     93250,    111250,    119250,    127250,    135250,    143250,
    151250,    159250,    167250,    175250,    183250,    191250,    199250,    207250,    215250,    223250,
    231250,    239250,    247250,    255250,    263250,    271250,    279250,    287250,    295250,    303250,
    311250,    319250,    327250,    335250,    343250,    351250,    359250,    367250,    375250,    383250,
    391250,    399250,    407250,    415250,    423250,    431250,    439250,    447250,    455250,    463250,
    471250,    479250,    487250,    495250,    503250,    511250,    519250,    527250,    535250,    543250,
    551250,    559250,    567250,    575250,    583250,    591250,    599250,    607250,    615250,    623250,
    631250,    639250,    647250,    655250,    663250,    671250,    679250,    687250,    695250,    703250,
    711250,    719250,    727250,    735250,    743250,    751250,    759250,    767250,    775250,    783250,
    791250,    799250,    807250,    815250,    823250,    831250,    839250,    847250,    855250
};

const QStringList TvChannelTable::m_names {
    "1",    "2",    "3",    "4",     "5",    "S1",   "S2",   "S3",   "S4",    "S5",
    "S6",   "S7",   "S8",   "6",     "7",    "8",    "9",    "10",   "11",    "12",
    "S11",  "S12",  "S13",  "S14",   "S15",  "S16",  "S17",  "S18",  "S19",   "S20",
    "S21",  "S22",  "S23",  "S24",   "S25",  "S26",  "S27",  "S28",  "S29",   "S30",
    "S31",  "S32",  "S33",  "S34",   "S35",  "S36",  "S37",  "S38",  "S39",   "S40",
    "21",   "22",   "23",   "24",    "25",   "26",   "27",   "28",   "29",    "30",
    "31",   "32",   "33",   "34",    "35",   "36",   "37",   "38",   "39",    "40",
    "41",   "42",   "43",   "44",    "45",   "46",   "47",   "48",   "49",    "50",
    "51",   "52",   "53",   "54",    "55",   "56",   "57",   "58",   "59",    "60",
    "61",   "62",   "63",   "64",    "65",   "66",   "67",   "68",   "69"
};
