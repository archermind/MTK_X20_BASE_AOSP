# vendor/mediatek/proprietary/trustzone/Android.mk
LOCAL_PATH := $(call my-dir)
TRUSTZONE_ROOT_DIR := $(PWD)
TRUSTZONE_CUSTOM_BUILD_PATH := $(LOCAL_PATH)
TRUSTZONE_OUTPUT_PATH := $(PRODUCT_OUT)/trustzone
TRUSTZONE_IMAGE_OUTPUT_PATH := $(TRUSTZONE_OUTPUT_PATH)

$(info MTK_ATF_SUPPORT = $(MTK_ATF_SUPPORT))
$(info MTK_TEE_SUPPORT = $(MTK_TEE_SUPPORT))
$(info MTK_IN_HOUSE_TEE_SUPPORT = $(MTK_IN_HOUSE_TEE_SUPPORT))
$(info TRUSTONIC_TEE_SUPPORT = $(TRUSTONIC_TEE_SUPPORT))
$(info MICROTRUST_TEE_SUPPORT = $(MICROTRUST_TEE_SUPPORT))
$(info MTK_GOOGLE_TRUSTY_SUPPORT = $(MTK_GOOGLE_TRUSTY_SUPPORT))

ifeq ($(strip $(MTK_IN_HOUSE_TEE_SUPPORT)),yes)
  ifneq ($(wildcard vendor/mediatek/proprietary/trustzone/mtee/protect/Android.mk),)
    # source release
    # include vendor/mediatek/proprietary/trustzone/mtee/protect/Android.mk
    INSTALLED_TRUSTZONE_TARGET := $(PRODUCT_OUT)/tz.img
    BUILT_TRUSTZONE_TARGET := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/tz.img
  else
    # binary release
    INSTALLED_TRUSTZONE_TARGET := $(PRODUCT_OUT)/tz.img
    PREBUILT_TRUSTZONE_TARGET := $(PRODUCT_OUT)/tz.img
  endif
else
  ifneq ($(filter yes,$(MTK_ATF_SUPPORT) $(TRUSTONIC_TEE_SUPPORT) $(MICROTRUST_TEE_SUPPORT) $(MTK_GOOGLE_TRUSTY_SUPPORT)),)
    INSTALLED_TRUSTZONE_TARGET := $(PRODUCT_OUT)/trustzone.bin
    BUILT_TRUSTZONE_TARGET := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/trustzone.bin
  endif
endif

.PHONY: trustzone
ifneq ($(PREBUILT_TRUSTZONE_TARGET),)

else ifneq ($(BUILT_TRUSTZONE_TARGET),)

  # ATF
  ifeq ($(strip $(MTK_ATF_SUPPORT)),yes)
include $(TRUSTZONE_CUSTOM_BUILD_PATH)/atf_config.mk
$(BUILT_TRUSTZONE_TARGET): $(ATF_COMP_IMAGE_NAME)
  endif
  # MTEE
  ifeq ($(strip $(MTK_IN_HOUSE_TEE_SUPPORT)),yes)
  ifeq ($(strip $(MTK_TEE_SUPPORT)),yes)
include $(TRUSTZONE_CUSTOM_BUILD_PATH)/mtee_config.mk
$(BUILT_TRUSTZONE_TARGET): $(MTEE_COMP_IMAGE_NAME)
  else
MTEE_INTERMEDIATES_IMAGE_NAME := $(PRODUCT_OUT)/obj/EXECUTABLES/tz.img_intermediates/tz.img
$(BUILT_TRUSTZONE_TARGET): $(MTEE_INTERMEDIATES_IMAGE_NAME)
  endif
  endif
  #ifeq ($(MTK_TEE_SUPPORT),yes)
  # Trustonic
  ifeq ($(strip $(TRUSTONIC_TEE_SUPPORT)),yes)
include $(TRUSTZONE_CUSTOM_BUILD_PATH)/tee_config.mk
$(BUILT_TRUSTZONE_TARGET): $(TEE_COMP_IMAGE_NAME)
#trustzone: mcDriverDaemon
$(PRODUCT_OUT)/recovery.img: \
	$(TEE_modules_to_install) \
	$(call intermediates-dir-for,EXECUTABLES,mcDriverDaemon)/mcDriverDaemon
  endif
  # Microtrust
  ifeq ($(strip $(MICROTRUST_TEE_SUPPORT)),yes)
include $(TRUSTZONE_CUSTOM_BUILD_PATH)/microtrust_config.mk
$(BUILT_TRUSTZONE_TARGET): $(MICROTRUST_COMP_IMAGE_NAME)
  endif
  # Trusty
  ifeq ($(strip $(MTK_GOOGLE_TRUSTY_SUPPORT)),yes)
include $(TRUSTZONE_CUSTOM_BUILD_PATH)/trusty_config.mk
$(BUILT_TRUSTZONE_TARGET): $(TRUSTY_COMP_IMAGE_NAME)
  endif
  #endif
  # trustzone.bin
$(BUILT_TRUSTZONE_TARGET):
	@echo "Trustzone build: $@ <= $^"
	$(hide) mkdir -p $(dir $@)
	$(hide) cat $^ > $@

$(INSTALLED_TRUSTZONE_TARGET): $(BUILT_TRUSTZONE_TARGET) $(LOCAL_PATH)/Android.mk
	@echo Copying: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) cp -f $< $@

endif

ifneq ($(INSTALLED_TRUSTZONE_TARGET),)
$(TARGET_OUT_ETC)/trustzone.bin: $(INSTALLED_TRUSTZONE_TARGET) $(LOCAL_PATH)/Android.mk
	@echo Copying: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) cp -f $< $@

trustzone: $(INSTALLED_TRUSTZONE_TARGET) $(TEE_modules_to_install) $(TEE_modules_to_check) $(TARGET_OUT_ETC)/trustzone.bin
ALL_DEFAULT_INSTALLED_MODULES += $(INSTALLED_TRUSTZONE_TARGET) $(TEE_modules_to_install) $(TEE_modules_to_check) $(TARGET_OUT_ETC)/trustzone.bin

endif
