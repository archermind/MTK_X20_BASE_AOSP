LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libimageio_plat_drv
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libcamdrv_tuning_mgr libstdc++ libm4u libdpframework libcamdrv_isp libcamdrv_imem libcamdrv_twin libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libimageio_plat_drv.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libimageio_plat_drv
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libcamdrv_tuning_mgr libstdc++ libm4u libdpframework libcamdrv_isp libcamdrv_imem libcamdrv_twin libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libimageio_plat_drv.so
include $(BUILD_PREBUILT)
