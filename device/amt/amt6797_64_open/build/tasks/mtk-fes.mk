# fastboot empty storage
ifeq ($(strip $(PLATFORM_FASTBOOT_EMPTY_STORAGE)),yes)
ifeq ($(strip $(MTK_FES_COPY_FILES)),)

MTK_FES_COPY_FILES := \
    $(PRODUCT_OUT)/$(MTK_PTGEN_CHIP)_Android_scatter.txt\
    $(INSTALLED_LK_TARGET)\
    $(INSTALLED_LOGO_TARGET)\
    $(INSTALLED_TRUSTZONE_TARGET)

ifdef PRELOADER_TARGET_PRODUCT
MTK_FES_COPY_FILES += \
    $(PRODUCT_OUT)/preloader_$(PRELOADER_TARGET_PRODUCT).bin
endif

MTK_FES_COPY_PATH := $(PRODUCT_OUT)/FES

MTK_FES_INSTALLED_FILES :=
$(foreach src,$(MTK_FES_COPY_FILES),\
    $(eval dst := $(MTK_FES_COPY_PATH)/$(notdir $(src)))\
    $(eval $(call copy-one-file,$(src),$(dst)))\
    $(eval MTK_FES_INSTALLED_FILES += $(dst))\
)
mtk-fes: $(MTK_FES_INSTALLED_FILES)
droidcore: mtk-fes

endif
endif
