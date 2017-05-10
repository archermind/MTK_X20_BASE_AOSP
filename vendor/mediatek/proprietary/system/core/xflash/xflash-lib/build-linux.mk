.NAME                        := XFlashLib

.OBJ                         := $(OUTPUT_PATH)/obj
XFlashLib.SECLIB          := $(SEC_LIB)/XFlashLib/$(HOST)/$(BUILD_TYPE)
XFlashLib.OUTPUT          := $(OUTPUT_PATH)/XFlashLib.lib
XFlashLib.INCLUDE         := $(INCLUDE_PATH) 
XFlashLib.LDFLAGS         := 
XFlashLib.LDFLAGS.Debug   :=
XFlashLib.LDFLAGS.Release :=
XFlashLib.CCFLAGS         := $(call gcc-inc-path,$(XFlashLib.INCLUDE)) -fvisibility="hidden" -fPIC \
                                -D_TRANS_LINUX_FORMAT -DENABLE_64BIT_PROTOCOL -D_USRDLL -DXFLASH_EXPORTS -DMDEBUG_RUNTIME_TRACE -D_MBCS -D_LINUX
XFlashLib.CCFLAGS.Debug   := -D_DEBUG -g
XFlashLib.CCFLAGS.Release := -DNDEBUG
XFlashLib.SO              := libxflash-lib.so
XFlashLib.LIBTYPE.Debug   := Debug Library
XFlashLib.LIBTYPE.Release := Release Library
XFlashLib.LIBFLAGS        := 
XFlashLib.LIBFLAGS.Debug  := -g
XFlashLib.LIBFLAGS.Release:= 
BUILD_RC                     := ./build.rc
BUILD_H                      := ./build.h
VER_MAJOR                    := $(if $(VER_MAJOR),$(VER_MAJOR),7)
VER_PATCH                    := $(if $(VER_PATCH),$(VER_PATCH),00)

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

XFlashLib.SRCLIST := \
    $(call rwildcard,arch/linux/,*.cpp)	\
    $(call rwildcard,logic/,*.cpp)	\
	$(call rwildcard,transfer/,*.cpp)\
	$(call rwildcard,brom/,*.cpp)	\
	$(call rwildcard,common/,*.cpp)	\
	$(call rwildcard,config/,*.cpp)	\
	$(call rwildcard,loader/,*.cpp)	\
	$(call rwildcard,functions/,*.cpp)\
	$(call rwildcard,interface/,*.cpp)	
