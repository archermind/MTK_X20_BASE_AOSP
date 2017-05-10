QUOTE = "
SQUOTE = '
PERCENT := %

###########################################################
## Look up the root directory of Android code base.
## Return the path if found, otherwise empty
###########################################################

define find_Android_root_dir
$(shell \
	p=$(1); \
	( \
	while [ $${p} != '/' ]; do \
	[ -f 'build/envsetup.sh' ] && echo $${p} && break; \
	cd ..; \
	p=$${PWD}; \
	done \
	) \
)
endef

###########################################################
## Get processor and platform names by project path.
## Return $(PROCESSOR):$(PLATFORM) string.
## Project path format: $(PROCESSOR)/$(PLATFORM)/$(PROJECT)
###########################################################

# $(1): path of project directory
define get_processor_and_platform
$(eval my_path_words := $(subst /, ,$(1))) \
$(if $(filter 3,$(words $(my_path_words))), \
  $(word 1,$(my_path_words)):$(word 2,$(my_path_words)), \
  $(error Project path layout ($(1)) is incorrect) \
)
endef

###########################################################
## Get processor(s) by platform path.
## Return $(PROCESSOR) string.
## Platform path format: $(PROCESSOR)/$(PLATFORM)
###########################################################

# $(1): path of platform directory
define get_processors_from_platform_paths
$(foreach p,$(1),\
  $(eval my_path_words := $(subst /, ,$(p))) \
  $(word 1,$(my_path_words)) \
) \
$(eval my_path_words :=)
endef

###########################################################
## Create the variable $(PROCESSOR).CONFIG_OPTIONS that
## contains $(CFG_NAME)=$($(CFG_NAME)) pairs from
## given configuration file list for each processor.
##
## The purpose is to collect all config options from given
## config files, which belong to different processors, and
## stash their final values.
###########################################################

# $1: List of configuration files that registers config options
define stash_config_options
$(eval _vars :=) \
$(foreach f,$(strip $(1)), \
  $(eval _vars += $(shell sed -nr 's#^[[:space:]]*(CFG_[0-9A-Z_]+)[ \t]*[:+?]?=.*#\1#gp' $(f))) \
) \
$(eval $(PROCESSOR).CONFIG_OPTIONS :=) \
$(foreach v,$(_vars), \
  $(if $($(v)),,$(eval $(v) := "")) \
  $(eval $(v) := $(subst $(QUOTE),\$(QUOTE),$($(v)))) \
  $(eval $(v) := $(subst $(SQUOTE),\$(SQUOTE),$($(v)))) \
  $(eval $(PROCESSOR).CONFIG_OPTIONS += $(strip $(v))=$(strip $($(v)))) \
) \
$(eval $(PROCESSOR).CONFIG_OPTIONS := $(sort $($(PROCESSOR).CONFIG_OPTIONS))) \
$(eval _vars :=)
endef

###########################################################
## Sort required Tiny System intermediate binaries and
## print the result.
## For each processor, loader must precede tinysys binary.
###########################################################

# $(1): the list of unordered binary file paths
define sort_tinysys_binaries
$(eval _arg := $(strip $(1))) \
$(if $(_arg),, \
  $(error $(TINYSYS_SCP): sort_tinysys_binaries: argument missing)) \
$(eval SORTED_TINYSYS_DEPS :=) \
$(foreach p,$(PROCESSORS), \
  $(eval m := $(filter %/$(TINYSYS_LOADER)-$(p).bin,$(_arg))) \
  $(if $(m), \
    $(eval SORTED_TINYSYS_DEPS := $(SORTED_TINYSYS_DEPS) $(m)), \
    $(error $(TINYSYS_SCP): Missing loader image for processor $(p)) \
  ) \
  $(eval m := $(filter %/$(TINYSYS_SCP)-$(p).bin,$(_arg))) \
  $(if $(m), \
    $(eval SORTED_TINYSYS_DEPS := $(SORTED_TINYSYS_DEPS) $(m)), \
    $(error $(TINYSYS_SCP): Missing tinysys image for processor $(p)) \
  ) \
) \
$(strip $(SORTED_TINYSYS_DEPS)) \
$(eval m :=) \
$(eval _arg :=)
endef

###########################################################
## Template for compiling C and S files to objects
###########################################################

define compile-c-or-s-to-o
	$(hide)mkdir -p $(dir $@)
	$(hide)$(PRIVATE_CC) $(PRIVATE_CFLAGS) $(PRIVATE_INCLUDES) \
		-MD -MP -MF $(patsubst %.o,%.d,$@) -c $< -o $@
endef

###########################################################
## Template for compiling C and S files to objects without
## .d files
###########################################################

define compile-c-or-s-to-o-without-d
	$(hide)mkdir -p $(dir $@)
	$(hide)$(PRIVATE_CC) $(PRIVATE_CFLAGS) $(PRIVATE_INCLUDES) -c $< -o $@
endef

###########################################################
## Template for creating configuration header
###########################################################

# $1: Wrapper macro name, e.g. __TINYSYS_CONFIG_H
# $2: Config options in KEY=VALUE format, separated with spaces
# $3: Identification string to be displayed in any output
define gen-tinysys-header
	$(hide)mkdir -p $(dir $@)
	$(hide)rm -f $(@).tmp
	@echo '/*' > $(@).tmp; \
	echo ' * Automatically generated file; DO NOT EDIT.' >> $(@).tmp; \
	echo ' */' >> $(@).tmp; \
	echo '#ifndef $(1)' >> $(@).tmp; \
	echo -e '#define $(1)\n' >> $(@).tmp; \
	for i in $(2); do \
		KEY="$${i//=*}"; \
		VAL="$${i##*=}"; \
		if [ "$${VAL}" = 'yes' ]; then \
			echo "#define $${KEY}" >> $(@).tmp; \
		elif [ "$${VAL}" = 'no' ]; then \
			echo "/* $${KEY} is not set */" >> $(@).tmp; \
		else \
			echo "#define $${KEY} $${VAL}" >> $(@).tmp; \
		fi; \
	done; \
	echo -e '\n#endif /* $(1) */' >> $(@).tmp; \
	if [ -f '$@' ]; then \
		if cmp -s '$@' '$(@).tmp'; then \
			rm '$(@).tmp'; \
			echo '$(3): $(@) is update to date.'; \
		else \
			mv '$(@).tmp' '$@'; \
			echo '$(strip $(3)): Updated $@'; \
		fi; \
	else \
		mv '$(@).tmp' '$@'; \
		echo '$(strip $(3)): Generated $@'; \
	fi
endef

###########################################################
## Template for creating image header
###########################################################

# $1: Identification string to be displayed in any output
define gen-image-header
	@echo '$(1): Generating $@ ...'
	@mkdir -p $(dir $@)
	$(hide)echo 'NAME = $(patsubst img_hdr_%.cfg,%,$(notdir $@))' > $@
endef
