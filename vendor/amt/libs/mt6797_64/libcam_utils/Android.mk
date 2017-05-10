LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam_utils
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libui libsync libstdc++ libhardware libgralloc_extra libcamera_metadata libcamera_client libmtkcamera_client libmtk_mmutils libion libion_mtk libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libcam_utils.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam_utils
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libui libsync libstdc++ libhardware libgralloc_extra libcamera_metadata libcamera_client libmtkcamera_client libmtk_mmutils libion libion_mtk libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libcam_utils.so
include $(BUILD_PREBUILT)
