.NAME                        := xflash

.OBJ                         := $(OUTPUT_PATH)/obj
XflashExe.OUTPUT          := $(OUTPUT_PATH)/xflash
XflashExe.INCLUDE         := $(INCLUDE_PATH) 
XflashExe.LDFLAGS         := 
XflashExe.LDFLAGS.Debug   :=
XflashExe.LDFLAGS.Release :=
XflashExe.CCFLAGS         := $(call gcc-inc-path,$(XflashExe.INCLUDE))  \
                                -D_TRANS_LINUX_FORMAT -DENABLE_64BIT_PROTOCOL -DMDEBUG_RUNTIME_TRACE -D_MBCS -D_LINUX
XflashExe.CCFLAGS.Debug   := -D_DEBUG -g
XflashExe.CCFLAGS.Release := -DNDEBUG
XflashExe.SO              := libxflash-lib.so
XflashExe.LIBTYPE.Debug   := Debug Library
XflashExe.LIBTYPE.Release := Release Library
XflashExe.LIBFLAGS        := 
XflashExe.LIBFLAGS.Debug  := -g
XflashExe.LIBFLAGS.Release:= 
BUILD_RC                     := ./build.rc
BUILD_H                      := ./build.h
VER_MAJOR                    := $(if $(VER_MAJOR),$(VER_MAJOR),7)
VER_PATCH                    := $(if $(VER_PATCH),$(VER_PATCH),00)

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

XflashExe.SRCLIST := \
    $(call rwildcard,xmain/,*.cpp)	\
	  $(call rwildcard,core/,*.cpp)
