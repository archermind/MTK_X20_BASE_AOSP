LOCAL_PATH:= $(call my-dir)
ifeq ($(MTK_TARGET_PROJECT),$(MTK_BASE_PROJECT))
include $(call all-makefiles-under,$(LOCAL_PATH)/$(TARGET_DEVICE))
else
include $(call all-makefiles-under,$(LOCAL_PATH)/$(MTK_TARGET_PROJECT))
endif
