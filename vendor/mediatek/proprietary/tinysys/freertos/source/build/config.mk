include $(BUILD_DIR)/clear_vars.mk

###########################################################
## Create processor-based environment and targets
###########################################################
PROCESSOR_DIR                  := $(BASE_DIR)/$(PROCESSOR)
PLATFORM_BASE_DIR              := $(PROCESSOR_DIR)/$(PLATFORM)
PLATFORM_DIR                   := $(PLATFORM_BASE_DIR)/platform
PROJECT_DIR                    := $(PLATFORM_BASE_DIR)/$(PROJECT)
MY_BUILT_DIR                   := $(BUILT_DIR)/$(PROCESSOR)
DRIVERS_PLATFORM_DIR           := $(DRIVERS_DIR)/$(PROCESSOR)/$(PLATFORM)
GEN_INCLUDE_DIR                := $(MY_BUILT_DIR)/include/generated
$(PROCESSOR).TINYSYS_CONFIG_H  := $(GEN_INCLUDE_DIR)/tinysys_config.h
$(PROCESSOR).TINYSYS_BIN_BUILT := \
  $(MY_BUILT_DIR)/$(TINYSYS_SCP)-$(PROCESSOR).bin

MY_BIN_STEM              := $(basename $($(PROCESSOR).TINYSYS_BIN_BUILT))
$(PROCESSOR).BIN_NH      := $(MY_BIN_STEM)-no-mtk-header.bin
$(PROCESSOR).ELF_FILE    := $(MY_BIN_STEM).elf
$(PROCESSOR).MAP_FILE    := $(MY_BIN_STEM).map
$(PROCESSOR).HEX_FILE    := $(MY_BIN_STEM).hex
$(PROCESSOR).IMG_HDR_CFG := $(MY_BUILT_DIR)/img_hdr_$(notdir $(MY_BIN_STEM)).cfg

ALL_SCP_BINS := $(ALL_SCP_BINS) $($(PROCESSOR).TINYSYS_BIN_BUILT)

#ifeq (1,$(V))
#  $(info $(TINYSYS_SCP): PROCESSOR=$(PROCESSOR))
#  $(info $(TINYSYS_SCP): PROCESSOR_DIR=$(PROCESSOR_DIR))
#  $(info $(TINYSYS_SCP): PLATFORM_BASE_DIR=$(PLATFORM_BASE_DIR))
#  $(info $(TINYSYS_SCP): PLATFORM_DIR=$(PLATFORM_DIR))
#  $(info $(TINYSYS_SCP): PROJECT_DIR=$(PROJECT_DIR))
#  $(info $(TINYSYS_SCP): MY_BUILT_DIR=$(MY_BUILT_DIR))
#  $(info $(TINYSYS_SCP): MY_TINYSYS_BIN=$(MY_TINYSYS_BIN))
#  $(info $(TINYSYS_SCP): $(PROCESSOR).TINYSYS_BIN_BUILT=$($(PROCESSOR).TINYSYS_BIN_BUILT))
#  $(info $(TINYSYS_SCP): $(PROCESSOR).BIN_NH=$($(PROCESSOR).BIN_NH))
#  $(info $(TINYSYS_SCP): $(PROCESSOR).ELF_FILE=$($(PROCESSOR).ELF_FILE))
#  $(info $(TINYSYS_SCP): $(PROCESSOR).MAP_FILE=$($(PROCESSOR).MAP_FILE))
#  $(info $(TINYSYS_SCP): $(PROCESSOR).HEX_FILE=$($(PROCESSOR).HEX_FILE))
#endif

PLATFORM_MK := $(PLATFORM_DIR)/platform.mk
ifeq ($(wildcard $(PLATFORM_MK)),)
  $(error $(PLATFORM_MK) is missing)
endif

include $(BUILD_DIR)/common.mk
include $(PLATFORM_MK)
include $(BUILD_DIR)/loader.mk

LDFLAGS  += -Wl,-T$(PLATFORM_DIR)/system.ld
INCLUDES += -I$(GEN_INCLUDE_DIR)

# Include project-specific files only when available
ifneq ($(wildcard $(PROJECT_DIR)/inc),)
  INCLUDES += -I$(PROJECT_DIR)/inc
endif
ifneq ($(wildcard $(PROJECT_DIR)/src/project.c),)
  C_FILES  += $(wildcard $(PROJECT_DIR)/src/project.c)
endif

C_OBJS   := $(sort $(C_FILES:%.c=$(MY_BUILT_DIR)/%.o))
S_OBJS   := $(sort $(S_FILES:%.S=$(MY_BUILT_DIR)/%.o))
OBJS     += $(sort $(C_OBJS) $(S_OBJS))
DEPS     += $(MAKEFILE_LIST)
$(OBJS): $($(PROCESSOR).TINYSYS_CONFIG_H)

# Stash the list of configuration names and values to generate config header
CONFIG_MK_FILES := $(PLATFORM_MK) $(wildcard $(PROJECT_DIR)/ProjectConfig.mk)
$(call stash_config_options,$(CONFIG_MK_FILES))

###########################################################
## Create processor-based build targets
###########################################################
$($(PROCESSOR).TINYSYS_BIN_BUILT): PRIVATE_BIN_NH := $($(PROCESSOR).BIN_NH)
$($(PROCESSOR).TINYSYS_BIN_BUILT): PRIVATE_HEX_FILE := $($(PROCESSOR).HEX_FILE)
$($(PROCESSOR).TINYSYS_BIN_BUILT): PRIVATE_BUILT_DIR := $(MY_BUILT_DIR)
$($(PROCESSOR).TINYSYS_BIN_BUILT): PRIVATE_IMG_HDR_CFG := $($(PROCESSOR).IMG_HDR_CFG)
$($(PROCESSOR).TINYSYS_BIN_BUILT): PRIVATE_ELF_FILE := $($(PROCESSOR).ELF_FILE)
$($(PROCESSOR).TINYSYS_BIN_BUILT): PRIVATE_PLATFORM_DIR := $(PLATFORM_DIR)
$($(PROCESSOR).TINYSYS_BIN_BUILT): PRIVATE_PROJECT_DIR := $(PROJECT_DIR)

$($(PROCESSOR).TINYSYS_BIN_BUILT): \
  $($(PROCESSOR).ELF_FILE) $($(PROCESSOR).IMG_HDR_CFG) | $(OBJSIZE) $(MKIMAGE)
	$(hide)$(OBJCOPY) -O ihex $(PRIVATE_ELF_FILE) $(PRIVATE_HEX_FILE)
	$(hide)$(OBJCOPY) -O binary $(PRIVATE_ELF_FILE) $(PRIVATE_BIN_NH)
	$(hide)$(MKIMAGE) $(PRIVATE_BIN_NH) $(PRIVATE_IMG_HDR_CFG) > $@
ifeq (1,$(V))
	$(hide)$(SIZE) $(PRIVATE_ELF_FILE)
	$(hide)$(OBJSIZE) $(PRIVATE_BUILT_DIR)/$(RTOS_SRC_DIR)
	$(hide)$(OBJSIZE) $(PRIVATE_BUILT_DIR)/$(PRIVATE_PLATFORM_DIR)
	$(hide)$(OBJSIZE) $(PRIVATE_BUILT_DIR)/$(PRIVATE_PROJECT_DIR)
endif

$($(PROCESSOR).ELF_FILE): PRIVATE_MAP_FILE := $($(PROCESSOR).MAP_FILE)
$($(PROCESSOR).ELF_FILE): PRIVATE_LDFLAGS := $(LDFLAGS)
$($(PROCESSOR).ELF_FILE): PRIVATE_LIBFLAGS := $(LIBFLAGS)
$($(PROCESSOR).ELF_FILE): PRIVATE_OBJS := $(OBJS)
$($(PROCESSOR).ELF_FILE): $(OBJS)
	$(hide)$(CC) $(PRIVATE_LDFLAGS) $(PRIVATE_OBJS) -Wl,-Map=$(PRIVATE_MAP_FILE) -o $@ -Wl,--start-group $(PRIVATE_LIBFLAGS) -Wl,--end-group

$(C_OBJS): PRIVATE_CC := $(CC)
$(C_OBJS): PRIVATE_CFLAGS := $(CFLAGS)
$(C_OBJS): PRIVATE_INCLUDES := $(INCLUDES)
$(C_OBJS): $(MY_BUILT_DIR)/%.o: %.c
	$(compile-c-or-s-to-o)

$(S_OBJS): PRIVATE_CC := $(CC)
$(S_OBJS): PRIVATE_CFLAGS := $(CFLAGS)
$(S_OBJS): PRIVATE_INCLUDES := $(INCLUDES)
$(S_OBJS): $(MY_BUILT_DIR)/%.o: %.S
	$(compile-c-or-s-to-o)

$($(PROCESSOR).IMG_HDR_CFG): $(DEPS)
	$(call gen-image-header,$(TINYSYS_SCP))

# Generate header file that contains all config options and its values
.PHONY: configheader
configheader: $($(PROCESSOR).TINYSYS_CONFIG_H)

$($(PROCESSOR).TINYSYS_CONFIG_H): PRIVATE_PROCESSOR := $(PROCESSOR)
# Let target header file depends on FORCE to ensure that for each make the
# config option updater mechanism kicks off. Target header file will not
# be updated if config option are not changed.
$($(PROCESSOR).TINYSYS_CONFIG_H): FORCE
	$(call gen-tinysys-header,\
		__TINYSYS_CONFIG_H, \
		$($(PRIVATE_PROCESSOR).CONFIG_OPTIONS), \
		$(TINYSYS_SCP) \
	)

-include $(OBJS:.o=.d)
$(OBJS): $(DEPS)
