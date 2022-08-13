#-------------------------------------------------
#
# Project created by QtCreator 2022-07-12T04:25:17
#
#-------------------------------------------------

QT       += core gui
QT       += network
QT       += multimedia multimediawidgets multimedia-private multimediawidgets-private

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

TARGET = MeetingClient
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    Ui/aboutdialog.cpp \
    Ui/settingdialog.cpp \
    Widget/iperror.cpp \
    Ui/filterdialog.cpp \
    Ui/ipeditor.cpp \
    Class/Setting/settinginfo.cpp \
    Widget/localvideowidget.cpp \
    Widget/localaudiowidget.cpp \
    Class/Init/initinfo.cpp \
    Class/Control/mediacontrol.cpp \
    Class/Control/filtercontrol.cpp \
    Class/Control/iocontrol.cpp \
    Class/IO/audiodevice.cpp \
    Layout/localwidgetlayout.cpp \
    Widget/localmediawidget.cpp \
    Class/Adapter/filteradapter.cpp \
    Class/Adapter/meetingadapter.cpp \
    Class/Lib/meetingcore.cpp

HEADERS += \
        mainwindow.h \
    Ui/aboutdialog.h \
    Ui/settingdialog.h \
    Widget/iperror.h \
    Ui/filterdialog.h \
    Ui/ipeditor.h \
    Class/Setting/settinginfo.h \
    Widget/localvideowidget.h \
    Widget/localaudiowidget.h \
    Class/Init/initinfo.h \
    Class/Control/mediacontrol.h \
    Class/Control/filtercontrol.h \
    Class/Control/iocontrol.h \
    Class/IO/audiodevice.h \
    Layout/localwidgetlayout.h \
    Widget/localmediawidget.h \
    Class/Adapter/filteradapter.h \
    Class/Adapter/meetingadapter.h \
    Class/Lib/meetingcore.h

FORMS += \
        mainwindow.ui \
    Ui/aboutdialog.ui \
    Ui/settingdialog.ui \
    Ui/filterdialog.ui \
    Ui/ipeditor.ui

RESOURCES += \
    icons.qrc

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/release/ -lMeetingLib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/debug/ -lMeetingLib
else:unix: LIBS += -L$$PWD/lib/ -lMeetingLib

INCLUDEPATH += $$PWD/lib/
DEPENDPATH += $$PWD/lib/

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/release/libMeetingLib.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/debug/libMeetingLib.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/lib/release/MeetingLib.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/lib/debug/MeetingLib.lib
else:unix: PRE_TARGETDEPS += $$PWD/lib/libMeetingLib.a
