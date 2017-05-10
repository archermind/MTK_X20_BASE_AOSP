ifeq ($(LOCAL_MAKEFILE),)
  $(error LOCAL_MAKEFILE is not defined)
endif
ifeq ($(OUTPUT_NAME),)
  $(error $(LOCAL_MAKEFILE): OUTPUT_NAME is not defined)
endif
ifeq ($(TEE_MODE),)
  $(error $(LOCAL_MAKEFILE): TEE_MODE is not defined)
endif
ifneq ($(strip $(DRIVER_UUID)),)
  ifneq ($(strip $(SRC_LIB_C)),)
    module_id := $(OUTPUT_NAME).lib
  else ifneq ($(strip $(SRC_CPP))$(strip $(SRC_C))$(strip $(SRC_ASM)),)
    module_id := $(OUTPUT_NAME).drbin
  else ifneq ($(wildcard $(foreach n,$(TEE_MODE) $(call LowerCase,$(TEE_MODE)) release debug,$(dir $(LOCAL_MAKEFILE))$(n)/$(OUTPUT_NAME).lib)),)
    module_id := $(OUTPUT_NAME).lib
  else
    module_id := $(OUTPUT_NAME).drbin
  endif
else ifneq ($(strip $(TRUSTLET_UUID)),)
  ifeq ($(BUILD_TRUSTLET_LIBRARY_ONLY),yes)
    module_id := $(OUTPUT_NAME).lib
  else ifeq ($(GP_ENTRYPOINTS),Y)
    module_id := $(OUTPUT_NAME).tabin
  else
    module_id := $(OUTPUT_NAME).tlbin
  endif
else
  $(error $(LOCAL_MAKEFILE): DRIVER_UUID and TRUSTLET_UUID are not defined)
endif

ifneq ($(filter $(module_id),$(TEE_ALL_MODULES)),)
  ifneq ($(LOCAL_MAKEFILE),$(TEE_ALL_MODULES.$(module_id).MAKEFILE))
    $(error $(LOCAL_MAKEFILE): $(module_id) already defined by $(TEE_ALL_MODULES.$(module_id).MAKEFILE))
  endif
endif

TEE_ALL_MODULES := $(TEE_ALL_MODULES) $(module_id)
TEE_ALL_MODULES.$(module_id).MAKEFILE := $(LOCAL_MAKEFILE)
TEE_ALL_MODULES.$(module_id).PATH := $(patsubst %/makefile.mk,%,$(patsubst %/Locals/Code/makefile.mk,%,$(LOCAL_MAKEFILE)))
ifneq ($(strip $(DRIVER_UUID)),)
    TEE_ALL_MODULES.$(module_id).CLASS := DRIVER
    TEE_ALL_MODULES.$(module_id).OUTPUT_ROOT := $(TEE_DRIVER_OUTPUT_PATH)/$(OUTPUT_NAME)
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).AXF := $(TEE_DRIVER_OUTPUT_PATH)/$(OUTPUT_NAME)/$(TEE_MODE)/$(OUTPUT_NAME).axf
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT := $(TEE_DRIVER_OUTPUT_PATH)/$(OUTPUT_NAME)/$(TEE_MODE)/$(strip $(DRIVER_UUID)).drbin
    TEE_ALL_MODULES.$(module_id).INSTALLED := $(TEE_APP_INSTALL_PATH)/$(strip $(DRIVER_UUID)).drbin $(TEE_APP_INSTALL_PATH)/$(strip $(DRIVER_UUID)).tlbin
  ifneq ($(strip $(SRC_LIB_C)),)
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).LIB := $(TEE_DRIVER_OUTPUT_PATH)/$(OUTPUT_NAME)/$(TEE_MODE)/$(OUTPUT_NAME).lib
  endif
  ifneq ($(strip $(EXTRA_LIBS)),)
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).REQUIRED := $(notdir $(EXTRA_LIBS))
  endif
else ifneq ($(strip $(TRUSTLET_UUID)),)
    TEE_ALL_MODULES.$(module_id).CLASS := TRUSTLET
    TEE_ALL_MODULES.$(module_id).OUTPUT_ROOT := $(TEE_TRUSTLET_OUTPUT_PATH)/$(OUTPUT_NAME)
  ifeq ($(BUILD_TRUSTLET_LIBRARY_ONLY),yes)
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).LIB := $(TEE_TRUSTLET_OUTPUT_PATH)/$(OUTPUT_NAME)/$(TEE_MODE)/$(OUTPUT_NAME).lib
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT := $(TEE_TRUSTLET_OUTPUT_PATH)/$(OUTPUT_NAME)/$(TEE_MODE)/$(OUTPUT_NAME).lib
  else ifeq ($(GP_ENTRYPOINTS),Y)
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).AXF := $(TEE_TRUSTLET_OUTPUT_PATH)/$(OUTPUT_NAME)/$(TEE_MODE)/$(OUTPUT_NAME).axf
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT := $(TEE_TRUSTLET_OUTPUT_PATH)/$(OUTPUT_NAME)/$(TEE_MODE)/$(strip $(TRUSTLET_UUID)).tabin
    TEE_ALL_MODULES.$(module_id).INSTALLED := $(TEE_APP_INSTALL_PATH)/$(strip $(TRUSTLET_UUID)).tabin
  else
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).AXF := $(TEE_TRUSTLET_OUTPUT_PATH)/$(OUTPUT_NAME)/$(TEE_MODE)/$(OUTPUT_NAME).axf
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT := $(TEE_TRUSTLET_OUTPUT_PATH)/$(OUTPUT_NAME)/$(TEE_MODE)/$(strip $(TRUSTLET_UUID)).tlbin
    TEE_ALL_MODULES.$(module_id).INSTALLED := $(TEE_APP_INSTALL_PATH)/$(strip $(TRUSTLET_UUID)).tlbin
  endif
  ifneq ($(strip $(CUSTOMER_DRIVER_LIBS)),)
    TEE_ALL_MODULES.$(module_id).$(TEE_MODE).REQUIRED := $(notdir $(CUSTOMER_DRIVER_LIBS))
  endif
else
  $(error $(LOCAL_MAKEFILE): DRIVER_UUID and TRUSTLET_UUID are not defined)
endif

$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT): PRIVATE_PATH := $(abspath $(TEE_ALL_MODULES.$(module_id).PATH))
$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT): PRIVATE_MAKEFILE := $(abspath $(TEE_ALL_MODULES.$(module_id).MAKEFILE))
$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT): PRIVATE_MAKE_OPTION := $(TEE_GLOBAL_MAKE_OPTION)
$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT): PRIVATE_MAKE_OPTION += TOOLCHAIN=$(TEE_TOOLCHAIN)
$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT): PRIVATE_MAKE_OPTION += MODE=$(TEE_MODE)
$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT): PRIVATE_MAKE_OPTION += TEE_MODE=$(TEE_MODE)
$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT): PRIVATE_MAKE_OPTION += OUTPUT_ROOT=$(abspath $(TEE_ALL_MODULES.$(module_id).OUTPUT_ROOT))
$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT): PRIVATE_MAKE_OPTION += TEE_DRIVER_OUTPUT_PATH=$(abspath $(TEE_DRIVER_OUTPUT_PATH))
$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT): FORCE
	@echo TEE build: $@
	$(MAKE) -C $(PRIVATE_PATH) -f $(PRIVATE_MAKEFILE) $(PRIVATE_MAKE_OPTION) all

ifeq ($(TEE_ALL_MODULES.$(module_id).INSTALLED),)
TEE_modules_to_check := $(TEE_modules_to_check) $(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT)
else ifneq ($(TEE_MODE),$(strip $(TEE_INSTALL_MODE)))
TEE_modules_to_check := $(TEE_modules_to_check) $(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT)
else
TEE_modules_to_install := $(TEE_modules_to_install) $(TEE_ALL_MODULES.$(module_id).INSTALLED)
$(TEE_ALL_MODULES.$(module_id).INSTALLED): $(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT)
	@echo Copying: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) cp -f $(dir $<)$(notdir $@) $@

endif

ifeq (dump,dump)
TEE_dumpvar_log := $(TRUSTZONE_OUTPUT_PATH)/dump/$(module_id).log
$(TEE_dumpvar_log): DUMPVAR_VALUE := $(DUMPVAR_VALUE)
$(TEE_dumpvar_log): DUMPVAR_VALUE += $(TEE_MODE).BUILT=$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).BUILT)
$(TEE_dumpvar_log): DUMPVAR_VALUE += $(TEE_MODE).LIB=$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).LIB)
$(TEE_dumpvar_log): DUMPVAR_VALUE += $(TEE_MODE).AXF=$(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).AXF)
$(TEE_dumpvar_log): PRIVATE_$(TEE_MODE).REQUIRED := $(TEE_ALL_MODULES.$(module_id).$(TEE_MODE).REQUIRED)
  ifeq ($(TEE_MODE),$(strip $(TEE_INSTALL_MODE)))
$(TEE_dumpvar_log): DUMPVAR_VALUE += OUTPUT_NAME=$(strip $(OUTPUT_NAME))
    ifneq ($(strip $(DRIVER_UUID)),)
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_UUID=$(strip $(DRIVER_UUID))
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_MEMTYPE=$(strip $(DRIVER_MEMTYPE))
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_NO_OF_THREADS=$(strip $(DRIVER_NO_OF_THREADS))
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_SERVICE_TYPE=$(strip $(DRIVER_SERVICE_TYPE))
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_KEYFILE=$(strip $(DRIVER_KEYFILE))
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_FLAGS=$(strip $(DRIVER_FLAGS))
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_VENDOR_ID=$(strip $(DRIVER_VENDOR_ID))
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_NUMBER=$(strip $(DRIVER_NUMBER))
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_INTERFACE_VERSION_MAJOR=$(strip $(DRIVER_INTERFACE_VERSION_MAJOR))
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_INTERFACE_VERSION_MINOR=$(strip $(DRIVER_INTERFACE_VERSION_MINOR))
$(TEE_dumpvar_log): DUMPVAR_VALUE += DRIVER_INTERFACE_VERSION=$(strip $(DRIVER_INTERFACE_VERSION))
    else ifneq ($(strip $(TRUSTLET_UUID)),)
$(TEE_dumpvar_log): DUMPVAR_VALUE += TRUSTLET_UUID=$(strip $(TRUSTLET_UUID))
$(TEE_dumpvar_log): DUMPVAR_VALUE += TRUSTLET_MEMTYPE=$(strip $(TRUSTLET_MEMTYPE))
$(TEE_dumpvar_log): DUMPVAR_VALUE += TRUSTLET_NO_OF_THREADS=$(strip $(TRUSTLET_NO_OF_THREADS))
$(TEE_dumpvar_log): DUMPVAR_VALUE += TRUSTLET_SERVICE_TYPE=$(strip $(TRUSTLET_SERVICE_TYPE))
$(TEE_dumpvar_log): DUMPVAR_VALUE += TRUSTLET_KEYFILE=$(strip $(TRUSTLET_KEYFILE))
$(TEE_dumpvar_log): DUMPVAR_VALUE += TRUSTLET_FLAGS=$(strip $(TRUSTLET_FLAGS))
$(TEE_dumpvar_log): DUMPVAR_VALUE += TRUSTLET_INSTANCES=$(strip $(TRUSTLET_INSTANCES))
$(TEE_dumpvar_log): DUMPVAR_VALUE += TRUSTLET_MOBICONFIG_KEY=$(strip $(TRUSTLET_MOBICONFIG_KEY))
$(TEE_dumpvar_log): DUMPVAR_VALUE += TRUSTLET_MOBICONFIG_KID=$(strip $(TRUSTLET_MOBICONFIG_KID))
$(TEE_dumpvar_log): DUMPVAR_VALUE += TRUSTLET_MOBICONFIG_USE=$(strip $(TRUSTLET_MOBICONFIG_USE))
$(TEE_dumpvar_log): DUMPVAR_VALUE += BUILD_TRUSTLET_LIBRARY_ONLY=$(strip $(BUILD_TRUSTLET_LIBRARY_ONLY))
$(TEE_dumpvar_log): DUMPVAR_VALUE += GP_ENTRYPOINTS=$(strip $(GP_ENTRYPOINTS))
    endif
$(TEE_dumpvar_log): DUMPVAR_VALUE += TBASE_API_LEVEL=$(strip $(TBASE_API_LEVEL))
$(TEE_dumpvar_log): DUMPVAR_VALUE += HEAP_SIZE_INIT=$(strip $(HEAP_SIZE_INIT))
$(TEE_dumpvar_log): DUMPVAR_VALUE += HEAP_SIZE_MAX=$(strip $(HEAP_SIZE_MAX))
$(TEE_dumpvar_log): DUMPVAR_VALUE += MAKEFILE=$(TEE_ALL_MODULES.$(module_id).MAKEFILE)
$(TEE_dumpvar_log): DUMPVAR_VALUE += PATH=$(TEE_ALL_MODULES.$(module_id).PATH)
$(TEE_dumpvar_log): DUMPVAR_VALUE += CLASS=$(TEE_ALL_MODULES.$(module_id).CLASS)
$(TEE_dumpvar_log): PRIVATE_INSTALLED := $(TEE_ALL_MODULES.$(module_id).INSTALLED)
TEE_modules_to_check := $(TEE_modules_to_check) $(TEE_dumpvar_log)
$(TEE_dumpvar_log): FORCE
	@echo Dump: $@
	@mkdir -p $(dir $@)
	@rm -f $@
	@$(foreach v,$(DUMPVAR_VALUE),echo $(v) >>$@;)
	@$(foreach v,$(TEE_BUILD_MODE),echo $(v).REQUIRED=$(PRIVATE_$(v).REQUIRED) >>$@;)
	@echo INSTALLED=$(PRIVATE_INSTALLED) >>$@

  endif
endif
