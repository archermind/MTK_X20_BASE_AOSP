LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.paramsmgr
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libcamera_client libmtkcamera_client libcam_utils libcam_hwutils libcam.hal3a.v3 libcam.halsensor libcam.metadata libcam.metadataprovider libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libcam.paramsmgr.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.paramsmgr
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libcamera_client libmtkcamera_client libcam_utils libcam_hwutils libcam.hal3a.v3 libcam.halsensor libcam.metadata libcam.metadataprovider libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libcam.paramsmgr.so
include $(BUILD_PREBUILT)
