LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = hwcomposer.mt6797
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH = hw
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libgem libsync libm4u libion libbwc libion_mtk libdpframework libhardware libgralloc_extra libged libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = hwcomposer.mt6797.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = hwcomposer.mt6797
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH = hw
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libgem libsync libm4u libion libbwc libion_mtk libdpframework libhardware libgralloc_extra libged libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/hwcomposer.mt6797.so
include $(BUILD_PREBUILT)
