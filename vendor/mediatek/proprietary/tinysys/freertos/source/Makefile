###########################################################
## Generic definitions
###########################################################
# Remove $@ if error occurs
.DELETE_ON_ERROR:

# Turn off suffix build rules built into make
.SUFFIXES:

.PHONY: FORCE
FORCE:

SHELL          := /bin/bash
.DEFAULT_GOAL  := all

TINYSYS_SCP    := tinysys-scp
TINYSYS_LOADER := tinysys-loader

###########################################################
## Parameter control
###########################################################
PROJECT := $(strip $(PROJECT))
ifeq ($(PROJECT),)
  $(error $(TINYSYS_SCP): project name is required)
endif

ifeq ($(strip $(O)),)
O := $(TINYSYS_SCP)_out
endif

INSTALLED_DIR ?= $(O)

# Verbosity control
# Add 'V=1' with make or 'showcommands' when building Android
V ?= 1
ifeq ($(V),1)
hide :=
else
hide := @
endif

###########################################################
## Common directory locations and generic variables
###########################################################
SOURCE_DIR         := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
BUILT_DIR          := $(O)/freertos/source

DRIVERS_DIR        := $(SOURCE_DIR)/drivers
DRIVERS_COMMON_DIR := $(DRIVERS_DIR)/common
BUILD_DIR          := $(SOURCE_DIR)/build
TOOLS_DIR          := $(SOURCE_DIR)/tools
RTOS_SRC_DIR       := $(SOURCE_DIR)/kernel/FreeRTOS/Source
BASE_DIR           := $(SOURCE_DIR)/project
TINYSYS_SCP_BIN    := $(INSTALLED_DIR)/$(TINYSYS_SCP).bin
TINYSYS_SECURE_DIR := $(SOURCE_DIR)/../secure

MKIMAGE            := $(TOOLS_DIR)/mkimage
OBJSIZE            := $(TOOLS_DIR)/objsize

# Common functions and utilities
include $(BUILD_DIR)/definitions.mk
include $(BUILD_DIR)/toolchain.mk

# Initialize the environment for each processor
include $(BUILD_DIR)/main.mk

ifeq (1,$(V))
  $(info $(TINYSYS_SCP): PROCESSORS=$(PROCESSORS))
  $(info $(TINYSYS_SCP): PROJECT=$(PROJECT))
  $(info $(TINYSYS_SCP): PLATFORM=$(PLATFORM))
  $(info $(TINYSYS_SCP): O=$(O))
  $(info $(TINYSYS_SCP): SOURCE_DIR=$(SOURCE_DIR))
  $(info $(TINYSYS_SCP): ALL_SCP_BINS=$(ALL_SCP_BINS))
endif

###########################################################
## Build targets
###########################################################
all: $(TINYSYS_SCP_BIN) ;

SORTED_SCP_BINS := $(call sort_tinysys_binaries,$(ALL_SCP_BINS))

$(TINYSYS_SCP_BIN): $(SORTED_SCP_BINS)
	@mkdir -p $(dir $@)
	@echo "$(TINYSYS_SCP): Generating $@ from:"; \
	for i in $^; do echo "  * $${i}"; done; \
	cat $^ > $@

clean:
	rm -rf $(TINYSYS_SCP_BIN) $(O)
