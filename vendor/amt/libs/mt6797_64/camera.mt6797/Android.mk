LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = camera.mt6797
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH = hw
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libcamera_client libmtkcamera_client libcam_utils libcam_hwutils libcam.metadataprovider libcam.metadata libcamera_metadata libcam.halsensor libcam.hal3a.v3 libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = camera.mt6797.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = camera.mt6797
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH = hw
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libcamera_client libmtkcamera_client libcam_utils libcam_hwutils libcam.metadataprovider libcam.metadata libcamera_metadata libcam.halsensor libcam.hal3a.v3 libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/camera.mt6797.so
include $(BUILD_PREBUILT)
