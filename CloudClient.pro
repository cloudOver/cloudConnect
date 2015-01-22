#-------------------------------------------------
#
# Project created by QtCreator 2015-01-07T10:40:15
#
#-------------------------------------------------

QT       -= gui core

TARGET = CloudClient
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += /usr/local/include/
LIBS += -L/usr/local/lib/ -lzmq
SOURCES += main.cpp \
    proto/syscall.c \
    log.c \
    proto/file.c

HEADERS += \
    proto/syscall.h \
    log.h \
    proto/file.h
