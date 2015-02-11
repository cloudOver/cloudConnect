#-------------------------------------------------
#
# Project created by QtCreator 2015-01-07T10:40:15
#
#-------------------------------------------------

QT       -= gui core

TARGET = CloudConnect
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += /usr/local/include/ /usr/include/glib-2.0/ /usr/local/include/glib-2.0/ /usr/local/lib/glib-2.0/include/ /usr/lib/x86_64-linux-gnu/glib-2.0/include/

LIBS += -L/usr/local/lib/ -lzmq -lm -lglib-2.0
SOURCES += \
    proto/syscall.c \
    log.c \
    proto/file.c \
    main.c \
    router.c \
    forwarder.c

HEADERS += \
    proto/syscall.h \
    log.h \
    proto/file.h \
    router.h \
    kvec.h \
    forwarder.h
