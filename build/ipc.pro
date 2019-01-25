TEMPLATE = lib
TARGET = ipc

CONFIG -= qt
CONFIG += c++14 c++1z # may be useless

DEFINES += __IPC_LIBRARY__
DESTDIR = ../output

INCLUDEPATH += \
    ../include \
    ../src

HEADERS += \
    ../include/export.h \
    ../include/def.h \
    ../include/shm.h \
    ../include/waiter.h \
    ../include/queue.h \
    ../include/ipc.h \
    ../include/rw_lock.h \
    ../include/tls_pointer.h \
    ../include/pool_alloc.h \
    ../include/buffer.h \
    ../src/memory/detail.h \
    ../src/memory/alloc.h \
    ../src/memory/wrapper.h \
    ../src/memory/resource.h \
    ../src/platform/detail.h \
    ../src/platform/waiter_wrapper.h \
    ../src/circ/elem_def.h \
    ../src/circ/elem_array.h \
    ../src/prod_cons.h \
    ../src/policy.h \
    ../src/queue.h \
    ../src/log.h \
    ../src/id_pool.h

SOURCES += \
    ../src/shm.cpp \
    ../src/ipc.cpp \
    ../src/pool_alloc.cpp \
    ../src/buffer.cpp \
    ../src/waiter.cpp \
    ../src/channel.cpp

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
