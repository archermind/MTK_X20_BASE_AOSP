include $(TRUSTZONE_CUSTOM_BUILD_PATH)/common_config.mk


### IN_HOUSE TEE SETTINGS ###
MTEE_ADDITIONAL_DEPENDENCIES := $(TRUSTZONE_PROJECT_MAKEFILE) $(TRUSTZONE_CUSTOM_BUILD_PATH)/common_config.mk $(TRUSTZONE_CUSTOM_BUILD_PATH)/mtee_config.mk
ifeq ($(MTK_IN_HOUSE_TEE_FORCE_32_SUPPORT),yes)
  MTEE_RAW_IMAGE_NAME := $(call intermediates-dir-for,EXECUTABLES,tz.img,,,$(TARGET_2ND_ARCH_VAR_PREFIX))/tz.img
else
  MTEE_RAW_IMAGE_NAME := $(call intermediates-dir-for,EXECUTABLES,tz.img)/tz.img
endif
$(info MTEE_RAW_IMAGE_NAME = $(MTEE_RAW_IMAGE_NAME))
MTEE_SIGNED_IMAGE_NAME := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/$(ARCH_MTK_PLATFORM)_$(TRUSTZONE_IMPL)_signed.img
MTEE_PADDING_IMAGE_NAME := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/$(ARCH_MTK_PLATFORM)_$(TRUSTZONE_IMPL)_pad.img
MTEE_COMP_IMAGE_NAME := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/$(ARCH_MTK_PLATFORM)_$(TRUSTZONE_IMPL).img
MTEE_TEMP_CFG_FILE := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/img_hdr_mtee.cfg

ifeq ($(HOST_OS),darwin)
MTEE_PROT_TOOL := vendor/mediatek/proprietary/trustzone/custom/build/tools/TeeImgSignEncTool.$(HOST_OS)
else
MTEE_PROT_TOOL := vendor/mediatek/proprietary/trustzone/custom/build/tools/TeeImgSignEncTool
endif

ifeq ($(MTK_ATF_SUPPORT),yes)
MTEE_TEMP_PADDING_FILE := $(TRUSTZONE_IMAGE_OUTPUT_PATH)/bin/$(ARCH_MTK_PLATFORM)_$(TRUSTZONE_IMPL)_pad.txt

$(MTEE_TEMP_PADDING_FILE): ALIGNMENT=512
$(MTEE_TEMP_PADDING_FILE): MKIMAGE_HDR_SIZE=512
$(MTEE_TEMP_PADDING_FILE): RSA_SIGN_HDR_SIZE=576
$(MTEE_TEMP_PADDING_FILE): $(MTEE_RAW_IMAGE_NAME) $(MTEE_ADDITIONAL_DEPENDENCIES)
	@echo MTEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) rm -f $@
	$(hide) FILE_SIZE=$$(($$(wc -c < "$(MTEE_RAW_IMAGE_NAME)")+$(MKIMAGE_HDR_SIZE)+$(RSA_SIGN_HDR_SIZE)));\
	REMAINDER=$$(($${FILE_SIZE} % $(ALIGNMENT)));\
	if [ $${REMAINDER} -ne 0 ]; then dd if=/dev/zero of=$@ bs=$$(($(ALIGNMENT)-$${REMAINDER})) count=1; else touch $@; fi

$(MTEE_TEMP_CFG_FILE): $(TEE_DRAM_SIZE_CFG) $(MTEE_ADDITIONAL_DEPENDENCIES)
	@echo MTEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) rm -f $@
	@echo "LOAD_MODE = 0" > $@
	@echo "NAME = tee" >> $@
	@echo "LOAD_ADDR =" $(TEE_TOTAL_DRAM_SIZE) >> $@

$(MTEE_PADDING_IMAGE_NAME): $(MTEE_RAW_IMAGE_NAME) $(MTEE_TEMP_PADDING_FILE) $(MTEE_ADDITIONAL_DEPENDENCIES)
	@echo MTEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) cat $(MTEE_RAW_IMAGE_NAME) $(MTEE_TEMP_PADDING_FILE) > $@

$(MTEE_SIGNED_IMAGE_NAME):ALIGNMENT=512
$(MTEE_SIGNED_IMAGE_NAME): $(MTEE_PADDING_IMAGE_NAME) $(TRUSTZONE_IMG_PROTECT_CFG) $(TEE_DRAM_SIZE_CFG) $(MTEE_ADDITIONAL_DEPENDENCIES)
	@echo MTEE build: $@
	$(hide) $(MTEE_PROT_TOOL) $(TRUSTZONE_IMG_PROTECT_CFG) $(MTEE_PADDING_IMAGE_NAME) $@ $(TEE_DRAM_SIZE)
	$(hide) FILE_SIZE=$$(wc -c < "$(MTEE_SIGNED_IMAGE_NAME)");REMAINDER=$$(($${FILE_SIZE} % $(ALIGNMENT)));\
	if [ $${REMAINDER} -ne 0 ]; then echo "[ERROR] File $@ size $${FILE_SIZE} is not $(ALIGNMENT) bytes aligned";exit 1; fi

$(MTEE_COMP_IMAGE_NAME):ALIGNMENT=512
$(MTEE_COMP_IMAGE_NAME): $(MTEE_SIGNED_IMAGE_NAME) $(MTK_MKIMAGE_TOOL) $(MTEE_TEMP_CFG_FILE) $(MTEE_ADDITIONAL_DEPENDENCIES)
	@echo MTEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) $(MTK_MKIMAGE_TOOL) $(MTEE_SIGNED_IMAGE_NAME) $(MTEE_TEMP_CFG_FILE) > $@
	$(hide) FILE_SIZE=$$(wc -c < "$(MTEE_COMP_IMAGE_NAME)");REMAINDER=$$(($${FILE_SIZE} % $(ALIGNMENT)));\
	if [ $${REMAINDER} -ne 0 ]; then echo "[ERROR] File $@ size $${FILE_SIZE} is not $(ALIGNMENT) bytes aligned";exit 1; fi

else

# support Mac OS and Linux OS
ifeq ($(HOST_OS),darwin)
MTEE_PROT_TOOL := vendor/mediatek/proprietary/trustzone/mtee/build/tools/MteeImgSignEncTool.$(HOST_OS)
else
#MTEE_PROT_TOOL := vendor/mediatek/proprietary/trustzone/custom/build/tools/MteeImgSignEncTool/MteeImgSignEncTool.linux
MTEE_PROT_TOOL := vendor/mediatek/proprietary/trustzone/custom/build/tools/TeeImgSignEncTool
endif

$(MTEE_TEMP_CFG_FILE): $(MTEE_ADDITIONAL_DEPENDENCIES)
	@echo MTEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) rm -f $@
	@echo "LOAD_MODE = -1" > $@
	@echo "NAME = tee" >> $@
	@echo "LOAD_ADDR = 0xffffffff" >> $@

$(MTEE_SIGNED_IMAGE_NAME): $(MTEE_RAW_IMAGE_NAME) $(MTEE_PROT_TOOL) $(TEE_DRAM_SIZE_CFG) $(MTEE_ADDITIONAL_DEPENDENCIES)
	@echo MTEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) $(MTEE_PROT_TOOL) $(TRUSTZONE_IMG_PROTECT_CFG) $(MTEE_RAW_IMAGE_NAME) $@ $(TEE_DRAM_SIZE)

$(MTEE_COMP_IMAGE_NAME): $(MTEE_SIGNED_IMAGE_NAME) $(MTK_MKIMAGE_TOOL) $(MTEE_TEMP_CFG_FILE) $(MTEE_ADDITIONAL_DEPENDENCIES)
	@echo MTEE build: $@
	$(hide) mkdir -p $(dir $@)
	$(hide) $(MTK_MKIMAGE_TOOL) $(MTEE_SIGNED_IMAGE_NAME) $(MTEE_TEMP_CFG_FILE) > $@

endif

