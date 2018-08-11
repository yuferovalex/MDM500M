QT += gui widgets serialport

TARGET = app

CONFIG += c++14
#CONFIG += console
#CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

VERSION = 3.1.0

win32 {
    QMAKE_TARGET_COMPANY = ������
    QMAKE_TARGET_DESCRIPTION = ��������� ��� ��������� ��������� ���-500�
    QMAKE_TARGET_COPYRIGHT = (c) 2018 $$QMAKE_TARGET_COMPANY
    QMAKE_TARGET_PRODUCT = ���-500(�) $$VERSION
    # Russian (Ru)
    RC_LANG = 0x0419
    RC_ICONS = $$PWD/img/icon.ico

    # WinXP32
    DEFINES += _ATL_XP_TARGETING
    DEFINES += PSAPI_VERSION=1
    QMAKE_LFLAGS_WINDOWS = /SUBSYSTEM:WINDOWS,5.01
    #end WinXP32
}

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
    EventLog.h

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
    EventLog.cpp

FORMS += \
    MainWindow.ui \
    SettingsView.ui \
    MiniView.ui \
    DM500View.ui \
    DM500FMView.ui \
    DM500MView.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
