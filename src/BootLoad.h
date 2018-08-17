#pragma once

#include <cstdint>
#include <vector>

#include "Types.h"

#pragma pack(push, 1)
// ---------------------------------------------------------------------------------------//
using BYTE = uint8_t;
using WORD = uint16_t;
typedef std::vector<BYTE> TvBytes;
typedef std::vector<WORD> TvWords;
// ---------------------------------------------------------------------------------------//

typedef enum{
    tbAcknowledg = 1,
    tbBootQuery  = 2,
    tbWritePage  = 3,
    tbError      = 4,
}ETypeBoot;

struct TBootPack {
    // пакет запроса прибора и подтверждения прибора
    unsigned char start_byte;          // всегда  0x55
    unsigned char type_boot;           // тип пакета boot_query=2 -запрос пакета  acknowledg=1 подтв записи, 3-запись, 4-фатальная ошибка
    SoftwareVersion version_soft;   // версия софта прибора
    HardwareVersion version_hard;   // версия железа прибора
    unsigned short int nmb_page:12;    // номер запрашиваемой страницы
    unsigned short int log_page_size:4;// порядок(2)размера страницы (8 для page_size=256, 6 для page_size=64)
    unsigned short int crc;            // контрольная сумма
};

class TPageHeader
{
public:
    unsigned short int  reserve_1;
    unsigned short int  open_key;
    unsigned short int  reserve_3;
    unsigned short int  reserve_4;
};

class TLoadPage
{
public:
    TPageHeader Header;
    TvBytes Data[64];
};

class TVarLoadPage
{
public:
    TPageHeader Header;
    TvWords Data;
};

typedef struct {
    //ответ компьютера
    unsigned char start_byte;           // всегда  0x55
    unsigned char type_boot;            // тип пакета load_page=2 -загрузка страницы
    unsigned short int nmb_page:12;     // номер запрашиваемой страницы
    unsigned short int log_page_size:4; // порядок(2)размера страницы (8 для page_size=256, 6 для page_size=64)
} TBootHeader;

typedef struct {
    //ответ компьютера
    unsigned char start_byte;            // всегда  0x55
    unsigned char type_boot;             // тип пакета load_page=2 -загрузка страницы
    unsigned short int nmb_page:12;      // номер запрашиваемой страницы
    unsigned short int log_page_size:4;  // порядок(2)размера страницы (8 для page_size=256, 6 для page_size=64)
    TLoadPage page;                      // загружаемая скремблированная страница
    unsigned short int crc;              // контрольная сумма
}TBootPage;

class TFileHeader
{
public:
    char                ID[10];
    unsigned short int  Compat;           // зарезервировано
    unsigned short int  NumPages;         // количество скремблированных страниц в файле
    unsigned short int  PageSize;         // размер скремблированной страницы sizeof(__scram_page)
    SoftwareVersion SoftwareVersion;  // версия софта в файле
    HardwareVersion HardwareVersion;  // версия железа для файла
    unsigned short int  CListLength;      // длина списка совместимости (количество совместимых)
    unsigned short int  SCListLength;     // длина списка совместимости (количество совместимых)
};
#pragma pack(pop)

uint16_t slow_crc16RAM(uint16_t sum, const void *p, uint16_t len);
