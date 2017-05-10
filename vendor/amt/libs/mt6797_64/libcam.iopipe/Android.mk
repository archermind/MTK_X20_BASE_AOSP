LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.iopipe
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libcam_hwutils libstdc++ libimageio_plat_drv libimageio_plat_pipe libcam_utils libcam.metadata libcam.halsensor libimageio libcamdrv_isp libcamdrv_imem libdpframework libJpgEncPipe libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libcam.iopipe.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.iopipe
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libcam_hwutils libstdc++ libimageio_plat_drv libimageio_plat_pipe libcam_utils libcam.metadata libcam.halsensor libimageio libcamdrv_isp libcamdrv_imem libdpframework libJpgEncPipe libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libcam.iopipe.so
include $(BUILD_PREBUILT)
