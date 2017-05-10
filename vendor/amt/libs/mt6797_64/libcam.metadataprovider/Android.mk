LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.metadataprovider
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libcam_utils libcam.metadata libcamera_metadata libcam.halsensor libcam.hal3a.v3.dng libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libcam.metadataprovider.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.metadataprovider
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libcam_utils libcam.metadata libcamera_metadata libcam.halsensor libcam.hal3a.v3.dng libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libcam.metadataprovider.so
include $(BUILD_PREBUILT)
