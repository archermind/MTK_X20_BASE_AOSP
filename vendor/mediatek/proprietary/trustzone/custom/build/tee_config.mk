include $(TRUSTZONE_CUSTOM_BUILD_PATH)/common_config.mk

##
# global setting
##
TEE_BUILD_MODE ?= Debug Release
ifeq ($(TARGET_BUILD_VARIANT),eng)
  TEE_INSTALL_MODE ?= Debug
else
  TEE_INSTALL_MODE ?= Release
endif
TEE_TOOLCHAIN ?= GNU


##
# vendor/mediatek/proprietary/trustzone/build.sh
# source file path
##
TEE_PRIVATE_TRUSTLET_PATH := $(MTK_PATH_SOURCE)/trustzone/trustonic/private/trustlets
TEE_PROTECT_TRUSTLET_PATH := $(MTK_PATH_SOURCE)/trustzone/trustonic/secure/trustlets
TEE_SOURCE_TRUSTLET_PATH  := $(MTK_PATH_SOURCE)/trustzone/trustonic/source/trustlets

# driver/trustlet module path list
TEE_ALL_MODULE_MAKEFILE :=
# $(1): path
# $(2): common or platform
# $(3): sub-path
define mtk_tee_find_module_makefile
$(firstword \
  $(wildcard \
    $(TEE_PRIVATE_TRUSTLET_PATH)/$(strip $(1))/$(if $(filter platform,$(2)),platform/$(ARCH_MTK_PLATFORM),$(strip $(2)))/$(strip $(3))/Locals/Code/makefile.mk \
    $(TEE_PROTECT_TRUSTLET_PATH)/$(strip $(1))/$(if $(filter platform,$(2)),platform/$(ARCH_MTK_PLATFORM),$(strip $(2)))/$(strip $(3))/Locals/Code/makefile.mk \
    $(TEE_SOURCE_TRUSTLET_PATH)/$(strip $(1))/$(if $(filter platform,$(2)),platform/$(ARCH_MTK_PLATFORM),$(strip $(2)))/$(strip $(3))/Locals/Code/makefile.mk \
    $(TEE_SOURCE_TRUSTLET_PATH)/$(strip $(1))/$(if $(filter platform,$(2)),platform/$(ARCH_MTK_PLATFORM),$(strip $(2)))/$(strip $(3))/makefile.mk \
  )\
)
endef
include $(MTK_PATH_SOURCE)/trustzone/trustonic/source/build/platform/$(ARCH_MTK_PLATFORM)/tee_config.mk


##
# vendor/trustonic/platform/mtXXXX/t-base/build.sh
# TEE_MACH_TYPE for denali
##
export TEE_MACH_TYPE := $(MTK_MACH_TYPE)


##
# Locals/Build/Build.sh
# SDK path
##
export TLSDK_DIR := $(CURDIR)/$(TEE_SDK_ROOT)/TlSdk/Out
export DRSDK_DIR := $(CURDIR)/$(TEE_SDK_ROOT)/DrSdk/Out


##
# vendor/mediatek/proprietary/trustzone/build.sh
# drbin/tlbin build/install rule
##
TEE_DRIVER_OUTPUT_PATH := $(TRUSTZONE_OUTPUT_PATH)/driver
TEE_TRUSTLET_OUTPUT_PATH := $(TRUSTZONE_OUTPUT_PATH)/trustlet
TEE_TLC_OUTPUT_PATH := $(TRUSTZONE_OUTPUT_PATH)/tlc
TEE_APP_INSTALL_PATH := $(TARGET_OUT_APPS)/mcRegistry
export TEE_ADDITIONAL_DEPENDENCIES := $(abspath $(TRUSTZONE_PROJECT_MAKEFILE) $(TRUSTZONE_CUSTOM_BUILD_PATH)/common_config.mk $(TRUSTZONE_CUSTOM_BUILD_PATH)/tee_config.mk $(MTK_PATH_SOURCE)/trustzone/trustonic/source/build/platform/$(ARCH_MTK_PLATFORM)/tee_config.mk)
TEE_CLEAR_VARS := $(TRUSTZONE_CUSTOM_BUILD_PATH)/tee_clear_vars.mk
TEE_BASE_RULES := $(TRUSTZONE_CUSTOM_BUILD_PATH)/tee_base_rules.mk
TEE_LIB_MODULES := $(TEE_ALL_MODULES)
TEE_ALL_MODULES :=
TEE_modules_to_install :=
TEE_modules_to_check :=
ifeq ($(strip $(SHOW_COMMANDS)),)
  TEE_GLOBAL_MAKE_OPTION += --silent
endif
ifneq ($(TRUSTZONE_ROOT_DIR),)
  TEE_GLOBAL_MAKE_OPTION += ROOTDIR=$(TRUSTZONE_ROOT_DIR)
endif
TEE_GLOBAL_MAKE_OPTION += MTK_PROJECT=$(MTK_PROJECT)
TEE_DUMP_MAKEFILE_ONLY := true
$(foreach p,$(sort $(TEE_ALL_MODULE_MAKEFILE)),\
	$(eval include $(TEE_CLEAR_VARS))\
	$(eval LOCAL_MAKEFILE := $(p))\
	$(eval include $(LOCAL_MAKEFILE))\
	$(foreach n,$(TEE_BUILD_MODE),\
		$(eval TEE_MODE := $(n))\
		$(eval include $(TEE_BASE_RULES))\
	)\
)
TEE_DUMP_MAKEFILE_ONLY :=
# library and include path dependency between modules
$(foreach m,$(sort $(TEE_ALL_MODULES)),\
	$(foreach n,$(TEE_BUILD_MODE),\
		$(foreach r,$(TEE_ALL_MODULES.$(m).$(n).REQUIRED),\
			$(eval $(TEE_ALL_MODULES.$(m).$(n).BUILT): $(TEE_ALL_MODULES.$(r).$(n).BUILT))\
		)\
		$(foreach r,drutils.lib $(TEE_ALL_MODULES.$(m).$(n).REQUIRED),\
			$(eval s := $(call UpperCase,$(basename $(r))))\
			$(eval $(TEE_ALL_MODULES.$(m).$(n).BUILT): PRIVATE_MAKE_OPTION += COMP_PATH_$(basename $(r))=$(abspath $(TEE_ALL_MODULES.$(r).PATH)))\
			$(eval $(TEE_ALL_MODULES.$(m).$(n).BUILT): PRIVATE_MAKE_OPTION += $(s)_DIR=$(abspath $(TEE_ALL_MODULES.$(r).PATH)))\
			$(eval $(TEE_ALL_MODULES.$(m).$(n).BUILT): PRIVATE_MAKE_OPTION += $(s)_OUT_DIR=$(abspath $(TEE_ALL_MODULES.$(r).OUTPUT_ROOT)))\
		)\
	)\
)
# copy headers
$(foreach m,$(filter-out $(TEE_ALL_MODULES) $(TEE_LIB_MODULES),tlutils.tlbin drutils.lib),\
	$(if $(TEE_ALL_MODULES.$(m).PATH),,\
		$(eval TEE_ALL_MODULES.$(m).PATH := $(TARGET_OUT_HEADERS)/$(m))\
	)\
)


### TEE SETTING ###
ifeq ($(TEE_INSTALL_MODE),Debug)
  TEE_INSTALL_MODE_LC := debug
  TEE_ENDORSEMENT_PUB_KEY := $(MTK_PATH_SOURCE)/trustzone/trustonic/source/bsp/platform/$(ARCH_MTK_PLATFORM)/kernel/debugEndorsementPubKey.pem
else
  TEE_INSTALL_MODE_LC := release
  TEE_ENDORSEMENT_PUB_KEY := $(MTK_PATH_SOURCE)/trustzone/trustonic/source/bsp/platform/$(ARCH_MTK_PLATFORM)/kernel/endorsementPubKey.pem
endif
TEE_TRUSTLET_KEY := $(MTK_PATH_SOURCE)/trustzone/trustonic/source/bsp/platform/$(ARCH_MTK_PLATFORM)/kernel/pairVendorTltSig.pem
TEE_ORI_IMAGE_NAME := $(MTK_PATH_SOURCE)/trustzone/trustonic/source/bsp/platform/$(ARCH_MTK_PLATFORM)/kernel/$(TEE_INSTALL_MODE)/$(ARCH_MTK_PLATFORM)_mobicore_$(TEE_INSTALL_MODE_LC).raw
TEE_RAW_IMAGE_NAME := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/$(ARCH_MTK_PLATFORM)_tee_$(TEE_INSTALL_MODE_LC)_raw.img
TEE_TEMP_PADDING_FILE := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/$(ARCH_MTK_PLATFORM)_tee_$(TEE_INSTALL_MODE_LC)_pad.txt
TEE_TEMP_CFG_FILE := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/img_hdr_tee.cfg
TEE_SIGNED_IMAGE_NAME := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/$(ARCH_MTK_PLATFORM)_tee_$(TEE_INSTALL_MODE_LC)_signed.img
TEE_PADDING_IMAGE_NAME := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/$(ARCH_MTK_PLATFORM)_tee_$(TEE_INSTALL_MODE_LC)_pad.img
TEE_COMP_IMAGE_NAME := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/$(ARCH_MTK_PLATFORM)_tee.img

$(TEE_RAW_IMAGE_NAME): $(COMP_PATH_MobiConfig)/Bin/MobiConfig.jar $(TEE_ORI_IMAGE_NAME) $(TEE_TRUSTLET_KEY) $(TEE_ENDORSEMENT_PUB_KEY) $(TEE_ADDITIONAL_DEPENDENCIES)
	@echo TEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) java -jar $(COMP_PATH_MobiConfig)/Bin/MobiConfig.jar -c -i $(TEE_ORI_IMAGE_NAME) -o $@ -k $(TEE_TRUSTLET_KEY) -ek $(TEE_ENDORSEMENT_PUB_KEY)

$(TEE_TEMP_PADDING_FILE): ALIGNMENT=512
$(TEE_TEMP_PADDING_FILE): MKIMAGE_HDR_SIZE=512
$(TEE_TEMP_PADDING_FILE): RSA_SIGN_HDR_SIZE=576
$(TEE_TEMP_PADDING_FILE): $(TEE_RAW_IMAGE_NAME) $(TEE_ADDITIONAL_DEPENDENCIES)
	@echo TEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) rm -f $@
	$(hide) FILE_SIZE=$$(($$(wc -c < "$(TEE_RAW_IMAGE_NAME)")+$(MKIMAGE_HDR_SIZE)+$(RSA_SIGN_HDR_SIZE)));\
	REMAINDER=$$(($${FILE_SIZE} % $(ALIGNMENT)));\
	if [ $${REMAINDER} -ne 0 ]; then dd if=/dev/zero of=$@ bs=$$(($(ALIGNMENT)-$${REMAINDER})) count=1; else touch $@; fi

$(TEE_TEMP_CFG_FILE): $(TEE_DRAM_SIZE_CFG) $(TEE_ADDITIONAL_DEPENDENCIES)
	@echo TEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) rm -f $@
	@echo "LOAD_MODE = 0" > $@
	@echo "NAME = tee" >> $@
	@echo "LOAD_ADDR =" $(TEE_TOTAL_DRAM_SIZE) >> $@

$(TEE_PADDING_IMAGE_NAME): $(TEE_RAW_IMAGE_NAME) $(TEE_TEMP_PADDING_FILE) $(TEE_ADDITIONAL_DEPENDENCIES)
	@echo TEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) cat $(TEE_RAW_IMAGE_NAME) $(TEE_TEMP_PADDING_FILE) > $@

$(TEE_SIGNED_IMAGE_NAME): ALIGNMENT=512
$(TEE_SIGNED_IMAGE_NAME): $(TEE_PADDING_IMAGE_NAME) $(TRUSTZONE_SIGN_TOOL) $(TRUSTZONE_IMG_PROTECT_CFG) $(TEE_DRAM_SIZE_CFG) $(TEE_ADDITIONAL_DEPENDENCIES)
	@echo TEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) $(TRUSTZONE_SIGN_TOOL) $(TRUSTZONE_IMG_PROTECT_CFG) $(TEE_PADDING_IMAGE_NAME) $@ $(TEE_DRAM_SIZE)
	$(hide) FILE_SIZE=$$(wc -c < "$(TEE_SIGNED_IMAGE_NAME)");REMAINDER=$$(($${FILE_SIZE} % $(ALIGNMENT)));\
	if [ $${REMAINDER} -ne 0 ]; then echo "[ERROR] File $@ size $${FILE_SIZE} is not $(ALIGNMENT) bytes aligned";exit 1; fi

$(TEE_COMP_IMAGE_NAME): ALIGNMENT=512
$(TEE_COMP_IMAGE_NAME): $(TEE_SIGNED_IMAGE_NAME) $(MTK_MKIMAGE_TOOL) $(TEE_TEMP_CFG_FILE) $(TEE_ADDITIONAL_DEPENDENCIES)
	@echo TEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) $(MTK_MKIMAGE_TOOL) $(TEE_SIGNED_IMAGE_NAME) $(TEE_TEMP_CFG_FILE) > $@
	$(hide) FILE_SIZE=$$(stat -c%s "$(TEE_COMP_IMAGE_NAME)");REMAINDER=$$(($${FILE_SIZE} % $(ALIGNMENT)));\
	if [ $${REMAINDER} -ne 0 ]; then echo "[ERROR] File $@ size $${FILE_SIZE} is not $(ALIGNMENT) bytes aligned";exit 1; fi

