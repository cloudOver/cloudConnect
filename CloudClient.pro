#-------------------------------------------------
#
# Project created by QtCreator 2015-01-07T10:40:15
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = CloudClient
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += /usr/local/include/

SOURCES += main.cpp \
    syscall.cpp \
    proto/syscall.c \
    log.c

HEADERS += \
    syscall.h \
    proto/request.h \
    proto/syscall.h \
    log.h
