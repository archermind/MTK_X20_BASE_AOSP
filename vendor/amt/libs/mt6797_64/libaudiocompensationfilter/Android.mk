LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libaudiocompensationfilter
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libcustom_nvram libnvram libnativehelper libaudiocustparam libaudio_param_parser libmedia libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libaudiocompensationfilter.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libaudiocompensationfilter
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libcustom_nvram libnvram libnativehelper libaudiocustparam libaudio_param_parser libmedia libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libaudiocompensationfilter.so
include $(BUILD_PREBUILT)
