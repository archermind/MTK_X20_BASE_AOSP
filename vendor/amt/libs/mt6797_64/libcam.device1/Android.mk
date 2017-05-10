LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.device1
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libcamera_client libmtkcamera_client libcam_mmp libcam_hwutils libcam_utils libcam.utils libcam1_utils libcam.utils.cpuctrl libcam.paramsmgr libcam.client libcam.camadapter libcameracustom libcam.halsensor libcam.hal3a.v3 libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libcam.device1.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.device1
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libcamera_client libmtkcamera_client libcam_mmp libcam_hwutils libcam_utils libcam.utils libcam1_utils libcam.utils.cpuctrl libcam.paramsmgr libcam.client libcam.camadapter libcameracustom libcam.halsensor libcam.hal3a.v3 libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libcam.device1.so
include $(BUILD_PREBUILT)
