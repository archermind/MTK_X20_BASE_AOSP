LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcamalgo
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libmtk_drvb libEGL libGLESv2 libgui libbinder libui libgralloc_extra libcamdrv_imem libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libcamalgo.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcamalgo
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libmtk_drvb libEGL libGLESv2 libgui libbinder libui libgralloc_extra libcamdrv_imem libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libcamalgo.so
include $(BUILD_PREBUILT)
