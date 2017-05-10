###########################################################
## Obtain processor and platform name by project name
###########################################################
my_project_paths := $(strip $(shell find $(BASE_DIR) -maxdepth 3 \
  -type d -name $(PROJECT) -printf '%P\n'))
ifeq ($(my_project_paths),)
  $(error $(TINYSYS_SCP): Cannot find project $(PROJECT) under $(BASE_DIR))
endif

my_processor_and_platform_names :=
$(foreach p,$(my_project_paths), \
  $(eval my_processor_and_platform_names += \
    $(call get_processor_and_platform,$(p))) \
)
$(foreach i,$(my_processor_and_platform_names), \
  $(eval PROCESSORS += $(strip $(word 1,$(subst :, ,$(i))))) \
  $(eval PLATFORM  += $(strip $(word 2,$(subst :, ,$(i))))) \
)

PROCESSORS := $(sort $(PROCESSORS))
PLATFORM   := $(sort $(PLATFORM))
ifneq (1,$(words $(PLATFORM)))
  $(error $(TINYSYS_SCP): $(PROJECT) is found in platforms [$(PLATFORM)], but one project cannot belong to multiple platforms. Please choose different project names for different platform.)
endif

ifeq (1,$(V))
  $(info $(TINYSYS_SCP): PROCESSORS=$(PROCESSORS))
  $(info $(TINYSYS_SCP): PLATFORM=$(PLATFORM))
endif

###########################################################
## Initialize environment and targets for each processor
###########################################################
$(foreach p,$(PROCESSORS), \
  $(eval PROCESSOR := $(p)) \
  $(eval include $(BUILD_DIR)/config.mk) \
)

my_project_path :=
my_project :=
my_processor_and_platform_names :=
