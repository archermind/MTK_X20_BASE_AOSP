LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libacdk
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libimageio libcam.iopipe libcam_utils libcam.halsensor libcam.metadata libm4u libcamdrv_imem libstdc++ libhardware libbinder libcamera_client libmtkcamera_client libcam.hal3a.v3 libcam.hal3a.v3.nvram libcam.hal3a.v3.lsctbl libcamalgo libcam.metadataprovider libcam.camshot libJpgEncPipe libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libacdk.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libacdk
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libimageio libcam.iopipe libcam_utils libcam.halsensor libcam.metadata libm4u libcamdrv_imem libstdc++ libhardware libbinder libcamera_client libmtkcamera_client libcam.hal3a.v3 libcam.hal3a.v3.nvram libcam.hal3a.v3.lsctbl libcamalgo libcam.metadataprovider libcam.camshot libJpgEncPipe libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libacdk.so
include $(BUILD_PREBUILT)
