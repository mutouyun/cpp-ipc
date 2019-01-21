TEMPLATE = lib
TARGET = ipc

CONFIG -= qt
CONFIG += c++14 c++1z # may be useless

!msvc:QMAKE_CXXFLAGS += -Wno-attributes -Wno-missing-field-initializers -Wno-unused-variable

DEFINES += __IPC_LIBRARY__
DESTDIR = ../output

INCLUDEPATH += \
    ../include \
    ../src

HEADERS += \
    ../include/export.h \
    ../include/def.h \
    ../include/shm.h \
    ../include/elem_def.h \
    ../include/elem_circ.h \
    ../include/elem_link.h \
    ../include/waiter.h \
    ../include/queue.h \
    ../include/ipc.h \
    ../include/rw_lock.h \
    ../include/tls_pointer.h \
    ../include/pool_alloc.h \
    ../include/buffer.h \
    ../src/memory/detail.h \
    ../src/memory/alloc.hpp \
    ../src/memory/wrapper.hpp \
    ../src/memory/resource.hpp \
    ../src/platform/detail.h \
    ../src/platform/waiter.h

SOURCES += \
    ../src/shm.cpp \
    ../src/ipc.cpp \
    ../src/pool_alloc.cpp \
    ../src/buffer.cpp \
    ../src/waiter.cpp

unix {

HEADERS += \
    ../src/platform/waiter_linux.h

SOURCES += \
    ../src/platform/shm_linux.cpp \
    ../src/platform/tls_pointer_linux.cpp

LIBS += -lrt

target.path = /usr/lib
INSTALLS += target

} # unix

else:win32 {

HEADERS += \
    ../src/platform/to_tchar.h \
    ../src/platform/waiter_win.h

SOURCES += \
    ../src/platform/shm_win.cpp \
    ../src/platform/tls_pointer_win.cpp

LIBS += -lKernel32

} # else:win32
