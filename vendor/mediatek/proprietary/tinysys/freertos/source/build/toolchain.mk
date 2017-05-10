###########################################################
## Android root directory determination
###########################################################
# By default, use ANDROID_BUILD_TOP defined in build/envsetup.sh if the user
# had included it. Otherwise find Android root directory by ourselves.
ANDROID_ROOT_DIR  := $(ANDROID_BUILD_TOP)

ifeq ($(ANDROID_ROOT_DIR),)
  ANDROID_ROOT_DIR := $(call find_Android_root_dir,$(SOURCE_DIR))
endif
ANDROID_ROOT_DIR := $(strip $(ANDROID_ROOT_DIR))

# ANDROID_ROOT_DIR may be empty if we are not in Android environment. This is
# possible if user only has tinysys source codes only.

###########################################################
## Toolchain
###########################################################
CC      := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE    := $(CROSS_COMPILE)size

ifneq ($(ANDROID_ROOT_DIR),)
  TINYSYS_TOOLCHAIN_HOST   := linux-x86
  TINYSYS_TOOLCHAIN_STEM   := gcc-arm-none-eabi-4_8-2014q3
  TINYSYS_TOOLCHAIN_PREFIX ?= arm-none-eabi-

  TINYSYS_TOOLCHAIN_BIN_PATH := $(ANDROID_ROOT_DIR)/prebuilts/gcc/$(TINYSYS_TOOLCHAIN_HOST)/arm/$(TINYSYS_TOOLCHAIN_STEM)/bin

  my_tinysys_cc := $(TINYSYS_TOOLCHAIN_BIN_PATH)/$(TINYSYS_TOOLCHAIN_PREFIX)gcc
  ifneq ($(wildcard $(my_tinysys_cc)),)
    CC      := $(my_tinysys_cc)
    OBJCOPY := $(TINYSYS_TOOLCHAIN_BIN_PATH)/$(TINYSYS_TOOLCHAIN_PREFIX)objcopy
    SIZE    := $(TINYSYS_TOOLCHAIN_BIN_PATH)/$(TINYSYS_TOOLCHAIN_PREFIX)size
  endif
endif # ifneq ($(ANDROID_ROOT_DIR),)

ifeq (1,$(V))
  $(info $(TINYSYS_SCP): CC=$(CC))
  $(info $(TINYSYS_SCP): OBJCOPY=$(OBJCOPY))
  $(info $(TINYSYS_SCP): SIZE=$(SIZE))
endif
