QT += gui widgets serialport

CONFIG += c++14

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

VER_MAJ=3
VER_MIN=1
VER_PAT=1
VERSION = $${VER_MAJ}.$${VER_MIN}.$${VER_PAT}
DEFINES += VER_MAJ=$$VER_MAJ
DEFINES += VER_MIN=$$VER_MIN
DEFINES += VER_PAT=$$VER_PAT
DEFINES += VERSION_STR=\\\"$$VERSION\\\"
TARGET = MDM500M_$$VERSION

RC_FILE = app_resource.rc

!contains(QMAKE_TARGET.arch, x86_64) {
    # WinXP32
    DEFINES += _ATL_XP_TARGETING
    DEFINES += PSAPI_VERSION=1
    QMAKE_LFLAGS_WINDOWS = /SUBSYSTEM:WINDOWS,5.01
    #end WinXP32
    CONFIG(release, debug|release) {
        DEFINES += QT_NO_DEBUG_OUTPUT
        DEFINES += QT_NO_INFO_OUTPUT
        DEFINES += QT_NO_WARNING_OUTPUT
        DESTDIR = $$PWD/../bin
    }
}

OTHER_FILES = app_resource.rc

HEADERS += \
    Types.h \
    Commands.h \
    Driver.h \
    Cancelation.h \
    Protocol.h \
    Device.h \
    Modules.h \
    MainWindow.h \
    SettingsView.h \
    MiniView.h \
    Scale.h \
    ChannelTable.h \
    Frequency.h \
    ModuleViews.h \
    NameRepository.h \
    XmlSerializer.h \
    EventLog.h \
    BootLoad.h \
    UpdaterProtocol.h \
    Firmware.h

SOURCES += \
    main.cpp \
    Commands.cpp \
    Driver.cpp \
    Cancelation.cpp \
    Protocol.cpp \
    Device.cpp \
    Modules.cpp \
    MainWindow.cpp \
    SettingsView.cpp \
    MiniView.cpp \
    Scale.cpp \
    ChannelTable.cpp \
    ModuleViews.cpp \
    NameRepository.cpp \
    XmlSerializer.cpp \
    EventLog.cpp \
    UpdaterProtocol.cpp \
    Firmware.cpp

FORMS += \
    MainWindow.ui \
    SettingsView.ui \
    MiniView.ui \
    DM500View.ui \
    DM500FMView.ui \
    DM500MView.ui

RESOURCES += \
    resources.qrc
