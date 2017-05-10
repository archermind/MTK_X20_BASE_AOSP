LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PREBUILT_EXECUTABLES := thermal_manager
include $(BUILD_MULTI_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE:= libmtcloader
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES_arm := libmtcloader.so

#bobule workaround pdk build error, needing review
LOCAL_MULTILIB := 32
LOCAL_MODULE_SUFFIX := .so

include $(BUILD_PREBUILT)



