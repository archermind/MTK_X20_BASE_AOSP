VC2008Path          := C:\Program Files\Microsoft Visual Studio 9.0\VC
VC2010Path          := C:\Program Files\Microsoft Visual Studio 10.0\VC
VCPath              := $(if $(VC2010),$(VC2010Path),$(VC2008Path))
SDKV6Path           := C:\Program Files\Microsoft SDKs\Windows\v6.0A
SDKV7Path           := C:\Program Files\Microsoft SDKs\Windows\v7.0A
SDKPath             := $(if $(VC2010),$(SDKV7Path),$(SDKV6Path))
CC                  := $(if $(LINUX),g++,"$(VCPath)\bin\cl.exe")
LIB                 := $(if $(LINUX),ar,"$(VCPath)\bin\lib.exe")
LINK                := $(if $(LINUX),ld,"$(VCPath)\bin\link.exe")
MT                  := $(if $(LINUX),mt,"$(SDKPath)\bin\mt.exe")
RC                  := $(if $(LINUX),rc,"$(SDKPath)\bin\rc.exe")

# relative to flashtoollib root
RM                  := $(if $(LINUX),rm,$(word 1,$(wildcard ../Tools/rm.exe Tools/rm.exe) rm.exe))
MKDIR               := $(if $(LINUX),mkdir,$(word 1,$(wildcard ../Tools/mkdir.exe Tools/mkdir.exe) mkdir.exe))
CP                  := $(if $(LINUX),cp,$(word 1,$(wildcard ../Tools/cp.exe Tools/cp.exe) cp.exe))
IMPDEF              := $(if $(LINUX),impdef,$(word 1,$(wildcard ../Tools/impdef.exe Tools/impdef.exe) impdef.exe))
IMPLIB              := $(if $(LINUX),implib,$(word 1,$(wildcard ../Tools/implib.exe Tools/implib.exe) implib.exe))
ZIP                 := $(if $(LINUX),zip,$(word 1,$(wildcard ../Tools/zip.exe Tools/zip.exe) zip.exe))
CHMOD               := $(if $(LINUX),chmod,$(word 1,$(wildcard ../Tools/chmod.exe Tools/chmod.exe) chmod.exe))
HPARSE              := $(if $(LINUX),hparse,"$(word 1,$(wildcard ../Tools/hparse.exe Tools/hparse.exe) hparse.exe)")
MINOR               := $(if $(LINUX),"$(word 1,$(wildcard ../Tools/minor Tools/minor) minor)","$(word 1,$(wildcard ../Tools/minor.exe Tools/minor.exe) minor.exe)")
FIND				:= $(if $(LINUX),find,"$(word 1,$(wildcard ../Tools/find.exe Tools/find.exe) find.exe)")
XARGS				:= $(if $(LINUX),xargs,"$(word 1,$(wildcard ../Tools/xargs.exe Tools/xargs.exe) xargs.exe)")

LIB_PATH            := ./lib
HOST	            := $(if $(LINUX),linux,windows)
OUTPUT_PATH         := ./_Output/$(HOST)/$(BUILD_TYPE)
EXPORT_PATH			:= ../lib_Output


# global options
INCLUDE_PATH        :=  -I ./	\
					   	$(if $(LINUX), -I "./arch/linux", -I "./arch/win") \
						$(if $(LINUX), -I "/share/pub/boost/include/152", -I "D:\home\boost157") \
						$(if $(LINUX), , -I "$(VCPath)\include") \
						$(if $(LINUX), , -I "$(SDKPath)\Include") \
						-I "../inc" \
						-I "../api" \
						-I "./arch" \
						-I "./brom" \
						-I "./common" \
						-I "./lib/include/yaml-cpp-0.5.1/" \
						-I "./config" \
						-I "./loader/file/" \
						-I "./functions/" \
						-I "./functions/scatter" \
						-I "./interface" \
						-I "./logic" \
						-I "./transfer" 

define generate-dependency-windows
dependency/$2.d: $2 $1/$(OBJECT_PATH)/.dummy $(OUTPUT_PATH)/.dummy
	@$(MKDIR) -p $(patsubst %/,%,$(dir dependency/$2.d))
	$(CC) $($1.CCFLAGS) $($1.CCFLAGS.$(BUILD_TYPE)) /showIncludes $2 | $(HPARSE) $(o.2) dependency/$2.d 
ifeq ($(filter $(patsubst clean-%,clean,$(MAKECMDGOALS)),clean),)
    -include dependency/$2.d
endif
endef

define generate-dependency-linux
dependency/$2.d: $2 $1/$(OBJECT_PATH)/.dummy $(OUTPUT_PATH)/.dummy
	@$(MKDIR) -p $(patsubst %/,%,$(dir dependency/$2.d))
	$(CC) $($1.CCFLAGS) $($1.CCFLAGS.$(BUILD_TYPE)) -M -MT '$(o.2)' $2 > dependency/$2.d 
ifeq ($(filter $(patsubst clean-%,clean,$(MAKECMDGOALS)),clean),)
    -include dependency/$2.d
endif
endef


define c-to-obj
$(addprefix ./,$(patsubst %.cpp,$(if $(LINUX),%.o,%.obj), $1  ))
endef

define obj-to-c
$(addprefix ./, $(patsubst $(if $(LINUX),%.o,%.obj),%.cpp,$1 ))
endef

define c-to-obj-base
$(addprefix $(.OBJ),$(patsubst %.cpp,$(if $(LINUX),%.o,%.obj),$(notdir $1)  ))
endef

define vc-inc-path
$(subst -I,/I,$1)
endef

define gcc-inc-path
$(subst -I,-I,$1)
endef

define clean_obj
$(FIND) $1 -name *.o* | $(XARGS) rm -f
 
endef
