#-------------------------------------------------
#
# Project created by QtCreator 2015-12-30T14:08:55
#
#-------------------------------------------------

QT       -= core gui
QT      += serialport

TARGET = xflash-lib
TEMPLATE = lib

DEFINES += XFLASHLIB_LIBRARY
DEFINES += Q_OS_MAC

QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas \
                          -Wmissing-field-initializers  \
                          -Wno-unused-variable \
                          -Wno-ignored-attributes   \
                          -Wno-unused-function  \
                          -Wno-unused-parameter \
                          -Wno-reorder  \
                          -Wno-missing-field-initializers   \
                          -Wno-missing-braces   \
                          -Wno-unused-private-field \
                          -Wno-c++11-compat-deprecated-writable-strings


QMAKE_CFLAGS_WARN_ON += -Wno-unknown-pragmas \
                        -Wmissing-field-initializers \
                          -Wno-unused-variable \
                          -Wno-ignored-attributes   \
                          -Wno-unused-function  \
                          -Wno-unused-parameter \
                          -Wno-reorder  \
                          -Wno-missing-field-initializers   \
                          -Wno-missing-braces   \
                          -Wno-unused-private-field \
                          -Wno-c++11-compat-deprecated-writable-strings

LIBS += -stdlib=libc++

QMAKE_CXXFLAGS += -stdlib=libc++
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -mmacosx-version-min=10.7
QMAKE_LFLAGS += -std=c++11
QMAKE_LFLAGS += -mmacosx-version-min=10.7
QMAKE_LFLAGS +=  -stdlib=libc++

INCLUDEPATH +=  ./lib/include \
                ./api \
                ./inc \
                ./lib/include/yaml-cpp-0.5.1/   \
                ./arch/mac/

LIBS += "./lib/mac/libboost_system.a"   \
        "./lib/mac/libboost_filesystem.a"   \
        "./lib/mac//libboost_log.a"      \
        "./lib/mac/libboost_chrono.a"   \
        "./lib/mac/libboost_regex.a"    \
        "./lib/mac/libboost_thread.a"   \
        "./lib/mac/libboost_locale.a"    \
        "./lib/mac/libboost_date_time.a" \
        "./lib/mac/libboost_program_options.a" \
        "./lib/mac/libyaml-cpp.a"

SOURCES += \
    arch/mac/comm_engine.cpp \
    arch/mac/UsbScan.cpp \
    brom/boot_rom_logic.cpp \
    brom/boot_rom_sla_cb.cpp \
    brom/boot_rom.cpp \
    common/call_log.cpp \
    common/utils.cpp \
    common/zlog.cpp \
    config/lib_config_parser.cpp \
    functions/gui_callbacks.cpp \
    functions/scatter/scatter_transfer.cpp \
    functions/scatter/yaml_reader.cpp \
    interface/code_translate.cpp \
    interface/xflash_api.cpp \
    interface/xflash_test.cpp \
    loader/file/loader_image.cpp \
    logic/connection.cpp \
    logic/kernel.cpp \
    logic/logic.cpp

HEADERS += xflash-lib_global.h \
    api/xflash_api.h \
    api/xflash_struct.h \
    arch/host.h \
    arch/IUsbScan.h \
    arch/mac/UsbScan.h \
    brom/boot_rom_cmd.h \
    brom/boot_rom_logic.h \
    brom/boot_rom_sla_cb.h \
    brom/boot_rom.h \
    common/call_log.h \
    common/common_include.h \
    common/runtime_exception.h \
    common/types.h \
    common/utils.h \
    common/zbuffer.h \
    common/zlog.h \
    config/lib_config_parser.h \
    functions/gui_callbacks.h \
    functions/scatter/IScatterFileReader.h \
    functions/scatter/scatter_transfer.h \
    functions/scatter/yaml_reader.h \
    inc/code_translate.h \
    inc/error_code.h \
    inc/type_define.h \
    interface/rc.h \
    loader/file/loader_image.h \
    loader/file/loader_struct.h \
    logic/connection.h \
    logic/kernel.h \
    logic/logic.h \
    transfer/comm_engine.h \
    transfer/ITransmission.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    xml-file/lib.cfg.xml
