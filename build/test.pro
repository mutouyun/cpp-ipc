TEMPLATE = app

QT += core testlib
QT -= gui
CONFIG += console c++14
CONFIG -= app_bundle

DESTDIR = ../output

INCLUDEPATH += \
    ../include \
    ../src \
    ../src/platform

HEADERS += \
    ../test/test.h

SOURCES += \
    ../test/main.cpp \
    ../test/test_shm.cpp \
    ../test/test_circ.cpp \
    ../test/test_ipc.cpp

LIBS += -L$${DESTDIR} -lipc
