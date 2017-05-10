LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.hal3a.v3.nvram
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libcam.halsensor libnvramagentclient libbinder libnvram libcameracustom libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libcam.hal3a.v3.nvram.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.hal3a.v3.nvram
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libcam.halsensor libnvramagentclient libbinder libnvram libcameracustom libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libcam.hal3a.v3.nvram.so
include $(BUILD_PREBUILT)
