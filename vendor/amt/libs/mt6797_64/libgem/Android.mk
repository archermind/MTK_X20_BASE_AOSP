LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libgem
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libbinder libhardware libhardware_legacy libion libion_mtk libgralloc_extra libpng libsync libEGL libGLESv2 libc++
LOCAL_EXPORT_C_INCLUDE_DIRS = $(LOCAL_PATH)/include
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libgem.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libgem
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libbinder libhardware libhardware_legacy libion libion_mtk libgralloc_extra libpng libsync libEGL libGLESv2 libc++
LOCAL_EXPORT_C_INCLUDE_DIRS = $(LOCAL_PATH)/include
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libgem.so
include $(BUILD_PREBUILT)
