include define.mk
BUILD_TYPE          ?= Debug
LIB_DEFAULT_TYPE    := Dy
LIB_SECOND_TYPE     := St
LIB_TYPE            := $(if $(findstring $(LIB_SECOND_TYPE),$(LIB_TYPE)),$(LIB_SECOND_TYPE),$(LIB_DEFAULT_TYPE))
JOBS				:= $(if $(LINUX),-j1,-j16)
BUILD_TARGET		:=

ifeq ($(MAKECMDGOALS),clean)
clean:
	-$(RM)	-rf ./_Output/
	-$(RM) -rf $(OUTPUT_PATH) 
	-$(foreach b, $(wildcard ./*),$(call clean_obj,$b) )
else

ifeq (off,$(LOG))

include build-$(if $(LINUX),linux,windows).mk
$(eval .all-source := $(XFlashLib.SRCLIST))
$(eval .all-object := $(call c-to-obj,$(.all-source)))

all:create_output_dir $(if $(LINUX),build-linux,build-windows)

create_output_dir:
	-$(MKDIR) -p $(OUTPUT_PATH)
	
	
build-windows: $(if $(findstring $(LIB_SECOND_TYPE),$(LIB_TYPE)),xflash-lib.lib ,xflash-lib.dll)
	@echo build  OK!


build-linux: $(XFlashLib.SO)
	@echo build $* OK! 


xflash-lib.dll: $(.all-object) 
	$(LINK) /nologo  /OUT:"$(OUTPUT_PATH)/$@"  $(XFlashLib.DLLFLAGS) $(XFlashLib.DLLFLAGS.$(BUILD_TYPE))  $^  
	$(CP)	./_Output/*  $(EXPORT_PATH) -fa
	@echo Build $@ Passed


xflash-lib.lib:$(.all-object) 
	$(LINK)	 /lib  /machine:I386 /nologo /OUT:"$(OUTPUT_PATH)/$@" $(XFlashLib.DLLFLAGS) $(XFlashLib.DLLFLAGS.$(BUILD_TYPE)) $^ 
	@echo static library Build Passed
	
$(XFlashLib.SO):$(.all-object) FORCE
	$(CC) -shared -o $(OUTPUT_PATH)/$@ $(XFlashLib.LIBFLAGS) $(XFlashLib.LIBFLAGS.$(BUILD_TYPE)) -Wl,--whole-archive $(.all-object)  lib/linux/libyaml-cpp.a  -Wl,--no-whole-archive $(wildcard /share/pub/boost/lib/152/linux/*.a) -Wl,-soname,$(XFlashLib.SO)
	$(CP)	./_Output/*   $(EXPORT_PATH) -fa
	@echo Build $@ Passed
	
$(.all-object): $(if $(LINUX),%.o ,%.obj): %.cpp
	$(CC) $(XFlashLib.CCFLAGS) $(XFlashLib.CCFLAGS.$(BUILD_TYPE))  $(if $(LINUX), -o $@,"/Fo$(dir $@)")  -c $(call obj-to-c,$@)
	
build-version:
	@echo "build-version"
	$(shell chmod +x $(MINOR))
	$(eval VER_MINOR := $(shell $(MINOR)))
	$(eval VER_BUILD := $(shell $(MINOR) -w))
	$(eval VERSION_STRING := $(VER_MAJOR).$(VER_MINOR).$(VER_BUILD).$(VER_PATCH))
	$(eval BUILD_DATE := $(shell date +%F))
	$(eval BUILD_TIME := $(shell date +%H:%M:%S))
	$(eval BUILD_STAMP := $(BUILD_DATE) $(BUILD_TIME))
	$(eval LIBRARY_TYPE := $(XFlashLib.LIBTYPE.$(BUILD_TYPE)))
	$(eval RELEASE_TYPE := $(if $(filter 00,$(VER_PATCH)),Official-Build,Patch-Build))

	@rm -rf $(BUILD_H)
	$(shell pwd > tmp0)
	$(shell sed -e "s/\/XFlashLib.*//g" tmp0 > tmp)
	$(eval BUILD_INFO := $(shell cat tmp))
	$(eval BUILD_INFO := $(subst /,\\/,$(BUILD_INFO)))
	@sed -e "s/_VER_MAJOR/$(VER_MAJOR)/g" -e "s/_VER_MINOR/$(VER_MINOR)/g" -e "s/_VER_BUILD/$(VER_BUILD)/g" -e "s/_VER_PATCH/$(VER_PATCH)/g" -e "s/_VERSION_STRING/\"$(VERSION_STRING)\"/g" -e "s/_BUILD_DATE/\"$(BUILD_DATE)\"/g" -e "s/_BUILD_TIME/\"$(BUILD_TIME)\"/g" -e "s/_LIBRARY_TYPE/\"$(LIBRARY_TYPE)\"/g" -e "s/_RELEASE_TYPE/\"$(RELEASE_TYPE)\"/g" -e "s/_BUILD_STAMP/\"$(BUILD_STAMP)\"/g" -e "s/_BUILD_INFO/\"$(BUILD_INFO)\"/g" $(BUILD_RC) > $(BUILD_H)
	@echo build-version: $(VERSION_STRING) $(RELEASE_TYPE)

PHONY: FORCE
FORCE:

else
LOG := build_log.txt
$(MAKECMDGOALS): recursive-make
recursive-make:
	make $(JOBS) BUILD_TYPE=$(BUILD_TYPE) LOG=off $(MAKECMDGOALS) 2>&1 | $(if $(LINUX),tee,"$(word 1,$(wildcard ../Tools/tee.exe Tools/tee.exe) tee.exe)") $(LOG)
endif

endif
