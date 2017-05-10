LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam3_hwpipeline
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libcam_utils libcam_hwutils libcam.metadata libcam.metadataprovider libcam3_utils libcam3_pipeline libcam3_hwnode libcam.halsensor libcam.iopipe libstdc++ libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libcam3_hwpipeline.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam3_hwpipeline
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libcam_utils libcam_hwutils libcam.metadata libcam.metadataprovider libcam3_utils libcam3_pipeline libcam3_hwnode libcam.halsensor libcam.iopipe libstdc++ libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libcam3_hwpipeline.so
include $(BUILD_PREBUILT)
