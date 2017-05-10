ifeq (yes,$(MTK_TINYSYS_SCP_SUPPORT))
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

TINYSYS_SCP := tinysys-scp
TINYSYS_TARGET_FILE := $(TINYSYS_SCP).bin
TINYSYS_CLEAN_MODULE := clean-$(TINYSYS_SCP)
TINYSYS_SCP_CLASS := TINYSYS_OBJ
TINYSYS_BUILT_INTERMEDIATES := \
	$(call intermediates-dir-for,$(TINYSYS_SCP_CLASS),$(TINYSYS_SCP))
TINYSYS_INSTALLED_TARGET := $(PRODUCT_OUT)/$(TINYSYS_TARGET_FILE)

TINYSYS_SCP_BUILD_CMD := \
	PROJECT=$(MTK_TARGET_PROJECT) \
	O=$(PWD)/$(TINYSYS_BUILT_INTERMEDIATES) \
	INSTALLED_DIR=$(PWD)/$(PRODUCT_OUT) \
	$(if $(filter showcommands,$(MAKECMDGOALS)),V=1) \
	BUILD_TYPE=$(if $(filter eng,$(TARGET_BUILD_VARIANT)),debug,release) \
	$(MAKE) -C $(LOCAL_PATH)

###########################################################
# Config header targets
###########################################################
TINYSYS_CONFIG_HEADER := tinysys-configheader

# Config option consistency check mechanism
check-tinysys-config: $(TINYSYS_CONFIG_HEADER)
ifneq (yes,$(strip $(DISABLE_MTK_CONFIG_CHECK)))
	python device/mediatek/build/build/tools/check_kernel_config.py --prjconfig $(MTK_TARGET_PROJECT_FOLDER)/ProjectConfig.mk --project $(MTK_TARGET_PROJECT) --header `find $(TINYSYS_BUILT_INTERMEDIATES) -type f -name tinysys_config.h | tr "\n" "," | sed -e "s/,$$//"`
else
	-python device/mediatek/build/build/tools/check_kernel_config.py --prjconfig $(MTK_TARGET_PROJECT_FOLDER)/ProjectConfig.mk --project $(MTK_TARGET_PROJECT) --header `find $(TINYSYS_BUILT_INTERMEDIATES) -type f -name tinysys_config.h | tr "\n" "," | sed -e "s/,$$//"`
endif

.PHONY: $(TINYSYS_CONFIG_HEADER)
$(TINYSYS_CONFIG_HEADER):
	+$(TINYSYS_SCP_BUILD_CMD) configheader

check-mtk-config: check-tinysys-config

###########################################################
# Main targets
###########################################################
.PHONY: $(TINYSYS_SCP)
$(TINYSYS_SCP): $(TINYSYS_INSTALLED_TARGET) ;

$(TINYSYS_INSTALLED_TARGET): check-tinysys-config
	+$(TINYSYS_SCP_BUILD_CMD)

# Ensure full build dependency
droid: $(TINYSYS_INSTALLED_TARGET)

###########################################################
# Clean targets
###########################################################
.PHONY: $(TINYSYS_CLEAN_MODULE)
$(TINYSYS_CLEAN_MODULE):
	@echo "Clean: $(TINYSYS_SCP)"
	+$(TINYSYS_SCP_BUILD_CMD) clean

###########################################################
# Collect NOTICE files
###########################################################
TINYSYS_NOTICE_FILES := \
  $(shell find $(LOCAL_PATH) -type f -name '*NOTICE*.txt' -printf '%P\n')
TINYSYS_NOTICE_FILES_INSTALLED := \
  $(TINYSYS_NOTICE_FILES:%=$(TARGET_OUT_NOTICE_FILES)/src/tinysys/%)

$(TARGET_OUT_INTERMEDIATES)/NOTICE.html: $(TINYSYS_NOTICE_FILES_INSTALLED)

$(TINYSYS_NOTICE_FILES_INSTALLED): \
  $(TARGET_OUT_NOTICE_FILES)/src/tinysys/%: $(LOCAL_PATH)/% | $(ACP)
	@echo Copying: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) $(ACP) $< $@

endif # $(MTK_TINYSYS_SCP_SUPPORT) is yes
