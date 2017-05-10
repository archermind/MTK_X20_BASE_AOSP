LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = audio.primary.mt6797
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH = hw
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libaudio_param_parser libmedia libbinder libhardware_legacy libhardware libblisrc libnvram libspeech_enh_lib libpowermanager libaudiocustparam libaudiocompensationfilter libcvsd_mtk libmsbc_mtk libaudioutils libaudiocomponentengine libtinyalsa libtinycompress libcustom_nvram libtinyxml libbluetooth_mtk_pure libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = audio.primary.mt6797.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = audio.primary.mt6797
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH = hw
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libaudio_param_parser libmedia libbinder libhardware_legacy libhardware libblisrc libnvram libspeech_enh_lib libpowermanager libaudiocustparam libaudiocompensationfilter libcvsd_mtk libmsbc_mtk libaudioutils libaudiocomponentengine libtinyalsa libtinycompress libcustom_nvram libtinyxml libbluetooth_mtk_pure libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/audio.primary.mt6797.so
include $(BUILD_PREBUILT)
