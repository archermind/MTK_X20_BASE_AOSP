# this is BRM release config
# you should migrate turnkey alps/device/mediatek/common/device.mk to this file in correct way

# VoLTE Process
#ifeq ($(strip $(MTK_IMS_SUPPORT)),yes)
PRODUCT_PACKAGES += Gba
PRODUCT_PACKAGES += volte_xdmc
PRODUCT_PACKAGES += volte_core
PRODUCT_PACKAGES += volte_ua
PRODUCT_PACKAGES += volte_stack
PRODUCT_PACKAGES += volte_imcb
PRODUCT_PACKAGES += libipsec_ims

# MAL
PRODUCT_PACKAGES += libmdfx
PRODUCT_PACKAGES += mtkmal
PRODUCT_PACKAGES += libmal_mdmngr
PRODUCT_PACKAGES += libmal_rilproxy
PRODUCT_PACKAGES += libmal_simmngr
PRODUCT_PACKAGES += libmal_datamngr
PRODUCT_PACKAGES += libmal_rds
PRODUCT_PACKAGES += libmal_epdga
PRODUCT_PACKAGES += libmal_imsmngr
PRODUCT_PACKAGES += libmal

PRODUCT_PACKAGES += volte_imsm
PRODUCT_PACKAGES += volte_imspa
#else
#    ifeq ($(strip $(MTK_EPDG_SUPPORT)),yes) # EPDG without IMS

    # MAL
    PRODUCT_PACKAGES += libmdfx
    PRODUCT_PACKAGES += mtkmal
    PRODUCT_PACKAGES += libmal_mdmngr
    PRODUCT_PACKAGES += libmal_rilproxy
    PRODUCT_PACKAGES += libmal_simmngr
    PRODUCT_PACKAGES += libmal_datamngr
    PRODUCT_PACKAGES += libmal_rds
    PRODUCT_PACKAGES += libmal_epdga
    PRODUCT_PACKAGES += libmal_imsmngr
    PRODUCT_PACKAGES += libmal

    PRODUCT_PACKAGES += volte_imsm
    PRODUCT_PACKAGES += volte_imspa

#    endif
#endif

#ifeq ($(strip $(MTK_VT3G324M_SUPPORT)), yes)
PRODUCT_PACKAGES += llibmtk_vt_swip
PRODUCT_PACKAGES += libmtk_vt_utils
PRODUCT_PACKAGES += libmtk_vt_service
PRODUCT_PACKAGES += vtservice
#endif

#ifeq ($(strip $(MTK_VILTE_SUPPORT)),yes)
  PRODUCT_PACKAGES += libmtk_vt_service
  PRODUCT_PACKAGES += vtservice
#endif

# WFCA Process
#ifeq ($(strip $(MTK_WFC_SUPPORT)),yes)
  PRODUCT_PACKAGES += wfca
#endif

#ifeq ($(strip $(MTK_RCS_SUPPORT)),yes)
PRODUCT_PACKAGES += Gba
PRODUCT_PACKAGES += libjni_mds
#endif

# MediatekDM package & lib
#ifeq ($(strip $(MTK_MDM_APP)),yes)
    PRODUCT_PACKAGES += MediatekDM
    PRODUCT_PACKAGES += libjni_mdm
#endif

# MTK_MDLOGGER_SUPPORT is FO white list, delete from SuperSet
#  Put in superset would have build error
#ifeq ($(strip $(MTK_MDLOGGER_SUPPORT)),yes)
#  PRODUCT_PACKAGES += \
#    libmdloggerrecycle \
#    libmemoryDumpEncoder \
#    mdlogger
#ifeq ($(strip $(MTK_ENABLE_MD1)), yes)
#  PRODUCT_PACKAGES += emdlogger1
#endif
#ifeq ($(strip $(MTK_ENABLE_MD2)), yes)
#  PRODUCT_PACKAGES += emdlogger2
#endif
#ifeq ($(strip $(MTK_ENABLE_MD5)), yes)
#  PRODUCT_PACKAGES += emdlogger5
#endif
#endif

#ifeq ($(strip $(MTK_C2K_SUPPORT)), yes)
    PRODUCT_PACKAGES += cmddumper
    PRODUCT_PACKAGES += libviatelecom-withuim-ril
    PRODUCT_PACKAGES += c2k-ril-prop
#endif

#ifneq ($(strip $(MTK_PLATFORM)),)
  PRODUCT_PACKAGES += libnativecheck-jni
#endif

#ifeq ($(strip $(MTK_AUDIO_ALAC_SUPPORT)), yes)
  PRODUCT_PACKAGES += libMtkOmxAlacDec
#endif

#ifeq ($(strip $(MTK_SEC_VIDEO_PATH_SUPPORT)), yes)
#  ifeq ($(strip $(MTK_IN_HOUSE_TEE_SUPPORT)), yes)
  PRODUCT_PACKAGES += lib_uree_mtk_video_secure_al
#  endif
#endif

#ifeq ($(strip $(MTK_WMA_PLAYBACK_SUPPORT)), yes)
  PRODUCT_PACKAGES += libMtkOmxWmaDec
#endif

#ifeq ($(strip $(MTK_WMA_PLAYBACK_SUPPORT))_$(strip $(MTK_SWIP_WMAPRO)), yes_yes)
  PRODUCT_PACKAGES += libMtkOmxWmaProDec
#endif

#ifeq ($(strip $(MTK_MTKLOGGER_SUPPORT)), yes)
  PRODUCT_PACKAGES += MTKLogger
#endif

#OMA DRM part, MTK_DRM_APP should be set to false
#ifeq ($(strip $(MTK_DRM_APP)),yes)
#  PRODUCT_PACKAGES += libdrmmtkutil
#  ifeq ($(strip $(MTK_OMADRM_SUPPORT)), yes)
    PRODUCT_PACKAGES += libdrmmtkplugin
    PRODUCT_PACKAGES += drm_chmod
    PRODUCT_PACKAGES += libdcfdecoderjni
#  endif
#  ifeq ($(strip $(MTK_CTA_SET)), yes)
    PRODUCT_PACKAGES += libdrmctaplugin
    PRODUCT_PACKAGES += DataProtection
#  endif
#endif

# MTK_MDLOGGER_SUPPORT is FO white list, delete from SuperSet
#  Put in superset would have build error
#ifeq ($(strip $(MTK_DT_SUPPORT)),yes)
#  ifneq ($(strip $(EVDO_DT_SUPPORT)),yes)
#    ifeq ($(strip $(MTK_MDLOGGER_SUPPORT)),yes)
#      PRODUCT_PACKAGES += ExtModemLog
#      PRODUCT_PACKAGES += libextmdlogger_ctrl_jni
#      PRODUCT_PACKAGES += libextmdlogger_ctrl
#      PRODUCT_PACKAGES += extmdlogger
#    endif
#  endif
#endif

#ifeq ($(strip $(MTK_WVDRM_SUPPORT)),yes)
  #both L1 and L3 library
  PRODUCT_PACKAGES += com.google.widevine.software.drm.xml
  PRODUCT_PACKAGES += com.google.widevine.software.drm
  PRODUCT_PACKAGES += libdrmmtkutil
#  ifeq ($(strip $(MTK_WVDRM_L1_SUPPORT)),yes)
    PRODUCT_PACKAGES += lib_uree_mtk_crypto
#  else
#  endif
#else
#endif

#ifeq ($(strip $(MTK_WVDRM_SUPPORT)),yes)
  #Mock modular drm plugin for cts
  #both L1 and L3 library
  PRODUCT_PACKAGES += libwvdrmengine
  #package depended by libwvdrmengine
  PRODUCT_PACKAGES += libcdm
  PRODUCT_PACKAGES += libcdm_utils
  PRODUCT_PACKAGES += libwvlevel3
  PRODUCT_PACKAGES += libwvdrmcryptoplugin
  PRODUCT_PACKAGES += libwvdrmdrmplugin
  PRODUCT_PACKAGES += libcdm_protos
  #package depended by libdrmwvmplugin
  PRODUCT_PACKAGES += libdrmwvmcommon
  PRODUCT_PACKAGES += liboemcrypto_static
#  ifeq ($(strip $(MTK_WVDRM_L1_SUPPORT)),yes)
    PRODUCT_PACKAGES += lib_uree_mtk_modular_drm
    PRODUCT_PACKAGES += liboemcrypto
#  endif
#endif

#ifeq ($(strip $(MTK_C2K_SUPPORT)), yes)
#For C2K control modules
PRODUCT_PACKAGES += \
          libc2kutils \
          flashlessd \
          statusd

#ifeq ($(strip $(MTK_C2K_SUPPORT)), yes)
    PRODUCT_PACKAGES += emdlogger3
    PRODUCT_PACKAGES += c2k-ril-prop
#endif

#For C2K GPS
PRODUCT_PACKAGES += \
          libviagpsrpc \
          librpc
#endif

#ifeq ($(strip $(MTK_AGPS_APP)), yes)
  PRODUCT_PACKAGES += LocationEM2 \
                      mtk_agpsd \
                      libssladp
#endif

#TODO: libepos is built via MTK_GPS_SUPPORT
#   but we don't know how
  PRODUCT_PACKAGES += libepos

#ifeq ($(strip $(MTK_HOTKNOT_SUPPORT)), yes)
  PRODUCT_PACKAGES += libhotknot_GT1XX
  PRODUCT_PACKAGES += libhotknot_GT9XX
#endif

#TODO: check why jpeg library built in some case
  PRODUCT_PACKAGES += libSwJpgCodec
  PRODUCT_PACKAGES += libJpgDecDrv_plat
  PRODUCT_PACKAGES += libJpgDecPipe
  PRODUCT_PACKAGES += libmhalImageCodec
