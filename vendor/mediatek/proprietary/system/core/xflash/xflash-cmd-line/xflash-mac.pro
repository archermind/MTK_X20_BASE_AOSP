#-------------------------------------------------
#
# Project created by QtCreator 2015-12-30T14:43:33
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = xflash
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += Q_OS_MAC

QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas

QMAKE_CFLAGS_WARN_ON += -Wno-unknown-pragmas

LIBS += -stdlib=libc++

QMAKE_CXXFLAGS += -stdlib=libc++
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -mmacosx-version-min=10.7
QMAKE_LFLAGS += -std=c++11
QMAKE_LFLAGS += -mmacosx-version-min=10.7
QMAKE_LFLAGS +=  -stdlib=libc++

INCLUDEPATH +=  ../xflash-lib/lib/include \
                ../xflash-lib/api \
                ../xflash-lib/inc \
                ./xmain/    \
                ../xflash-lib/  \
                ./core/


LIBS += "../xflash-lib/lib/mac/libboost_system.a"   \
        "../xflash-lib/lib/mac/libboost_filesystem.a"   \
        "../xflash-lib/lib/mac//libboost_log.a"      \
        "../xflash-lib/lib/mac/libboost_chrono.a"   \
        "../xflash-lib/lib/mac/libboost_regex.a"    \
        "../xflash-lib/lib/mac/libboost_thread.a"   \
        "../xflash-lib/lib/mac/libboost_locale.a"    \
        "../xflash-lib/lib/mac/libboost_date_time.a" \
        "../xflash-lib/lib/mac/libboost_program_options.a" \
        "../xflash-lib/lib/mac/libyaml-cpp.a"   \
        "../xflash-lib/lib/libxflash-lib.1.dylib"

SOURCES += main.cpp \
    core/cmds_impl.cpp \
    xmain/commands.cpp

HEADERS += \
    core/cmds_impl.h \
    core/zbuffer.h \
    xmain/commands.h
