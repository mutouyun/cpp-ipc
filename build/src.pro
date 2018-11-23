TEMPLATE = lib
TARGET = ipc

CONFIG += c++14
CONFIG -= qt

DEFINES += __IPC_LIBRARY__
DESTDIR = ../output

INCLUDEPATH += \
    ../include \
    ../src \
    ../src/platform

HEADERS += \
    ../include/export.h \
    ../include/shm.h \
    ../src/circ_elem_array.h

SOURCES += \
    ../src/shm.cpp

unix {

SOURCES += \
    ../src/platform/shm_linux.cpp

LIBS += -lrt

target.path = /usr/lib
INSTALLS += target

} # unix

else:win32 {

SOURCES += \
    ../src/platform/shm_win.cpp

LIBS += -lKernel32

} # else:win32
