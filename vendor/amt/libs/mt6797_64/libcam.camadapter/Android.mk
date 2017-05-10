LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.camadapter
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_64 = libbinder libui libjpeg libcamera_client libmtkcamera_client libskia libcam_utils libcam1_utils libcam.paramsmgr libcam.exif libcam.exif.v3 libcam_hwutils libfeature.face libfeature_cfb libcam.feature_utils libdngop libcamera_metadata libcam.camshot libJpgEncPipe libdpframework libcameracustom libcam.halsensor libcam.iopipe libcam.metadata libcam.legacypipeline libcam.utils libcam3_pipeline libcam3_utils libcam3_hwnode libcam.client libcam.metadataprovider libcamdrv_imem libcam.hal3a.v3 libcam.hal3a.v3.dng libc++
LOCAL_MULTILIB = 64
LOCAL_SRC_FILES_64 = libcam.camadapter.so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_IS_HOST_MODULE =
LOCAL_MODULE = libcam.camadapter
LOCAL_MODULE_CLASS = SHARED_LIBRARIES
LOCAL_MODULE_PATH =
LOCAL_MODULE_RELATIVE_PATH =
LOCAL_MODULE_SUFFIX = .so
LOCAL_SHARED_LIBRARIES_32 = libbinder libui libjpeg libcamera_client libmtkcamera_client libskia libcam_utils libcam1_utils libcam.paramsmgr libcam.exif libcam.exif.v3 libcam_hwutils libfeature.face libfeature_cfb libcam.feature_utils libdngop libcamera_metadata libcam.camshot libJpgEncPipe libdpframework libcameracustom libcam.halsensor libcam.iopipe libcam.metadata libcam.legacypipeline libcam.utils libcam3_pipeline libcam3_utils libcam3_hwnode libcam.client libcam.metadataprovider libcamdrv_imem libcam.hal3a.v3 libcam.hal3a.v3.dng libc++
LOCAL_MULTILIB = 32
LOCAL_SRC_FILES_32 = arm/libcam.camadapter.so
include $(BUILD_PREBUILT)
