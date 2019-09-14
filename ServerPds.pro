QT += core
QT += network
QT += sql
QT += widgets
QT += charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ServerPds
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
    callout.cpp \
        main.cpp \
        mainwindow.cpp \
    myserver.cpp \
    packet.cpp \
    esp.cpp \
    person.cpp \
    dbthread.cpp \
    elaboratethread.cpp \
    listenerthread.cpp \
    utility.cpp \
    view.cpp

HEADERS += \
    callout.h \
        mainwindow.h \
    myserver.h \
    packet.h \
    esp.h \
    person.h \
    dbthread.h \
    elaboratethread.h \
    listenerthread.h \
    utility.h \
    view.h

FORMS += \
        mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    esp.txt
