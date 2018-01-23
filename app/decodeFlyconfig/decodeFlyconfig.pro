#-------------------------------------------------
#
# Project created by QtCreator 2016-09-13T15:43:10
#
#-------------------------------------------------

QT       -= gui

TARGET = decodeFlyconfig
TEMPLATE = app
DESTDIR =$$PWD/../out/$$TARGET


SOURCES += *.c

INCLUDEPATH += $$PWD/../../include \


LIBS += -ldl


unix {
    target.path = /usr/lib
    INSTALLS += target
}


