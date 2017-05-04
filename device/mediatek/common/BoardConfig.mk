#this is platform common Boardconfig

# Use the non-open-source part, if present
-include vendor/mediatek/common/BoardConfigVendor.mk

# Use the connectivity Boardconfig
include device/mediatek/common/connectivity/BoardConfig.mk

# for flavor build base project assignment
ifeq ($(strip $(MTK_BASE_PROJECT)),)
  MTK_PROJECT_NAME := $(subst full_,,$(TARGET_PRODUCT))
else
  MTK_PROJECT_NAME := $(MTK_BASE_PROJECT)
endif
MTK_PROJECT := $(MTK_PROJECT_NAME)
MTK_PATH_SOURCE := vendor/mediatek/proprietary
MTK_ROOT := vendor/mediatek/proprietary
MTK_PATH_COMMON := vendor/mediatek/proprietary/custom/common
MTK_PATH_CUSTOM := vendor/mediatek/proprietary/custom/$(MTK_PROJECT)
MTK_PATH_CUSTOM_PLATFORM := vendor/mediatek/proprietary/custom/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')
KERNEL_CROSS_COMPILE:= $(abspath $(TOP))/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
TARGET_BOARD_KERNEL_HEADERS := device/mediatek/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')/kernel-headers \
                               device/mediatek/common/kernel-headers

MTK_HAL_CFLAGS += -I$(TOPDIR)vendor/mediatek/proprietary/hardware/audio/common/include
COMMON_GLOBAL_CFLAGS += $(MTK_HAL_CFLAGS)
MTK_CGEN_CFLAGS += -I$(MTK_PATH_CUSTOM)/cgen/cfgdefault  -I$(MTK_PATH_CUSTOM)/cgen/cfgfileinc -I$(MTK_PATH_CUSTOM)/cgen/inc -I$(MTK_PATH_CUSTOM)/cgen
MTK_CGEN_CFLAGS += -I$(MTK_PATH_CUSTOM_PLATFORM)/cgen/cfgdefault  -I$(MTK_PATH_CUSTOM_PLATFORM)/cgen/cfgfileinc -I$(MTK_PATH_CUSTOM_PLATFORM)/cgen/inc -I$(MTK_PATH_CUSTOM_PLATFORM)/cgen
MTK_CGEN_CFLAGS += -I$(MTK_PATH_COMMON)/cgen/cfgdefault  -I$(MTK_PATH_COMMON)/cgen/cfgfileinc -I$(MTK_PATH_COMMON)/cgen/inc -I$(MTK_PATH_COMMON)/cgen

COMMON_GLOBAL_CFLAGS += $(MTK_CGEN_CFLAGS)
COMMON_GLOBAL_CPPFLAGS += $(MTK_CGEN_CFLAGS)

# Add MTK compile options to wrap MTK's modifications on AOSP.
ifneq ($(strip $(MTK_BASIC_PACKAGE)),yes)
ifneq ($(strip $(MTK_EMULATOR_SUPPORT)),yes)
  COMMON_GLOBAL_CFLAGS += -DMTK_AOSP_ENHANCEMENT
  COMMON_GLOBAL_CPPFLAGS += -DMTK_AOSP_ENHANCEMENT
endif
endif

#MTK_PLATFORM := $(shell echo $(MTK_PROJECT_NAME) | awk -F "_" {'print $$1'})
MTK_PATH_PLATFORM := $(MTK_PATH_SOURCE)/platform/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')
MTK_PATH_KERNEL := kernel
GOOGLE_RELEASE_RIL := no
BUILD_NUMBER := $(shell date +%s)

#Enable HWUI by default
USE_OPENGL_RENDERER := true

#SELinux Policy File Configuration
BOARD_SEPOLICY_DIRS := \
        device/mediatek/common/sepolicy

# Include an expanded selection of fonts
EXTENDED_FONT_FOOTPRINT := true

# ptgen
# Add MTK's MTK_PTGEN_OUT definitions
ifeq (,$(strip $(OUT_DIR)))
  ifeq (,$(strip $(OUT_DIR_COMMON_BASE)))
    MTK_PTGEN_OUT_DIR := $(TOPDIR)out
  else
    MTK_PTGEN_OUT_DIR := $(OUT_DIR_COMMON_BASE)/$(notdir $(PWD))
  endif
else
    MTK_PTGEN_OUT_DIR := $(strip $(OUT_DIR))
endif
MTK_PTGEN_PRODUCT_OUT := $(MTK_PTGEN_OUT_DIR)/target/product/$(MTK_TARGET_PROJECT)
MTK_PTGEN_OUT := $(MTK_PTGEN_OUT_DIR)/target/product/$(MTK_TARGET_PROJECT)/obj/PTGEN
MTK_PTGEN_MK_OUT := $(MTK_PTGEN_OUT_DIR)/target/product/$(MTK_TARGET_PROJECT)/obj/PTGEN
MTK_PTGEN_TMP_OUT := $(MTK_PTGEN_OUT_DIR)/target/product/$(MTK_TARGET_PROJECT)/obj/PTGEN_TMP

#disable kernel config sync check
ifeq ($(MTK_BASIC_PACKAGE),yes)
DISABLE_MTK_CONFIG_CHECK = yes
endif

#Add MTK's hook
-include device/mediatek/build/build/tools/base_rule_hook.mk

