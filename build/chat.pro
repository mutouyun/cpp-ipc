TEMPLATE = app

CONFIG += console
CONFIG += c++14 c++1z # may be useless
CONFIG -= app_bundle
CONFIG -= qt

DESTDIR = ../output

INCLUDEPATH += \
    ../include

SOURCES += \
    ../demo/chat/main.cpp

LIBS += \
    -L$${DESTDIR} -lipc

unix:LIBS += -lrt -lpthread
