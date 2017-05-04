# this is platform common device config
# you should migrate turnkey alps/build/target/product/common.mk to this file in correct way

# TARGET_PREBUILT_KERNEL should be assigned by central building system
#ifeq ($(TARGET_PREBUILT_KERNEL),)
#LOCAL_KERNEL := device/mediatek/common/kernel
#else
#LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
#endif

#PRODUCT_COPY_FILES += $(LOCAL_KERNEL):kernel

# MediaTek framework base modules
PRODUCT_PACKAGES += \
    mediatek-common \
    mediatek-framework \
    CustomPropInterface \
    mediatek-telephony-common 

ifeq (OP09_SPEC0212_SEGC,$(OPTR_SPEC_SEG_DEF))
PRODUCT_PACKAGES += \
    FwkPluginClib
else
PRODUCT_PACKAGES += \
    FwkPlugin
endif


ifneq ($(strip $(MTK_BASIC_PACKAGE)), yes)
# Override the PRODUCT_BOOT_JARS to include the MediaTek system base modules for global access
PRODUCT_BOOT_JARS += \
    mediatek-common \
    mediatek-framework \

ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
PRODUCT_BOOT_JARS += \
    mediatek-telephony-common
endif

PRODUCT_COPY_FILES += vendor/mediatek/proprietary/frameworks/opt/GeoCoding/geocoding.db:system/etc/geocoding.db
PRODUCT_COPY_FILES += vendor/mediatek/proprietary/frameworks/opt/GeoCoding/NumberHeadWithIDToByte:system/etc/NumberHeadWithIDToByte
endif

# Telephony
PRODUCT_COPY_FILES += device/mediatek/common/apns-conf.xml:system/etc/apns-conf.xml
PRODUCT_COPY_FILES += device/mediatek/common/spn-conf.xml:system/etc/spn-conf.xml

# Audio
PRODUCT_COPY_FILES += device/mediatek/common/audio_em.xml:system/etc/audio_em.xml

# For C2K CDMA feature file
ifeq ($(strip $(MTK_C2K_SUPPORT)), yes)
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.telephony.cdma.xml:system/etc/permissions/android.hardware.telephony.cdma.xml
endif

#MTB
PRODUCT_PACKAGES += mtk_setprop

#MMS
ifeq ($(strip $(MTK_BASIC_PACKAGE)), yes)
    ifndef MTK_TB_WIFI_3G_MODE
        PRODUCT_PACKAGES += messaging
    else
        ifeq ($(strip $(MTK_TB_WIFI_3G_MODE)), 3GDATA_SMS)
            PRODUCT_PACKAGES += messaging
        endif
    endif
endif

ifeq ($(strip $(MTK_BSP_PACKAGE)), yes)
    ifndef MTK_TB_WIFI_3G_MODE
        PRODUCT_PACKAGES += messaging
    else
        ifeq ($(strip $(MTK_TB_WIFI_3G_MODE)), 3GDATA_SMS)
            PRODUCT_PACKAGES += messaging
        endif
    endif
endif

ifneq ($(strip $(MTK_BASIC_PACKAGE)), yes)
    ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
        ifneq ($(strip $(MTK_A1_FEATURE)), yes)
            ifndef MTK_TB_WIFI_3G_MODE
                PRODUCT_PACKAGES += MtkMms
            else
                ifeq ($(strip $(MTK_TB_WIFI_3G_MODE)), 3GDATA_SMS)
                    PRODUCT_PACKAGES += MtkMms
                endif
            endif
        endif
    endif
endif

ifneq ($(strip $(MTK_BASIC_PACKAGE)), yes)
    ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
        PRODUCT_PACKAGES += MtkCalendar
        PRODUCT_PACKAGES += MtkBrowser
        PRODUCT_PACKAGES += MtkQuickSearchBox
    endif
endif

# Telephony begin
PRODUCT_PACKAGES += muxreport
PRODUCT_PACKAGES += mtkrild
PRODUCT_PACKAGES += mtk-ril
PRODUCT_PACKAGES += libutilrilmtk
PRODUCT_PACKAGES += gsm0710muxd
PRODUCT_PACKAGES += mtkrildmd2
PRODUCT_PACKAGES += mtk-rilmd2
PRODUCT_PACKAGES += librilmtkmd2
PRODUCT_PACKAGES += gsm0710muxdmd2
PRODUCT_PACKAGES += md_minilog_util
PRODUCT_PACKAGES += BSPTelephonyDevTool
PRODUCT_PACKAGES += ppl_agent

ifeq ($(strip $(MTK_C2K_SUPPORT)), yes)
#For C2K RIL
PRODUCT_PACKAGES += \
          viarild \
          libc2kril \
          libviatelecom-withuim-ril \
          viaradiooptions \
          librpcril \
          ctclient

#Set CT6M_SUPPORT
ifeq ($(strip $(CT6M_SUPPORT)), yes)
PRODUCT_PACKAGES += CdmaSystemInfo
PRODUCT_PROPERTY_OVERRIDES += ro.ct6m_support=1
PRODUCT_COPY_FILES += vendor/mediatek/proprietary/frameworks/base/telephony/etc/spn-conf-op09.xml:system/etc/spn-conf-op09.xml
endif

#For PPPD
PRODUCT_PACKAGES += \
          ip-up-cdma \
          ipv6-up-cdma \
          link-down-cdma \
          pppd_via

#For C2K control modules
PRODUCT_PACKAGES += \
          libc2kutils \
          flashlessd \
          statusd

#For C2K GPS
PRODUCT_PACKAGES += \
          libviagpsrpc \
          librpc
endif

# MAL shared library
PRODUCT_PACKAGES += libmdfx
PRODUCT_PACKAGES += libmal_mdmngr
PRODUCT_PACKAGES += libmal_nwmngr
PRODUCT_PACKAGES += libmal_rilproxy
PRODUCT_PACKAGES += libmal_simmngr
PRODUCT_PACKAGES += libmal_datamngr
PRODUCT_PACKAGES += libmal_rds
PRODUCT_PACKAGES += libmal_epdga
PRODUCT_PACKAGES += libmal_imsmngr
PRODUCT_PACKAGES += libmal

PRODUCT_PACKAGES += volte_imsm
PRODUCT_PACKAGES += volte_imspa

# MAL-Dongle shared library
PRODUCT_PACKAGES += libmd_mdmngr
PRODUCT_PACKAGES += libmd_rilproxy
PRODUCT_PACKAGES += libmd_simmngr
PRODUCT_PACKAGES += libmd_datamngr
PRODUCT_PACKAGES += libmd_nwmngr
PRODUCT_PACKAGES += libmd

# # Volte IMS shared library
PRODUCT_PACKAGES += volte_imspa_md

# Add for (VzW) chipset test
ifneq ($(strip $(MTK_VZW_CHIPTEST_MODE_SUPPORT)), 0)
PRODUCT_PACKAGES += libatch
PRODUCT_PACKAGES += libatcputil
PRODUCT_PACKAGES += atcp
PRODUCT_PACKAGES += libswext_plugin
PRODUCT_PACKAGES += libnetmngr_plugin

PRODUCT_PACKAGES += liblannetmngr_core
PRODUCT_PACKAGES += liblannetmngr_api
PRODUCT_PACKAGES += lannetmngrd
PRODUCT_PACKAGES += lannetmngr_test
endif

# VoLTE Process
ifeq ($(strip $(MTK_IMS_SUPPORT)),yes)
PRODUCT_PACKAGES += Gba
PRODUCT_PACKAGES += volte_xdmc
PRODUCT_PACKAGES += volte_core
PRODUCT_PACKAGES += volte_ua
PRODUCT_PACKAGES += volte_stack
PRODUCT_PACKAGES += volte_imcb
PRODUCT_PACKAGES += libipsec_ims

# MAL Process
PRODUCT_PACKAGES += mtkmal

# # Volte IMS Dongle Process
PRODUCT_PACKAGES += volte_imsm_md

# MAL/MAL-Dongle init script
PRODUCT_COPY_FILES += device/mediatek/common/init.mal.rc:root/init.mal.rc

else
    ifeq ($(strip $(MTK_EPDG_SUPPORT)),yes) # EPDG without IMS

    # MAL Process
    PRODUCT_PACKAGES += mtkmal

    # # Volte IMS Dongle Process
#    PRODUCT_PACKAGES += volte_imsm_md

    # MAL/MAL-Dongle init script
    PRODUCT_COPY_FILES += device/mediatek/common/init.mal.rc:root/init.mal.rc
 
    endif
endif

# include init.volte.rc
ifeq ($(MTK_IMS_SUPPORT),yes)
    ifneq ($(wildcard $(MTK_TARGET_PROJECT_FOLDER)/init.volte.rc),)
        PRODUCT_COPY_FILES += $(MTK_TARGET_PROJECT_FOLDER)/init.volte.rc:root/init.volte.rc
    else
        ifneq ($(wildcard $(MTK_PROJECT_FOLDER)/init.volte.rc),)
            PRODUCT_COPY_FILES += $(MTK_PROJECT_FOLDER)/init.volte.rc:root/init.volte.rc
        else
            PRODUCT_COPY_FILES += device/mediatek/common/init.volte.rc:root/init.volte.rc
        endif
    endif
endif

ifeq ($(strip $(MTK_VT3G324M_SUPPORT)), yes)
PRODUCT_PACKAGES += llibmtk_vt_swip
PRODUCT_PACKAGES += libmtk_vt_utils
PRODUCT_PACKAGES += libmtk_vt_wrapper
PRODUCT_PACKAGES += libmtk_vt_service
PRODUCT_PACKAGES += vtservice
endif

ifeq ($(strip $(MTK_VILTE_SUPPORT)),yes)
  PRODUCT_PACKAGES += libmtk_vt_wrapper
  PRODUCT_PACKAGES += libmtk_vt_service
  PRODUCT_PACKAGES += vtservice
endif

# WFCA Process
ifeq ($(strip $(MTK_WFC_SUPPORT)),yes)
  PRODUCT_PACKAGES += wfca
  PRODUCT_COPY_FILES += device/mediatek/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')/init.wfca.rc:root/init.wfca.rc
endif


# Hwui program binary service
PRODUCT_PACKAGES += program_binary_service

ifeq ($(strip $(MTK_RCS_SUPPORT)),yes)
PRODUCT_PACKAGES += Gba
PRODUCT_PACKAGES += libjni_mds
endif

ifeq ($(strip $(MTK_PRIVACY_PROTECTION_LOCK)),yes)
  PRODUCT_PACKAGES += PrivacyProtectionLock
endif

ifeq ($(strip $(MTK_USB_CBA_SUPPORT)),yes)
  PRODUCT_PACKAGES += UsbChecker
endif

ifeq ($(strip $(GOOGLE_RELEASE_RIL)), yes)
    PRODUCT_PACKAGES += libril
else
    PRODUCT_PACKAGES += librilmtk
endif
# Telephony end

# For MTK Camera
PRODUCT_PACKAGES += Camera

ifeq ($(strip $(MTK_HEART_RATE_MONITOR_SUPPORT)),yes)
PRODUCT_PACKAGES += HeartRate
endif

PRODUCT_DEFAULT_PROPERTY_OVERRIDES += camera.disable_zsl_mode=1

PRODUCT_PACKAGES += libBnMtkCodec
PRODUCT_PACKAGES += MtkCodecService
PRODUCT_PACKAGES += autokd
RODUCT_PACKAGES += \
    dhcp6c \
    dhcp6ctl \
    dhcp6c.conf \
    dhcp6cDNS.conf \
    dhcp6s \
    dhcp6s.conf \
    dhcp6c.script \
    dhcp6cctlkey \
    libifaddrs

# meta tool
ifeq ($(MTK_INTERNAL),yes)
ifneq ($(wildcard vendor/mediatek/proprietary/buildinfo/label.ini),)
  include vendor/mediatek/proprietary/buildinfo/label.ini
  ifeq ($(MTK_BUILD_VERNO),ALPS.W10.24.p0)
    MTK_BUILD_VERNO := $(MTK_INTERNAL_BUILD_VERNO)
  endif
  ifeq ($(MTK_WEEK_NO),W10.24)
    MTK_WEEK_NO := $(MTK_INTERNAL_WEEK_NO)
  endif
endif
endif
$(call inherit-product-if-exists, vendor/mediatek/proprietary/buildinfo/branch.mk)
PRODUCT_PROPERTY_OVERRIDES += ro.mediatek.version.release=$(strip $(MTK_BUILD_VERNO))
PRODUCT_PROPERTY_OVERRIDES += ro.mediatek.version.sdk=4

# To specify customer's releasekey
ifeq ($(MTK_INTERNAL),yes)
  PRODUCT_DEFAULT_DEV_CERTIFICATE := device/mediatek/common/security/releasekey
else
  ifeq ($(MTK_SIGNATURE_CUSTOMIZATION),yes)
    ifeq ($(wildcard device/mediatek/common/security/$(strip $(MTK_TARGET_PROJECT))),)
      $(error Please create device/mediatek/common/security/$(strip $(MTK_TARGET_PROJECT))/ and put your releasekey there!!)
    else
      PRODUCT_DEFAULT_DEV_CERTIFICATE := device/mediatek/common/security/$(strip $(MTK_TARGET_PROJECT))/releasekey
    endif
  else
#   Not specify PRODUCT_DEFAULT_DEV_CERTIFICATE and the default testkey will be used.
  endif
endif

# Handheld core hardware
PRODUCT_COPY_FILES += frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml

# Bluetooth Low Energy Capability
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.bluetooth_le.xml:system/etc/permissions/android.hardware.bluetooth_le.xml

# Bluetooth DUN profile
ifeq ($(MTK_BT_BLUEDROID_DUN_GW_12),yes)
PRODUCT_PROPERTY_OVERRIDES += bt.profiles.dun.enabled=1
PRODUCT_PACKAGES += pppd_btdun libpppbtdun.so
endif

# Bluetooth BIP profile cover art feature
ifeq ($(MTK_BT_BLUEDROID_AVRCP_TG_16),yes)
  PRODUCT_PROPERTY_OVERRIDES += bt.profiles.bip.coverart.enable=1
endif

# Customer configurations
ifneq ($(wildcard $(MTK_TARGET_PROJECT_FOLDER)/custom.conf),)
PRODUCT_COPY_FILES += $(MTK_TARGET_PROJECT_FOLDER)/custom.conf:system/etc/custom.conf
else
ifdef OPTR_SPEC_SEG_DEF
    ifneq ($(strip $(OPTR_SPEC_SEG_DEF)),NONE)
        OPTR := $(word 1,$(subst _,$(space),$(OPTR_SPEC_SEG_DEF)))
        SPEC := $(word 2,$(subst _,$(space),$(OPTR_SPEC_SEG_DEF)))
        SEG  := $(word 3,$(subst _,$(space),$(OPTR_SPEC_SEG_DEF)))
        ifneq ($(wildcard vendor/mediatek/proprietary/operator/$(OPTR)/$(SPEC)/$(SEG)/custom.conf),)
        PRODUCT_COPY_FILES += vendor/mediatek/proprietary/operator/$(OPTR)/$(SPEC)/$(SEG)/custom.conf:system/etc/custom.conf
        else
        PRODUCT_COPY_FILES += device/mediatek/common/custom.conf:system/etc/custom.conf
        endif
    else
        PRODUCT_COPY_FILES += device/mediatek/common/custom.conf:system/etc/custom.conf
    endif
else
    PRODUCT_COPY_FILES += device/mediatek/common/custom.conf:system/etc/custom.conf
endif
endif

# Recovery
PRODUCT_COPY_FILES += $(MTK_PROJECT_FOLDER)/recovery.fstab:system/etc/recovery.fstab

ifndef MTK_PLATFORM_DIR
  ifneq ($(wildcard device/mediatek/$(MTK_PLATFORM)),)
    MTK_PLATFORM_DIR = $(MTK_PLATFORM)
  else
    MTK_PLATFORM_DIR = $(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')
  endif
endif

ifeq ($(wildcard device/mediatek/$(MTK_PLATFORM_DIR)),)
  $(error the platform dir changed, expected: device/mediatek/$(MTK_PLATFORM_DIR), please check manually)
endif

# GMS interface
ifdef BUILD_GMS
ifeq ($(strip $(BUILD_GMS)), yes)
$(call inherit-product-if-exists, vendor/google/products/gms.mk)

PRODUCT_PROPERTY_OVERRIDES += \
      ro.com.google.clientidbase=alps-$(TARGET_PRODUCT)-{country} \
      ro.com.google.clientidbase.ms=alps-$(TARGET_PRODUCT)-{country} \
      ro.com.google.clientidbase.yt=alps-$(TARGET_PRODUCT)-{country} \
      ro.com.google.clientidbase.am=alps-$(TARGET_PRODUCT)-{country} \
      ro.com.google.clientidbase.gmm=alps-$(TARGET_PRODUCT)-{country}
endif
endif

# prebuilt interface
$(call inherit-product-if-exists, vendor/mediatek/common/device-vendor.mk)
# SIP VoIP
$(call inherit-product-if-exists, vendor/mediatek/proprietary/external/sip/sip.mk)

# AEE Config
ifeq ($(HAVE_AEE_FEATURE),yes)
  ifneq ($(MTK_CHIPTEST_INT),yes)
    ifneq ($(wildcard vendor/mediatek/proprietary/external/aee_config_internal/init.aee.mtk.rc),)
$(call inherit-product-if-exists, vendor/mediatek/proprietary/external/aee_config_internal/aee.mk)
    else
$(call inherit-product-if-exists, vendor/mediatek/proprietary/external/aee/config_external/aee.mk)
    endif
  else
$(call inherit-product-if-exists, vendor/mediatek/proprietary/external/aee/config_external/aee.mk)
  endif
endif

# mtklog config
ifeq ($(strip $(MTK_BASIC_PACKAGE)), yes)
ifeq ($(TARGET_BUILD_VARIANT),eng)
PRODUCT_COPY_FILES += device/mediatek/common/mtklog/mtklog-config-basic-eng.prop:system/etc/mtklog-config.prop
else
PRODUCT_COPY_FILES += device/mediatek/common/mtklog/mtklog-config-basic-user.prop:system/etc/mtklog-config.prop
endif
else
ifeq ($(TARGET_BUILD_VARIANT),eng)
PRODUCT_COPY_FILES += device/mediatek/common/mtklog/mtklog-config-bsp-eng.prop:system/etc/mtklog-config.prop
else
PRODUCT_COPY_FILES += device/mediatek/common/mtklog/mtklog-config-bsp-user.prop:system/etc/mtklog-config.prop
endif
endif

# ECC List Customization
$(call inherit-product-if-exists, vendor/mediatek/proprietary/external/EccList/EccList.mk)

#fonts
$(call inherit-product-if-exists, frameworks/base/data/fonts/fonts.mk)
$(call inherit-product-if-exists, external/naver-fonts/fonts.mk)
$(call inherit-product-if-exists, external/noto-fonts/fonts.mk)
$(call inherit-product-if-exists, external/roboto-fonts/fonts.mk)
$(call inherit-product-if-exists, frameworks/base/data/fonts/openfont/fonts.mk)

#3Dwidget
$(call inherit-product-if-exists, vendor/mediatek/proprietary/frameworks/base/3dwidget/appwidget.mk)

# AAPT Config
$(call inherit-product-if-exists, device/mediatek/common/aapt/aapt_config.mk)

#
# MediaTek Operator features configuration
#

ifdef OPTR_SPEC_SEG_DEF
  ifneq ($(strip $(OPTR_SPEC_SEG_DEF)),NONE)
    OPTR := $(word 1,$(subst _,$(space),$(OPTR_SPEC_SEG_DEF)))
    SPEC := $(word 2,$(subst _,$(space),$(OPTR_SPEC_SEG_DEF)))
    SEG  := $(word 3,$(subst _,$(space),$(OPTR_SPEC_SEG_DEF)))
    $(call inherit-product-if-exists, vendor/mediatek/proprietary/operator/$(OPTR)/$(SPEC)/$(SEG)/optr_apk_config.mk)

    PRODUCT_PROPERTY_OVERRIDES += \
      ro.operator.optr=$(OPTR) \
      ro.operator.spec=$(SPEC) \
      ro.operator.seg=$(SEG)
  endif
endif

# add for ATCI JAVA layer service
PRODUCT_PACKAGES += AtciService

PRODUCT_PACKAGES += DataTransfer

# add for OMA DM, common module used by MediatekDM & red bend DM
PRODUCT_PACKAGES += dm_agent_binder

# red bend DM config files & lib
ifeq ($(strip $(MTK_DM_APP)),yes)
    PRODUCT_PACKAGES += reminder.xml
    PRODUCT_PACKAGES += tree.xml
    PRODUCT_PACKAGES += DmApnInfo.xml
    PRODUCT_PACKAGES += vdmconfig.xml
    PRODUCT_PACKAGES += libvdmengine.so
    PRODUCT_PACKAGES += libvdmfumo.so
    PRODUCT_PACKAGES += libvdmlawmo.so
    PRODUCT_PACKAGES += libvdmscinv.so
    PRODUCT_PACKAGES += libvdmscomo.so
    PRODUCT_PACKAGES += dm
endif

# MediatekDM package & lib
ifeq ($(strip $(MTK_MDM_APP)),yes)
    PRODUCT_PACKAGES += MediatekDM
endif

# SmsReg package
ifeq ($(strip $(MTK_SMSREG_APP)),yes)
    PRODUCT_PACKAGES += SmsReg
endif

ifeq ($(strip $(MTK_CMCC_FT_PRECHECK_SUPPORT)),yes)
  PRODUCT_PACKAGES += FTPreCheck
endif

ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),OP09_SPEC0212_SEGDEFAULT)
    PRODUCT_PACKAGES += ConfigureCheck
else
    ifeq ($(strip $(OPTR_SPEC_SEG_DEF)), OP09_SPEC0212_SEGC)
        PRODUCT_PACKAGES += ConfigureCheck
    endif
endif

$(call inherit-product-if-exists, vendor/mediatek/proprietary/frameworks/base/voicecommand/cfg/voicecommand.mk)

ifeq ($(strip $(MTK_VOICE_UNLOCK_SUPPORT)),yes)
    PRODUCT_PACKAGES += VoiceCommand
else
    ifeq ($(strip $(MTK_VOICE_UI_SUPPORT)),yes)
        PRODUCT_PACKAGES += VoiceCommand
    else
        ifeq ($(strip $(MTK_VOICE_CONTACT_SEARCH_SUPPORT)),yes)
            PRODUCT_PACKAGES += VoiceCommand
        else
            ifeq ($(strip $(MTK_VOW_SUPPORT)),yes)
                PRODUCT_PACKAGES += VoiceCommand
            endif
        endif
    endif
endif

ifeq ($(strip $(MTK_VOICE_UNLOCK_SUPPORT)),yes)
    PRODUCT_PACKAGES += VoiceUnlock
else
    ifeq ($(strip $(MTK_VOW_SUPPORT)),yes)
        PRODUCT_PACKAGES += VoiceUnlock
    endif
endif

ifeq ($(strip $(MTK_REGIONALPHONE_SUPPORT)), yes)
  PRODUCT_PACKAGES += RegionalPhoneManager
endif

ifeq ($(strip $(MTK_MDLOGGER_SUPPORT)),yes)
  PRODUCT_PACKAGES += \
    libmdloggerrecycle \
    libmemoryDumpEncoder \
    mdlogger
ifeq ($(strip $(MTK_ENABLE_MD1)), yes)
  PRODUCT_PACKAGES += emdlogger1
endif
ifeq ($(strip $(MTK_ENABLE_MD2)), yes)
  PRODUCT_PACKAGES += emdlogger2
endif
ifeq ($(strip $(MTK_ENABLE_MD5)), yes)
  PRODUCT_PACKAGES += emdlogger5
endif
#  $(call inherit-product-if-exists, vendor/mediatek/proprietary/protect-app/external/emdlogger/usb_port.mk)
  ifneq ($(wildcard device/mediatek/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')/emdlogger_usb_config.prop),)
   PRODUCT_COPY_FILES += device/mediatek/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')/emdlogger_usb_config.prop:system/etc/emdlogger_usb_config.prop
  endif
endif

ifneq ($(strip $(MTK_MDLOGGER_SUPPORT)),yes)

ifeq ($(strip $(MTK_C2K_SUPPORT)), yes)
  PRODUCT_PACKAGES += libmdloggerrecycle
endif

endif

ifeq ($(strip $(MTK_C2K_SUPPORT)), yes)
    PRODUCT_PACKAGES += emdlogger3
    PRODUCT_PACKAGES += c2k-ril-prop
endif

ifeq ($(strip $(MTK_FW_UPGRADE)), yes)
PRODUCT_PACKAGES += FWUpgrade \
                    FWUpgradeProvider
PRODUCT_COPY_FILES += vendor/mediatek/proprietary/packages/apps/FWUpgrade/fotabinder:system/bin/fotabinder
endif

ifeq ($(strip $(MTK_USB_CBA_SUPPORT)), yes)
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_usb_cba_support=1
endif

ifeq ($(strip $(MTK_FOTA_SUPPORT)), yes)
   PRODUCT_PACKAGES += fota1
endif

ifeq ($(strip $(GEMINI)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_gemini_support=1
  PRODUCT_PROPERTY_OVERRIDES += persist.radio.gemini_support=1
endif

ifeq ($(strip $(MTK_SHARE_MODEM_CURRENT)),1)
  PRODUCT_PROPERTY_OVERRIDES += ril.current.share_modem=1
endif
ifeq ($(strip $(MTK_SHARE_MODEM_CURRENT)),2)
  PRODUCT_PROPERTY_OVERRIDES += ril.current.share_modem=2
endif
ifeq ($(strip $(MTK_SHARE_MODEM_CURRENT)),3)
  PRODUCT_PROPERTY_OVERRIDES += ril.current.share_modem=3
endif
ifeq ($(strip $(MTK_SHARE_MODEM_CURRENT)),4)
  PRODUCT_PROPERTY_OVERRIDES += ril.current.share_modem=4
endif


ifeq ($(strip $(MTK_AUDIO_PROFILES)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_audio_profiles=1
endif

ifeq ($(strip $(MTK_AUDENH_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_audenh_support=1
endif

# MTK_LOSSLESS_BT
ifeq ($(strip $(MTK_LOSSLESS_BT_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_lossless_bt_audio=1
endif

# MTK_LOUNDNESS
ifeq ($(strip $(MTK_BESLOUDNESS_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_besloudness_support=1
endif

# MTK_BESSURROUND
ifeq ($(strip $(MTK_BESSURROUND_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_bessurround_support=1
endif

# MTK_ANC
ifeq ($(strip $(MTK_HEADSET_ACTIVE_NOISE_CANCELLATION)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_active_noise_cancel=1
endif

ifeq ($(strip $(MTK_MEMORY_COMPRESSION_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_mem_comp_support=1
endif

ifeq ($(strip $(MTK_WAPI_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wapi_support=1
endif

ifeq ($(strip $(MTK_BT_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_bt_support=1
endif

ifeq ($(strip $(MTK_WAPPUSH_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wappush_support=1
endif

ifeq ($(strip $(MTK_AGPS_APP)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_agps_app=1
endif

ifeq ($(strip $(MTK_FM_TX_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_fm_tx_support=1
endif

ifeq ($(strip $(MTK_VT3G324M_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_vt3g324m_support=1
endif

ifeq ($(strip $(MTK_VOICE_UI_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_voice_ui_support=1
endif

ifeq ($(strip $(MTK_VOICE_UNLOCK_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_voice_unlock_support=1
endif

ifeq ($(strip $(MTK_VOICE_CONTACT_SEARCH_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_voice_contact_support=1
endif

ifneq ($(MTK_AUDIO_TUNING_TOOL_VERSION),)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_audio_tuning_tool_ver=$(strip $(MTK_AUDIO_TUNING_TOOL_VERSION)) 
else
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_audio_tuning_tool_ver=V1
endif

ifeq ($(strip $(MTK_DM_APP)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_dm_app=1
endif

ifeq ($(strip $(MTK_MATV_ANALOG_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_matv_analog_support=1
endif

ifeq ($(strip $(MTK_WLAN_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wlan_support=1
  PRODUCT_PACKAGES += halutil
endif

ifeq ($(strip $(MTK_IPO_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ipo_support=1
endif

ifeq ($(strip $(MTK_GPS_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_gps_support=1
endif

ifeq ($(strip $(MTK_OMACP_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_omacp_support=1
endif

ifeq ($(strip $(HAVE_MATV_FEATURE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.have_matv_feature=1
endif

ifeq ($(strip $(MTK_BT_FM_OVER_BT_VIA_CONTROLLER)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_bt_fm_over_bt=1
endif

ifeq ($(strip $(MTK_SEARCH_DB_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_search_db_support=1
endif

ifeq ($(strip $(MTK_DIALER_SEARCH_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_dialer_search_support=1
endif

ifeq ($(strip $(MTK_DHCPV6C_WIFI)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_dhcpv6c_wifi=1
endif

ifeq ($(strip $(MTK_FM_SHORT_ANTENNA_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_fm_short_antenna_support=1
endif

ifeq ($(strip $(HAVE_AACENCODE_FEATURE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.have_aacencode_feature=1
endif

ifeq ($(strip $(MTK_CTA_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cta_support=1
endif

ifeq ($(strip $(MTK_CLEARMOTION_SUPPORT)),yes)
  PRODUCT_PACKAGES += libMJCjni
    ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),OP01_SPEC0200_SEGC)
        PRODUCT_PROPERTY_OVERRIDES += \
          persist.sys.display.clearMotion=0
    else
        PRODUCT_PROPERTY_OVERRIDES += \
          persist.sys.display.clearMotion=1
    endif
  PRODUCT_PROPERTY_OVERRIDES += \
    persist.clearMotion.fblevel.nrm=255
  PRODUCT_PROPERTY_OVERRIDES += \
    persist.clearMotion.fblevel.bdr=255
endif

ifeq ($(strip $(MTK_PHONE_VT_VOICE_ANSWER)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_phone_vt_voice_answer=1
endif

ifeq ($(strip $(MTK_PHONE_VOICE_RECORDING)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_phone_voice_recording=1
endif

ifeq ($(strip $(MTK_POWER_SAVING_SWITCH_UI_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_pwr_save_switch=1
endif

ifeq ($(strip $(MTK_FD_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_fd_support=1
endif

ifeq ($(strip $(MTK_CC33_SUPPORT)), yes)
# Only support the format: 0: turn off / 1: turn on
    PRODUCT_PROPERTY_OVERRIDES += persist.data.cc33.support=1
endif

#DRM part
ifeq ($(strip $(MTK_DRM_APP)), yes)
  #OMA DRM
  ifeq ($(strip $(MTK_OMADRM_SUPPORT)), yes)
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_oma_drm_support=1
  endif
  #CTA DRM
  ifeq ($(strip $(MTK_CTA_SET)), yes)
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cta_drm_support=1
  endif
endif

#Widevine DRM
ifeq ($(strip $(MTK_WVDRM_SUPPORT)), yes)
  #PRODUCT_PROPERTY_OVERRIDES += ro.mtk_widevine_drm_support=1
  ifeq ($(strip $(MTK_WVDRM_L1_SUPPORT)), yes)
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_widevine_drm_l1_support=1
  else
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_widevine_drm_l3_support=1
  endif
endif

#Playready DRM
ifeq ($(strip $(MTK_PLAYREADY_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_playready_drm_support=1
endif

########
ifeq ($(strip $(MTK_DISABLE_CAPABILITY_SWITCH)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_disable_cap_switch=1
endif

ifeq ($(strip $(MTK_EAP_SIM_AKA)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_eap_sim_aka=1
endif

ifeq ($(strip $(MTK_LOG2SERVER_APP)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_log2server_app=1
endif

ifeq ($(strip $(MTK_FM_RECORDING_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_fm_recording_support=1
endif

ifeq ($(strip $(MTK_AUDIO_APE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_audio_ape_support=1
endif

ifeq ($(strip $(MTK_FLV_PLAYBACK_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_flv_playback_support=1
endif

ifeq ($(strip $(MTK_FD_FORCE_REL_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_fd_force_rel_support=1
endif

ifeq ($(strip $(MTK_BRAZIL_CUSTOMIZATION_CLARO)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.brazil_cust_claro=1
endif

ifeq ($(strip $(MTK_BRAZIL_CUSTOMIZATION_VIVO)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.brazil_cust_vivo=1
endif

ifeq ($(strip $(MTK_WMV_PLAYBACK_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wmv_playback_support=1
endif

ifeq ($(strip $(MTK_HDMI_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_hdmi_support=1
endif

ifeq ($(strip $(MTK_FOTA_ENTRY)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_fota_entry=1
endif

ifeq ($(strip $(MTK_SCOMO_ENTRY)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_scomo_entry=1
endif

ifeq ($(strip $(MTK_MTKPS_PLAYBACK_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_mtkps_playback_support=1
endif

ifeq ($(strip $(MTK_SEND_RR_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_send_rr_support=1
endif

ifeq ($(strip $(MTK_RAT_WCDMA_PREFERRED)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_rat_wcdma_preferred=1
endif

ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),OP09_SPEC0212_SEGDEFAULT)
  PRODUCT_PACKAGES += DeviceRegister
  PRODUCT_PACKAGES += SelfRegister
else

  ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),OP09_SPEC0212_SEGC)
    PRODUCT_PACKAGES += DeviceRegister
    PRODUCT_PACKAGES += SelfRegister
  else

    ifeq ($(strip $(MTK_DEVREG_APP)),yes)
  	  PRODUCT_PACKAGES += DeviceRegister
    endif

    ifeq ($(strip $(MTK_CT4GREG_APP)),yes)
      PRODUCT_PACKAGES += SelfRegister
    endif

  endif
endif

ifeq ($(strip $(MTK_ESN_TRACK_APP)),yes)
  PRODUCT_PACKAGES += EsnTrack
endif

ifeq ($(strip $(MTK_ESN_TRACK_APP)), yes)
  PRODUCT_PROPERTY_OVERRIDES += persist.sys.esn_track_switch=0
endif

ifeq ($(strip $(MTK_SMSREG_APP)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_smsreg_app=1
endif

ifeq ($(strip $(MTK_DEFAULT_DATA_OFF)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_default_data_off=1
endif

ifeq ($(strip $(MTK_TB_APP_CALL_FORCE_SPEAKER_ON)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_tb_call_speaker_on=1
endif

ifeq ($(strip $(MTK_EMMC_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_emmc_support=1
endif

ifeq ($(strip $(MTK_FM_50KHZ_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_fm_50khz_support=1
endif

ifeq ($(strip $(MTK_BSP_PACKAGE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_bsp_package=1
endif

ifeq ($(strip $(MTK_TETHERINGIPV6_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_tetheringipv6_support=1
endif

ifeq ($(strip $(MTK_PHONE_NUMBER_GEODESCRIPTION)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_phone_number_geo=1
endif

ifeq ($(strip $(MTK_DT_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_dt_support=1
endif

ifeq ($(strip $(MTK_C2K_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_c2k_support=1
  PRODUCT_PROPERTY_OVERRIDES += persist.radio.flashless.fsm=0
  PRODUCT_PROPERTY_OVERRIDES += persist.radio.flashless.fsm_cst=0
  PRODUCT_PROPERTY_OVERRIDES += persist.radio.flashless.fsm_rw=0

  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cfu.enable=*72
  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cfu.disable=*720
  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cfb.enable=*90
  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cfb.disable=*900
  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cfnr.enable=*92
  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cfnr.disable=*920
  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cfdf.enable=*68
  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cfdf.disable=*680
  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cfall.disable=*730

  # callWaiting
  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cw.enable=*74
  PRODUCT_PROPERTY_OVERRIDES += ro.cdma.cw.disable=*740

  # network property
   ifeq ($(strip $(MTK_LTE_SUPPORT)), yes)
      # NETWORK_MODE_LTE_CDMA_EVDO_GSM_WCDMA (10)
      PRODUCT_PROPERTY_OVERRIDES += telephony.lteOnCdmaDevice=1
      PRODUCT_PROPERTY_OVERRIDES += ro.telephony.default_network=10,10
   else
      # NETWORK_MODE_GLOBAL(7)
      PRODUCT_PROPERTY_OVERRIDES += ro.telephony.default_network=7,7
   endif
endif

ifneq ($(strip $(MTK_C2K_SUPPORT)), yes)
   ifeq ($(strip $(MTK_LTE_SUPPORT)), yes)
      # NETWORK_MODE_LTE_GSM_WCDMA (9)
      PRODUCT_PROPERTY_OVERRIDES += ro.telephony.default_network=9
   else
      # NETWORK_MODE_WCDMA_PREF(0)
      PRODUCT_PROPERTY_OVERRIDES += ro.telephony.default_network=0
   endif
endif

ifeq ($(strip $(EVDO_DT_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.evdo_dt_support=1
endif

ifeq ($(strip $(EVDO_DT_VIA_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.evdo_dt_via_support=1
endif

ifeq ($(strip $(MTK_SVLTE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_svlte_support=1
  PRODUCT_PROPERTY_OVERRIDES += mtk.md1.status=stop
  PRODUCT_PROPERTY_OVERRIDES += mtk.md3.status=stop
endif

ifeq ($(strip $(MTK_SRLTE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_srlte_support=1
  PRODUCT_PROPERTY_OVERRIDES += mtk.md1.status=stop
  PRODUCT_PROPERTY_OVERRIDES += mtk.md3.status=stop
  PRODUCT_PACKAGES += rilproxy
  PRODUCT_PACKAGES += mtk-rilproxy
  PRODUCT_PACKAGES += lib-rilproxy
endif

ifeq ($(strip $(MTK_SVLTE_LCG_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_svlte_lcg_support=1
endif

ifeq ($(strip $(MTK_IRAT_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.c2k.irat.support=1
endif

ifeq ($(strip $(MTK_C2K_SLOT2_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk.c2k.slot2.support=1
endif

ifeq ($(strip $(MTK_SHARED_SDCARD)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_shared_sdcard=1
endif

ifeq ($(strip $(MTK_2SDCARD_SWAP)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_2sdcard_swap=1
endif

ifeq ($(strip $(MTK_RAT_BALANCING)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_rat_balancing=1
endif

ifeq ($(strip $(WIFI_WEP_KEY_ID_SET)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.wifi_wep_key_id_set=1
endif

ifeq ($(strip $(OP01_COMPATIBLE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.op01_compatible=1
endif

ifeq ($(strip $(MTK_ENABLE_MD1)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_enable_md1=1
endif

ifeq ($(strip $(MTK_ENABLE_MD2)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_enable_md2=1
endif

ifeq ($(strip $(MTK_ENABLE_MD3)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_enable_md3=1
endif

ifeq ($(strip $(MTK_ANDROID_FOR_WORK_SUPPORT)), yes)
    PRODUCT_COPY_FILES += frameworks/native/data/etc/android.software.device_admin.xml:system/etc/permissions/android.software.device_admin.xml
    PRODUCT_COPY_FILES += frameworks/native/data/etc/android.software.managed_users.xml:system/etc/permissions/android.software.managed_users.xml
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_afw_support=1
endif

ifeq ($(strip $(MTK_NETWORK_TYPE_ALWAYS_ON)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_network_type_always_on=1
endif

ifeq ($(strip $(MTK_NFC_ADDON_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_nfc_addon_support=1
endif

ifeq ($(strip $(MTK_BENCHMARK_BOOST_TP)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_benchmark_boost_tp=1
endif

ifeq ($(strip $(MTK_FLIGHT_MODE_POWER_OFF_MD)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_flight_mode_power_off_md=1
endif

ifeq ($(strip $(MTK_BT_BLE_MANAGER_SUPPORT)), yes)
  PRODUCT_PACKAGES += BluetoothLe \
                      BLEManager
endif

#For GattProfile
PRODUCT_PACKAGES += GattProfile

ifeq ($(strip $(MTK_AAL_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_aal_support=1
endif

ifeq ($(strip $(MTK_ULTRA_DIMMING_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ultra_dimming_support=1
endif

ifneq ($(strip $(MTK_PQ_SUPPORT)), no)
    ifeq ($(strip $(MTK_PQ_SUPPORT)), PQ_HW_VER_2)
      PRODUCT_PROPERTY_OVERRIDES += ro.mtk_pq_support=2
    else
        ifeq ($(strip $(MTK_PQ_SUPPORT)), PQ_HW_VER_3)
          PRODUCT_PROPERTY_OVERRIDES += ro.mtk_pq_support=3
        endif
    endif
endif

# pq color mode, default mode is 1 (DISP)
ifeq ($(strip $(MTK_PQ_COLOR_MODE)), OFF)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_pq_color_mode=0
else
  ifeq ($(strip $(MTK_PQ_COLOR_MODE)), MDP)
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_pq_color_mode=2
  else
    ifeq ($(strip $(MTK_PQ_COLOR_MODE)), DISP_MDP)
        PRODUCT_PROPERTY_OVERRIDES += ro.mtk_pq_color_mode=3
    else
        PRODUCT_PROPERTY_OVERRIDES += ro.mtk_pq_color_mode=1
    endif
  endif
endif

ifeq ($(strip $(MTK_MIRAVISION_SETTING_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_miravision_support=1
endif

ifeq ($(strip $(MTK_MIRAVISION_SETTING_SUPPORT)), yes)
  PRODUCT_PACKAGES += MiraVision
endif

ifeq ($(strip $(MTK_MIRAVISION_IMAGE_DC_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_miravision_image_dc=1
endif

ifeq ($(strip $(MTK_BLULIGHT_DEFENDER_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_blulight_def_support=1
endif

ifeq ($(strip $(MTK_TETHERING_EEM_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_tethering_eem_support=1
endif

ifeq ($(strip $(MTK_WFD_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wfd_support=1
endif

ifeq ($(strip $(MTK_WFD_SINK_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wfd_sink_support=1
endif

ifeq ($(strip $(MTK_WFD_SINK_UIBC_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wfd_sink_uibc_support=1
endif

ifeq ($(strip $(MTK_WIFI_MCC_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wifi_mcc_support=1
endif

ifeq ($(strip $(MTK_CROSSMOUNT_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_crossmount_support=1
endif

ifeq ($(strip $(MTK_MULTIPLE_TDLS_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_multiple_tdls_support=1
endif

ifeq ($(strip $(MTK_BEAM_PLUS_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_beam_plus_support=1
endif

ifeq ($(strip $(MTK_MT8193_HDMI_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_mt8193_hdmi_support=1
endif

ifeq ($(strip $(MTK_GEMINI_3SIM_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_gemini_3sim_support=1
endif

ifeq ($(strip $(MTK_GEMINI_4SIM_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_gemini_4sim_support=1
endif

ifeq ($(strip $(MTK_SYSTEM_UPDATE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_system_update_support=1
endif

ifeq ($(strip $(MTK_SIM_HOT_SWAP)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_sim_hot_swap=1
endif

ifeq ($(strip $(MTK_VIDEO_THUMBNAIL_PLAY_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_thumbnail_play_support=1
endif

ifeq ($(strip $(MTK_RADIOOFF_POWER_OFF_MD)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_radiooff_power_off_md=1
endif

ifeq ($(strip $(MTK_BIP_SCWS)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_bip_scws=1
endif

ifeq (OP09_SPEC0212_SEGDEFAULT,$(OPTR_SPEC_SEG_DEF))
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ctpppoe_support=1
endif

ifeq ($(strip $(MTK_IPV6_TETHER_PD_MODE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ipv6_tether_pd_mode=1
endif

ifeq ($(strip $(MTK_CACHE_MERGE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cache_merge_support=1
endif

ifeq ($(strip $(MTK_FAT_ON_NAND)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_fat_on_nand=1
endif

ifeq ($(strip $(MTK_GMO_RAM_OPTIMIZE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_gmo_ram_optimize=1
endif

ifeq ($(strip $(MTK_GMO_ROM_OPTIMIZE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_gmo_rom_optimize=1
endif

ifeq ($(strip $(MTK_CMCC_FT_PRECHECK_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cmcc_ft_precheck_support=1
endif

ifeq ($(strip $(MTK_MDM_APP)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_mdm_app=1
endif

ifeq ($(strip $(MTK_MDM_LAWMO)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_mdm_lawmo=1
endif

ifeq ($(strip $(MTK_MDM_FUMO)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_mdm_fumo=1
endif

ifeq ($(strip $(MTK_MDM_SCOMO)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_mdm_scomo=1
endif

ifeq ($(strip $(MTK_MULTISIM_RINGTONE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_multisim_ringtone=1
endif

ifeq ($(strip $(MTK_MT8193_HDCP_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_mt8193_hdcp_support=1
endif

ifeq ($(strip $(PURE_AP_USE_EXTERNAL_MODEM)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.pure_ap_use_external_modem=1
endif

ifeq ($(strip $(MTK_WFD_HDCP_TX_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wfd_hdcp_tx_support=1
endif

ifeq ($(strip $(MTK_WFD_HDCP_RX_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wfd_hdcp_rx_support=1
endif

ifeq ($(strip $(MTK_WORLD_PHONE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_world_phone=1
endif

ifeq ($(strip $(MTK_WORLD_PHONE_POLICY)), 1)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_world_phone_policy=1
else
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_world_phone_policy=0
endif

ifeq ($(strip $(MTK_MD1_SUPPORT)), 8)
  PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=1
else
  ifeq ($(strip $(MTK_MD1_SUPPORT)), 9)
    PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=1
  else
    ifeq ($(strip $(MTK_MD1_SUPPORT)), 10)
      PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=1
    else
      ifeq ($(strip $(MTK_MD1_SUPPORT)), 11)
        PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=1
      else
        ifeq ($(strip $(MTK_MD1_SUPPORT)), 12)
          PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=1
        else
          ifeq ($(strip $(MTK_MD1_SUPPORT)), 13)
            PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=1
          else
            ifeq ($(strip $(MTK_MD1_SUPPORT)), 14)
              PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=1
            else
              ifeq ($(strip $(MTK_MD1_SUPPORT)), 15)
                PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=1
              else
                ifeq ($(strip $(MTK_MD1_SUPPORT)), 16)
                  PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=1
                else
                  ifeq ($(strip $(MTK_MD1_SUPPORT)), 17)
                    PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=1
                  else
                    PRODUCT_PROPERTY_OVERRIDES +=ro.mtk_md_world_mode_support=0
                  endif
                endif
              endif
            endif
          endif
        endif
      endif
    endif
  endif
endif

ifeq ($(strip $(MTK_PERFSERVICE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_perfservice_support=1
endif

ifeq ($(strip $(MTK_HW_KEY_REMAPPING)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_hw_key_remapping=1
endif

ifeq ($(strip $(MTK_AUDIO_CHANGE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_audio_change_support=1
endif

ifeq ($(strip $(MTK_LOW_BAND_TRAN_ANIM)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_low_band_tran_anim=1
endif

ifeq ($(strip $(MTK_HDMI_HDCP_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_hdmi_hdcp_support=1
endif

ifeq ($(strip $(MTK_INTERNAL_HDMI_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_internal_hdmi_support=1
endif

ifeq ($(strip $(MTK_INTERNAL_MHL_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_internal_mhl_support=1
endif

ifeq ($(strip $(MTK_OWNER_SDCARD_ONLY_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_owner_sdcard_support=1
endif

ifeq ($(strip $(MTK_ONLY_OWNER_SIM_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_owner_sim_support=1
endif

ifeq ($(strip $(MTK_SIM_HOT_SWAP_COMMON_SLOT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_sim_hot_swap_common_slot=1
endif

ifeq ($(strip $(MTK_CTA_SET)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cta_set=1
endif

ifeq ($(strip $(MTK_CTSC_MTBF_INTERNAL_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ctsc_mtbf_intersup=1
endif

ifeq ($(strip $(MTK_3GDONGLE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_3gdongle_support=1
endif

ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),OP09_SPEC0212_SEGDEFAULT)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_devreg_app=1
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ct4greg_app=1
else

  ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),OP09_SPEC0212_SEGC)
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_devreg_app=1
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ct4greg_app=1
  else

    ifeq ($(strip $(MTK_DEVREG_APP)),yes)
      PRODUCT_PROPERTY_OVERRIDES += ro.mtk_devreg_app=1
    endif

    ifeq ($(strip $(MTK_CT4GREG_APP)),yes)
      PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ct4greg_app=1
    endif

  endif
endif

ifeq ($(strip $(EVDO_IR_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.evdo_ir_support=1
endif

ifeq ($(strip $(MTK_MULTI_PARTITION_MOUNT_ONLY_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_multi_patition=1
endif

ifeq ($(strip $(MTK_WIFI_CALLING_RIL_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wifi_calling_ril_support=1
endif

ifeq ($(strip $(MTK_DRM_KEY_MNG_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_key_manager_support=1
endif

ifeq ($(strip $(MTK_DOLBY_DAP_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_dolby_dap_support=1
endif

ifeq ($(strip $(MTK_MOBILE_MANAGEMENT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_mobile_management=1
endif

ifeq ($(strip $(MTK_RUNTIME_PERMISSION_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_runtime_permission=1
endif

ifneq ($(strip $(MTK_ANTIBRICKING_LEVEL)), 0)
  ifeq ($(strip $(MTK_ANTIBRICKING_LEVEL)), 2)
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_antibricking_level=2
  else
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_antibricking_level=1
  endif
endif

# enable zsd+hdr
ifeq ($(strip $(MTK_CAM_ZSDHDR_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_zsdhdr_support=1
endif

# default MFLL support level, [0~4]= off, mfll, ais, both, debug
ifeq ($(strip $(MTK_CAM_MFB_SUPPORT)), 0)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cam_mfb_support=0
endif
ifeq ($(strip $(MTK_CAM_MFB_SUPPORT)), 1)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cam_mfb_support=1
endif
ifeq ($(strip $(MTK_CAM_MFB_SUPPORT)), 2)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cam_mfb_support=2
endif
ifeq ($(strip $(MTK_CAM_MFB_SUPPORT)), 3)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cam_mfb_support=3
endif
ifeq ($(strip $(MTK_CAM_MFB_SUPPORT)), 4)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cam_mfb_support=4
endif

ifeq ($(strip $(MTK_CLEARMOTION_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_clearmotion_support=1
endif

ifeq ($(strip $(MTK_DISPLAY_120HZ_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_display_120hz_support=1
endif

ifeq ($(strip $(MTK_SLOW_MOTION_VIDEO_SUPPORT)), yes)
	PRODUCT_PROPERTY_OVERRIDES += ro.mtk_slow_motion_support=1
	PRODUCT_PACKAGES += libMtkVideoSpeedEffect
	PRODUCT_PACKAGES += libjni_slow_motion
endif

ifeq ($(strip $(MTK_CAM_LOMO_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cam_lomo_support=1
endif

ifeq ($(strip $(MTK_CAM_IMAGE_REFOCUS_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cam_img_refocus_support=1
endif

ifeq ($(strip $(MTK_16X_SLOWMOTION_VIDEO_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_16x_slowmotion_support=1
endif

ifeq ($(strip $(MTK_LTE_DC_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_lte_dc_support=1
endif

ifeq ($(strip $(MTK_LTE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_lte_support=1
endif

ifeq ($(strip $(MTK_ENABLE_MD5)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_enable_md5=1
endif

ifeq ($(strip $(MTK_FEMTO_CELL_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_femto_cell_support=1
endif

ifeq ($(strip $(MTK_SAFEMEDIA_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_safemedia_support=1
endif

ifeq ($(strip $(MTK_UMTS_TDD128_MODE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_umts_tdd128_mode=1
endif

ifeq ($(strip $(MTK_SINGLE_IMEI)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_single_imei=1
endif

ifeq ($(strip $(MTK_SINGLE_3DSHOT_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cam_single_3Dshot_support=1
endif

ifeq ($(strip $(MTK_CAM_MAV_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cam_mav_support=1
endif

ifeq ($(strip $(MTK_CAM_VIDEO_FACEBEAUTY_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_cam_vfb=1
endif

ifeq ($(strip $(MTK_RILD_READ_IMSI)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_rild_read_imsi=1
endif

ifeq ($(strip $(SIM_REFRESH_RESET_BY_MODEM)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.sim_refresh_reset_by_modem=1
endif

ifeq ($(strip $(MTK_EXTERNAL_SIM_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_external_sim_support=1
endif

ifeq ($(strip $(MTK_SUBTITLE_SUPPORT)), yes)
  PRODUCT_PACKAGES += libvobsub_jni
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_subtitle_support=1
endif

ifeq ($(strip $(MTK_DFO_RESOLUTION_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_dfo_resolution_support=1
endif

ifeq ($(strip $(MTK_SMARTBOOK_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_smartbook_support=1
endif

ifeq ($(strip $(MTK_DX_HDCP_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_dx_hdcp_support=1
endif

ifeq ($(strip $(MTK_LIVE_PHOTO_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_live_photo_support=1
endif

ifeq ($(strip $(MTK_MOTION_TRACK_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_motion_track_support=1
endif

ifeq ($(strip $(MTK_SLIDEVIDEO_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_slidevideo_support=1
endif

ifeq ($(strip $(MTK_HOTKNOT_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_hotknot_support=1
endif

ifeq ($(strip $(MTK_PASSPOINT_R1_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_passpoint_r1_support=1
endif

ifeq ($(strip $(MTK_PASSPOINT_R2_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_passpoint_r2_support=1
endif

ifeq ($(strip $(MTK_PRIVACY_PROTECTION_LOCK)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_privacy_protection_lock=1
endif

ifeq ($(strip $(MTK_BG_POWER_SAVING_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_bg_power_saving_support=1
endif

ifeq ($(strip $(MTK_BG_POWER_SAVING_UI_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_bg_power_saving_ui=1
endif

ifeq ($(strip $(MTK_WIFIWPSP2P_NFC_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wifiwpsp2p_nfc_support=1
endif

ifeq ($(strip $(MTK_TC1_FEATURE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_tc1_feature=1
endif

ifeq ($(strip $(MTK_A1_FEATURE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_a1_feature=1
endif

ifeq ($(strip $(HAVE_AEE_FEATURE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.have_aee_feature=1
  PRODUCT_COPY_FILES += vendor/mediatek/proprietary/external/aee/binary/bin/debuggerd:system/bin/debuggerd
  ifeq ($(MTK_K64_SUPPORT), yes)
    PRODUCT_COPY_FILES += vendor/mediatek/proprietary/external/aee/binary/bin/debuggerd64:system/bin/debuggerd64
  endif
endif

ifneq ($(strip $(SIM_ME_LOCK_MODE)),)
  PRODUCT_PROPERTY_OVERRIDES += ro.sim_me_lock_mode=$(strip $(SIM_ME_LOCK_MODE))
else
  PRODUCT_PROPERTY_OVERRIDES += ro.sim_me_lock_mode=0
endif

ifeq ($(strip $(MTK_DUAL_MIC_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_dual_mic_support=1
else
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_dual_mic_support=0
endif

ifeq ($(strip $(MTK_VOICE_UNLOCK_USE_TAB_LIB)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_is_tablet=1
else
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_is_tablet=0
endif

ifeq ($(strip $(MTK_EXTERNAL_MODEM_SLOT)), 1)
  PRODUCT_PROPERTY_OVERRIDES += ril.external.md=1
endif
ifeq ($(strip $(MTK_EXTERNAL_MODEM_SLOT)), 2)
  PRODUCT_PROPERTY_OVERRIDES += ril.external.md=2
endif

# serial port open or not
ifeq ($(strip $(MTK_SERIAL_PORT_DEFAULT_ON)),yes)
ADDITIONAL_DEFAULT_PROPERTIES += persist.service.acm.enable=1
else
ADDITIONAL_DEFAULT_PROPERTIES += persist.service.acm.enable=0
endif

# for pppoe
ifeq (OP09_SPEC0212_SEGDEFAULT,$(OPTR_SPEC_SEG_DEF))
  PRODUCT_PACKAGES += ip-up \
                      ip-down \
                      pppoe \
                      pppoe-server \
                      launchpppoe
  PRODUCT_PROPERTY_OVERRIDES += ro.config.pppoe_enable=1
endif
# for 3rd party app
ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),NONE)
  ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
    ifneq ($(strip $(MTK_A1_FEATURE)), yes)
      PRODUCT_PACKAGES += TouchPal
      PRODUCT_PACKAGES += YahooNewsWidget
    endif
  endif
endif

#For 3rd party NLP provider
PRODUCT_PACKAGES += Baidu_Location

# open TouchPal in OP02
ifeq (OP02,$(word 1,$(subst _, ,$(OPTR_SPEC_SEG_DEF))))
  ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
     PRODUCT_PACKAGES += TouchPal
  endif
endif
# open TouchPal in OP09A
ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),OP09_SPEC0212_SEGDEFAULT)
  ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
     PRODUCT_PACKAGES += TouchPal
  endif
endif

# open TouchPal in OP09C
ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),OP09_SPEC0212_SEGC)
  ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
     PRODUCT_PACKAGES += TouchPal
  endif
endif

# default IME
ifeq (OP01,$(word 1,$(subst _, ,$(OPTR_SPEC_SEG_DEF))))
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_default_ime =com.iflytek.inputmethod.FlyIME
endif

# Data usage overview
ifeq ($(strip $(MTK_DATAUSAGE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_datausage_support=1
endif

# wifi offload service common library
ifneq ($(strip $(MTK_BASIC_PACKAGE)), yes)
    ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
        PRODUCT_PACKAGES += wfo-common
        ifeq ($(strip $(MTK_EPDG_SUPPORT)), yes)
            PRODUCT_PACKAGES += WfoService libwfo_jni
        endif
    endif
endif

# IMS and VoLTE feature
ifeq ($(strip $(MTK_IMS_SUPPORT)), yes)
    ifneq ($(strip $(MTK_BASIC_PACKAGE)), yes)
        ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
            PRODUCT_PACKAGES += ImsService
        endif
    endif
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ims_support=1
endif

#WFC feature
ifeq ($(strip $(MTK_WFC_SUPPORT)),yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_wfc_support=1
  PRODUCT_PROPERTY_OVERRIDES += persist.mtk.wfc.enable=1
  PRODUCT_PROPERTY_OVERRIDES += persist.dbg.wfc_avail_ovr=1
endif

ifeq ($(strip $(MTK_VOLTE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_volte_support=1
  PRODUCT_PROPERTY_OVERRIDES += persist.mtk.volte.enable=1
  PRODUCT_PROPERTY_OVERRIDES += persist.dbg.volte_avail_ovr=1
endif

ifeq ($(strip $(MTK_VILTE_SUPPORT)),yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_vilte_support=1
  PRODUCT_PROPERTY_OVERRIDES += persist.mtk.ims.video.enable=1
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_vilte_ut_support=0
  PRODUCT_PROPERTY_OVERRIDES += persist.dbg.vt_avail_ovr=1
endif

ifeq ($(strip $(MTK_VILTE_SUPPORT)),no)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_vilte_support=0
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_vilte_ut_support=0
  PRODUCT_PROPERTY_OVERRIDES += persist.dbg.vt_avail_ovr=0
endif

ifeq ($(strip $(MTK_USSI_SUPPORT)),yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_ussi_support=1
endif

# DTAG DUAL APN
ifeq ($(strip $(MTK_DTAG_DUAL_APN_SUPPORT)),yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_dtag_dual_apn_support=1
endif

# Telstra PDP retry
ifeq ($(strip $(MTK_TELSTRA_PDP_RETRY_SUPPORT)),yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_fallback_retry_support=1
endif

# sbc security
ifeq ($(strip $(MTK_SECURITY_SW_SUPPORT)), yes)
  PRODUCT_PACKAGES += libsec
  PRODUCT_PACKAGES += sbchk
  PRODUCT_PACKAGES += S_ANDRO_SFL.ini
  PRODUCT_PACKAGES += S_SECRO_SFL.ini
  PRODUCT_PACKAGES += sec_chk.sh
  PRODUCT_PACKAGES += AC_REGION
endif

ifeq ($(strip $(MTK_USER_ROOT_SWITCH)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_user_root_switch=1
endif

ifeq ($(strip $(MTK_DOLBY_DAP_SUPPORT)), yes)
PRODUCT_COPY_FILES += frameworks/av/media/libeffects/data/audio_effects_dolby.conf:system/etc/audio_effects.conf
PRODUCT_COPY_FILES += $(MTK_PROJECT_FOLDER)/dolby/ds1-default.xml:system/etc/ds1-default.xml
else
PRODUCT_COPY_FILES += frameworks/av/media/libeffects/data/audio_effects.conf:system/etc/audio_effects.conf
endif
ifeq ($(strip $(HAVE_SRSAUDIOEFFECT_FEATURE)),yes)
  PRODUCT_COPY_FILES += vendor/mediatek/proprietary/external/SRS_AudioEffect/srs_processing/license/dts.lic:system/data/dts.lic
  PRODUCT_COPY_FILES += vendor/mediatek/proprietary/external/SRS_AudioEffect/srs_processing/srs_processing.cfg:system/data/srs_processing.cfg
endif

ifeq ($(strip $(MTK_PERMISSION_CONTROL)), yes)
  PRODUCT_PACKAGES += PermissionControl
endif

ifeq ($(strip $(MTK_NFC_SUPPORT)), yes)
    ifdef MTK_NFC_PACKAGE
        ifeq ($(wildcard vendor/mediatek/proprietary/hardware/nfc/mtknfc.mk),)
            $(call inherit-product-if-exists, vendor/mediatek/proprietary/hardware/nfc/mtknfc.mk)
        else
            $(call inherit-product-if-exists, vendor/mediatek/proprietary/external/mtknfc/mtknfc.mk)
        endif
    else
        PRODUCT_PACKAGES += nfcstackp
        PRODUCT_PACKAGES += DeviceTestApp
        PRODUCT_PACKAGES += libdta_mt6605_jni
        PRODUCT_PACKAGES += libmtknfc_dynamic_load_jni
        PRODUCT_PACKAGES += libnfc_mt6605_jni
        $(call inherit-product-if-exists, vendor/mediatek/proprietary/packages/apps/DeviceTestApp/DeviceTestApp.mk)
        $(call inherit-product-if-exists, vendor/mediatek/proprietary/external/mtknfc/mtknfc.mk)
    endif
endif

ifeq ($(strip $(MTK_NFC_SUPPORT)), yes)
    ifeq ($(wildcard $(MTK_TARGET_PROJECT_FOLDER)/nfcse.cfg),)
        ifeq ($(strip $(MTK_BSP_PACKAGE)), yes)
            PRODUCT_COPY_FILES += packages/apps/Nfc/mtk-nfc/nfcsebsp.cfg:system/etc/nfcse.cfg
        else
            PRODUCT_COPY_FILES += packages/apps/Nfc/mtk-nfc/nfcsetk.cfg:system/etc/nfcse.cfg
        endif
    else
        PRODUCT_COPY_FILES += $(MTK_TARGET_PROJECT_FOLDER)/nfcse.cfg:system/etc/nfcse.cfg
    endif
endif

ifeq (yes,$(strip $(MTK_NFC_SUPPORT)))

  PRODUCT_COPY_FILES += $(call add-to-product-copy-files-if-exists,frameworks/native/data/etc/android.hardware.nfc.xml:system/etc/permissions/android.hardware.nfc.xml)

  ifneq ($(MTK_BSP_PACKAGE), yes)
    PRODUCT_COPY_FILES +=$(call add-to-product-copy-files-if-exists,frameworks/base/nfc-extras/com.android.nfc_extras.xml:system/etc/permissions/com.android.nfc_extras.xml)
    PRODUCT_COPY_FILES +=$(call add-to-product-copy-files-if-exists,packages/apps/Nfc/etc/nfcee_access.xml:system/etc/nfcee_access.xml)
    ifeq ($(MTK_NFC_GSMA_SUPPORT), yes)
        PRODUCT_PACKAGES += com.android.nfcgsma_extras
        PRODUCT_PACKAGES += com.gsma.services.nfc
        PRODUCT_COPY_FILES +=$(call add-to-product-copy-files-if-exists,frameworks/base/gsma-extras/com.android.nfcgsma_extras.xml:system/etc/permissions/com.android.nfcgsma_extras.xml)
        PRODUCT_COPY_FILES +=$(call add-to-product-copy-files-if-exists,packages/apps/Nfc/gsma/jar/com.gsma.services.nfc.xml:system/etc/permissions/com.gsma.services.nfc.xml)
        PRODUCT_COPY_FILES +=$(call add-to-product-copy-files-if-exists,packages/apps/Nfc/gsma/jar/com.gsma.services.nfc.jar:system/framework/com.gsma.services.nfc.jar)

        ifeq ($(wildcard $(MTK_TARGET_PROJECT_FOLDER)/gsma.cfg),)
            PRODUCT_COPY_FILES += packages/apps/Nfc/gsma/gsma.cfg:system/etc/gsma.cfg
        else
            PRODUCT_COPY_FILES += $(MTK_TARGET_PROJECT_FOLDER)/gsma.cfg:system/etc/gsma.cfg
  endif

        PRODUCT_PROPERTY_OVERRIDES += ro.mtk_nfc_gsma_support=1
    endif
  endif

  PRODUCT_PACKAGES += Nfc
  PRODUCT_PACKAGES += Tag
  PRODUCT_PACKAGES += nfcc.default
  PRODUCT_PROPERTY_OVERRIDES +=  ro.nfc.port=I2C

  ifeq (yes,$(strip $(MTK_NFC_HCE_SUPPORT)))
    PRODUCT_COPY_FILES += $(call add-to-product-copy-files-if-exists,frameworks/native/data/etc/android.hardware.nfc.hce.xml:system/etc/permissions/android.hardware.nfc.hce.xml)
  endif

endif


ifeq ($(strip $(MTK_NFC_OMAAC_SUPPORT)),yes)
  PRODUCT_PACKAGES += SmartcardService
  PRODUCT_PACKAGES += org.simalliance.openmobileapi.jar
  PRODUCT_PACKAGES += org.simalliance.openmobileapi.xml
  PRODUCT_PACKAGES += eSETerminal
  PRODUCT_PACKAGES += Uicc1Terminal
  PRODUCT_PACKAGES += Uicc2Terminal
endif

ifeq ($(strip $(MTK_HOTKNOT_SUPPORT)), yes)
  PRODUCT_PACKAGES += libhotknot_GT1XX
  PRODUCT_PACKAGES += libhotknot_GT9XX
  PRODUCT_PROPERTY_OVERRIDES += ro.mediatek.hotknot.module=$(CUSTOM_KERNEL_TOUCHPANEL)
endif
ifeq ($(strip $(MTK_HOTKNOT_SUPPORT)), yes)
  PRODUCT_PACKAGES += HotKnot
  PRODUCT_PACKAGES += HotKnotBeam
  PRODUCT_PACKAGES += HotKnotCommonUI
  PRODUCT_PACKAGES += HotKnotConnectivity
  PRODUCT_PACKAGES += hotknot_native_service
  PRODUCT_PACKAGES += libhotknot_dev

    ifeq ($(wildcard $(MTK_TARGET_PROJECT_FOLDER)/hotknot.cfg),)
        PRODUCT_COPY_FILES += vendor/mediatek/proprietary/packages/apps/HotKnot/hotknot.cfg:system/etc/hotknot.cfg
    else
        PRODUCT_COPY_FILES += $(MTK_TARGET_PROJECT_FOLDER)/hotknot.cfg:system/etc/hotknot.cfg
    endif
endif

ifeq ($(strip $(MTK_CROSSMOUNT_SUPPORT)),yes)
  PRODUCT_PACKAGES += com.mediatek.crossmount.discovery
  PRODUCT_PACKAGES += com.mediatek.crossmount.discovery.xml
  PRODUCT_PACKAGES += CrossMount
  PRODUCT_PACKAGES += com.mediatek.crossmount.adapter
  PRODUCT_PACKAGES += com.mediatek.crossmount.adapter.xml
  PRODUCT_PACKAGES += CrossMountSettings
  PRODUCT_PACKAGES += CrossMountSourceCamera
  PRODUCT_PACKAGES += libcrossmount
  PRODUCT_PACKAGES += libcrossmount_jni
  PRODUCT_PACKAGES += sensors.virtual
  PRODUCT_PACKAGES += SWMountViewer
endif

$(call inherit-product-if-exists, frameworks/base/data/videos/FrameworkResource.mk)
ifeq ($(strip $(MTK_LIVE_PHOTO_SUPPORT)), yes)
  PRODUCT_PACKAGES += com.mediatek.effect
  PRODUCT_PACKAGES += com.mediatek.effect.xml
endif

ifeq ($(strip $(MTK_MULTICORE_OBSERVER_APP)), yes)
  PRODUCT_PACKAGES += MultiCoreObserver
endif

# for Search, ApplicationsProvider provides apps search
PRODUCT_PACKAGES += ApplicationsProvider

# Live wallpaper configurations
# #workaround: disable it directly since device.mk can't get the value of TARGET_BUILD_PDK
PRODUCT_COPY_FILES += packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml

# for JPE
PRODUCT_PACKAGES += jpe_tool

# for mmsdk
PRODUCT_PACKAGES += mmsdk.$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')

ifneq ($(strip $(MTK_PLATFORM)),)
  PRODUCT_PACKAGES += libnativecheck-jni
endif

# for mediatek-res
PRODUCT_PACKAGES += mediatek-res

# for TER service
PRODUCT_PACKAGES += terservice
PRODUCT_PACKAGES += tertestclient
ifeq ($(strip $(MTK_TER_SERVICE)),yes)
  PRODUCT_PROPERTY_OVERRIDES += ter.service.enable=1
endif

#SDK: Voice Interface Extension
ifeq ($(strip $(MTK_VOICE_INTERFACE_EXTENSION_SUPPORT)),yes)
PRODUCT_PROPERTY_OVERRIDES += ro.mtk_voice_extension_support=1
endif

#SDK: Voice Interface Extension
ifeq ($(strip $(MTK_VOICE_INTERFACE_EXTENSION_SUPPORT)),yes)
$(call inherit-product-if-exists, vendor/mediatek/proprietary/frameworks/base/voiceextension/cfg/voiceextension.mk)
PRODUCT_PACKAGES += VoiceExtension
endif
PRODUCT_PACKAGES += libvie
PRODUCT_PACKAGES += libvie_jni

# For Native downloader
PRODUCT_PACKAGES += downloader
ifeq ($(strip $(MTK_DT_SUPPORT)), yes)
  $(call inherit-product-if-exists, vendor/mediatek/proprietary/external/downloader/downloader.mk)
endif

# for RecoveryManagerService
PRODUCT_PACKAGES += \
    recovery \
    recovery.xml

PRODUCT_PROPERTY_OVERRIDES += wfd.dummy.enable=1

PRODUCT_PROPERTY_OVERRIDES += ro.mediatek.project.path=$(shell find device/* -maxdepth 1 -name $(subst full_,,$(TARGET_PRODUCT)))


ifeq ($(strip $(MTK_C2K_SUPPORT)), yes)
   PRODUCT_PACKAGES += Utk
endif

ifeq ($(strip $(EVDO_IR_SUPPORT)),yes)
  PRODUCT_PROPERTY_OVERRIDES += \
    ril.evdo.irsupport=1
endif

ifeq ($(strip $(EVDO_DT_SUPPORT)),yes)
  PRODUCT_PROPERTY_OVERRIDES += \
    ril.evdo.dtsupport=1
endif

# for libudf
ifeq ($(strip $(MTK_USER_SPACE_DEBUG_FW)),yes)
PRODUCT_PACKAGES += libudf
endif

PRODUCT_COPY_FILES += $(MTK_TARGET_PROJECT_FOLDER)/ProjectConfig.mk:system/data/misc/ProjectConfig.mk

ifeq ($(strip $(MTK_BICR_SUPPORT)), yes)
PRODUCT_COPY_FILES += device/mediatek/common/iAmCdRom.iso:system/etc/iAmCdRom.iso
endif

PRODUCT_COPY_FILES += $(call add-to-product-copy-files-if-exists,vendor/mediatek/proprietary/frameworks/base/telephony/etc/virtual-spn-conf-by-efgid1.xml:system/etc/virtual-spn-conf-by-efgid1.xml)
PRODUCT_COPY_FILES += $(call add-to-product-copy-files-if-exists,vendor/mediatek/proprietary/frameworks/base/telephony/etc/virtual-spn-conf-by-efpnn.xml:system/etc/virtual-spn-conf-by-efpnn.xml)
PRODUCT_COPY_FILES += $(call add-to-product-copy-files-if-exists,vendor/mediatek/proprietary/frameworks/base/telephony/etc/virtual-spn-conf-by-efspn.xml:system/etc/virtual-spn-conf-by-efspn.xml)
PRODUCT_COPY_FILES += $(call add-to-product-copy-files-if-exists,vendor/mediatek/proprietary/frameworks/base/telephony/etc/virtual-spn-conf-by-imsi.xml:system/etc/virtual-spn-conf-by-imsi.xml)

ifeq ($(strip $(MTK_AUDIO_ALAC_SUPPORT)), yes)
  PRODUCT_PACKAGES += libMtkOmxAlacDec
endif

ifeq ($(strip $(TRUSTONIC_TEE_SUPPORT)), yes)
  PRODUCT_PACKAGES += libMcClient
  PRODUCT_PACKAGES += libMcRegistry
  PRODUCT_PACKAGES += mcDriverDaemon
  PRODUCT_PACKAGES += libsec_mem
  PRODUCT_PACKAGES += libMcTeeKeymaster
  PRODUCT_PACKAGES += libMtkH264SecVencTLCLib
  PRODUCT_PACKAGES += libMtkH264SecVdecTLCLib
  PRODUCT_PACKAGES += libtlcWidevineModularDrm
  PRODUCT_PACKAGES += libtlcWidevineClassicDrm
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_trustonic_tee_support=1
endif

ifeq ($(strip $(MTK_GOOGLE_TRUSTY_SUPPORT)), yes)
  PRODUCT_PACKAGES += gatekeeper.trusty
  PRODUCT_PACKAGES += keystore.trusty
  PRODUCT_PACKAGES += libtrusty
endif

ifeq ($(strip $(MICROTRUST_TEE_SUPPORT)), yes)
  PRODUCT_PACKAGES += teei_daemon
  PRODUCT_PACKAGES += init_thh
#  PRODUCT_PACKAGES += libteei_fp
#  PRODUCT_PACKAGES += libfingerprint_tac
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_microtrust_tee_support=1
endif

ifeq ($(strip $(MTK_SEC_VIDEO_PATH_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_sec_video_path_support=1
  ifeq ($(filter $(MTK_IN_HOUSE_TEE_SUPPORT) $(MTK_GOOGLE_TRUSTY_SUPPORT),yes),yes)
  PRODUCT_PACKAGES += lib_uree_mtk_video_secure_al
  endif
endif
ifeq ($(strip $(MTK_COMBO_SUPPORT)), yes)
  $(call inherit-product-if-exists, device/mediatek/common/connectivity/product_package/product_package.mk)
endif

$(call inherit-product-if-exists, vendor/mediatek/proprietary/hardware/spm/product_package.mk)

ifeq ($(strip $(MTK_SENSOR_HUB_SUPPORT)),yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_sensorhub_support=1
  PRODUCT_PACKAGES += libhwsensorhub \
                      libsensorhub \
                      libsensorhub_jni \
                      sensorhubservice \
                      libsensorhubservice
endif

ifeq ($(strip $(MTK_TC7_FEATURE)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_tc7_feature=1
endif

PRODUCT_PACKAGES += Launcher3

#Add for CCCI Lib
PRODUCT_PACKAGES += libccci_util

ifeq ($(strip $(MTK_WMA_PLAYBACK_SUPPORT)), yes)
  PRODUCT_PACKAGES += libMtkOmxWmaDec
endif

ifeq ($(strip $(MTK_WMA_PLAYBACK_SUPPORT))_$(strip $(MTK_SWIP_WMAPRO)), yes_yes)
  PRODUCT_PACKAGES += libMtkOmxWmaProDec
endif

# ePDG
PRODUCT_PACKAGES += epdg_wod

# IKEv2
ifeq ($(strip $(MTK_EPDG_SUPPORT)), yes)
  PRODUCT_COPY_FILES += vendor/mediatek/proprietary/external/strongswan/epdg_conf/ipsec.conf:system/etc/ipsec/ipsec.conf
  PRODUCT_COPY_FILES += vendor/mediatek/proprietary/external/strongswan/epdg_conf/strongswan.conf:system/etc/ipsec/strongswan.conf
  PRODUCT_COPY_FILES += vendor/mediatek/proprietary/external/strongswan/epdg_conf/updown_script:system/etc/ipsec/updown_script
  PRODUCT_COPY_FILES += vendor/mediatek/proprietary/external/strongswan/epdg_conf/openssl.cnf:system/etc/ipsec/ssl/openssl.cnf

  ifeq ($(strip $(MTK_CIP_SUPPORT)), no)
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_epdg_support=1
    PRODUCT_COPY_FILES += vendor/mediatek/proprietary/external/strongswan/epdg_conf/Entrust.net_Certification_Authority_2048.cer:system/etc/ipsec/ipsec.d/cacerts/CA1.cer
    PRODUCT_COPY_FILES += vendor/mediatek/proprietary/external/strongswan/epdg_conf/test2_ca.crt:system/etc/ipsec/ipsec.d/cacerts/CA1L1.crt
  endif  

  PRODUCT_PACKAGES += charon \
                    libcharon \
                    libhydra \
                    libstrongswan \
                    libsimaka \
                    starter \
                    stroke \
                    ipsec
  ifneq ($(wildcard device/mediatek/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')/init.epdg.rc),)
    PRODUCT_COPY_FILES += device/mediatek/$(shell echo $(MTK_PLATFORM) | tr '[A-Z]' '[a-z]')/init.epdg.rc:root/init.epdg.rc
  else
    PRODUCT_COPY_FILES += device/mediatek/common/init.epdg.rc:root/init.epdg.rc
  endif
endif

ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
  ifneq ($(strip $(MTK_BASIC_PACKAGE)), yes)
    PRODUCT_PROPERTY_OVERRIDES += ro.com.android.mobiledata=false
  else
    PRODUCT_PROPERTY_OVERRIDES += ro.com.android.mobiledata=true
  endif
else
    PRODUCT_PROPERTY_OVERRIDES += ro.com.android.mobiledata=true
endif

PRODUCT_PROPERTY_OVERRIDES += persist.radio.mobile.data=0,0
#for meta mode dump data
PRODUCT_PROPERTY_OVERRIDES += persist.meta.dumpdata=0

ifneq ($(MTK_AUDIO_TUNING_TOOL_VERSION),)
  ifneq ($(strip $(MTK_AUDIO_TUNING_TOOL_VERSION)),V1)
    PRODUCT_PACKAGES += libaudio_param_parser
    AUDIO_PARAM_OPTIONS_LIST += 5_POLE_HS_SUPPORT=$(MTK_HEADSET_ACTIVE_NOISE_CANCELLATION)
    MTK_AUDIO_PARAM_DIR_LIST += device/mediatek/common/audio_param
    #MTK_AUDIO_PARAM_FILE_LIST += SOME_ZIP_FILE

    # speaker path customization for gain table
    ifeq ($(strip $(MTK_AUDIO_SPEAKER_PATH)),int_spk_amp)
      AUDIO_PARAM_OPTIONS_LIST += SPK_PATH_INT=yes
    else ifeq ($(strip $(MTK_AUDIO_SPEAKER_PATH)),int_lo_buf)
      AUDIO_PARAM_OPTIONS_LIST += SPK_PATH_LO=yes
    else ifeq ($(strip $(MTK_AUDIO_SPEAKER_PATH)),int_hp_buf)
      AUDIO_PARAM_OPTIONS_LIST += SPK_PATH_HP=yes
    else
      AUDIO_PARAM_OPTIONS_LIST += SPK_PATH_NO_ANA=yes
    endif

  endif
endif

ifeq ($(strip $(MTK_NTFS_OPENSOURCE_SUPPORT)), yes)
  PRODUCT_PACKAGES += ntfs-3g
  PRODUCT_PACKAGES += ntfsfix
endif

# Add for HetComm feature
ifeq ($(strip $(MTK_HETCOMM_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_hetcomm_support=1
  PRODUCT_PACKAGES += HetComm
endif

ifeq ($(strip $(MTK_DEINTERLACE_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_deinterlace_support=1
endif

ifeq ($(strip $(MTK_GPS_SUPPORT)), yes)
  PRODUCT_PACKAGES += NlpService
endif

ifeq ($(strip $(MTK_CDS_EM_SUPPORT)),yes)
    PRODUCT_PACKAGES += CDS_INFO
endif
ifeq ($(strip $(MTK_DOLBY_DAP_SUPPORT)),yes)

DOLBY_DAX_VERSION            := 1
DOLBY_DAP                    := true
DOLBY_DAP2                   := false
DOLBY_DAP_SW                 := true
DOLBY_DAP_HW                 := false
DOLBY_DAP_PREGAIN            := true
DOLBY_DAP_HW_QDSP_HAL_API    := false
DOLBY_DAP_MOVE_EFFECT        := true
DOLBY_DAP_BYPASS_SOUND_TYPES := false
DOLBY_CONSUMER_APP           := true
DOLBY_UDC                    := true
DOLBY_UDC_VIRTUALIZE_AUDIO   := false
DOLBY_MONO_SPEAKER           := true

include vendor/dolby/ds/dolby-buildspec.mk
$(call inherit-product, vendor/dolby/ds/dolby-product.mk)

PRODUCT_COPY_FILES := \
    vendor/dolby/device/mediatek_sw/audio_effects.conf:system/vendor/etc/audio_effects.conf:dolby \
    vendor/dolby/device/mediatek_sw/ds1-default.xml:system/vendor/etc/dolby/ds1-default.xml:dolby \
    $(PRODUCT_COPY_FILES)

PRODUCT_RESTRICT_VENDOR_FILES := false
endif

#Fix me: specific enable for build error workaround
SKIP_BOOT_JARS_CHECK := true

ifeq ($(strip $(MTK_SWITCH_ANTENNA_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_switch_antenna=1
endif

ifeq ($(strip $(MTK_TDD_DATA_ONLY_SUPPORT)), yes)
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_tdd_data_only_support=1
endif

# Add for C2K OM MODE
ifeq ($(strip $(MTK_C2K_OM_MODE)), CLLWTG)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk.c2k.om.mode=cllwtg
endif
ifeq ($(strip $(MTK_C2K_OM_MODE)), CLLWG)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk.c2k.om.mode=cllwg
endif
ifeq ($(strip $(MTK_C2K_OM_MODE)), CLLG)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk.c2k.om.mode=cllg
endif
ifeq ($(strip $(MTK_C2K_OM_MODE)), CWG)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk.c2k.om.mode=cwg
endif

ifneq ($(strip $(MTK_MD_SBP_CUSTOM_VALUE)),)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_md_sbp_custom_value=$(strip $(MTK_MD_SBP_CUSTOM_VALUE))
endif

# Add for C2K OM Network Selection Type
ifeq ($(strip $(MTK_C2K_OM_NW_SEL_TYPE)), 0)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_c2k_om_nw_sel_type=0
endif
ifeq ($(strip $(MTK_C2K_OM_NW_SEL_TYPE)), 1)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_c2k_om_nw_sel_type=1
endif

# Add for Automatic Setting for heapgrowthlimit & heapsize
RESOLUTION_HXW=$(shell expr $(LCM_HEIGHT) \* $(LCM_WIDTH))

ifeq ($(shell test $(RESOLUTION_HXW) -ge 0 && test $(RESOLUTION_HXW) -lt 1000000 && echo true), true)
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.heapgrowthlimit=128m
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.heapsize=256m
endif

ifeq ($(shell test $(RESOLUTION_HXW) -ge 1000000 && test $(RESOLUTION_HXW) -lt 3500000 && echo true), true)
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.heapgrowthlimit=192m
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.heapsize=384m
endif

ifeq ($(shell test $(RESOLUTION_HXW) -ge 3500000 && echo true), true)
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.heapgrowthlimit=384m
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.heapsize=768m
endif

#Add for BSP package SIP VoIP handle
ifeq ($(MTK_BSP_PACKAGE),yes)
ifeq ($(MTK_SIP_SUPPORT),yes)
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.software.sip.xml:system/etc/permissions/android.software.sip.xml
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml
endif
endif

# Add for Hardware Fused Location Related Modules
PRODUCT_PACKAGES += slpd
ifneq ($(TARGET_BUILD_VARIANT), user)
  PRODUCT_PACKAGES += FlpEM2
endif
PRODUCT_COPY_FILES += device/mediatek/common/slp/slp_conf:system/etc/slp_conf

PRODUCT_PACKAGES += CarrierConfig

ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),OP09_SPEC0212_SEGDEFAULT)
  PRODUCT_COPY_FILES += vendor/mediatek/proprietary/frameworks/base/telephony/etc/spn-conf-op09.xml:system/etc/spn-conf-op09.xml
endif

ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),OP09_SPEC0212_SEGC)
  PRODUCT_COPY_FILES += vendor/mediatek/proprietary/frameworks/base/telephony/etc/spn-conf-op09.xml:system/etc/spn-conf-op09.xml
endif

# Add for SensorHub
PRODUCT_PACKAGES += SensorHub

# Add for common service initialization
PRODUCT_COPY_FILES += device/mediatek/common/init.common_svc.rc:root/init.common_svc.rc

# M: GMO Zygote64 on demand @{
ifeq ($(strip $(MTK_GMO_ZYGOTE_ON_DEMAND)), yes)
    # 1 = support secondary zygote on demand, 0 = not support
    PRODUCT_PROPERTY_OVERRIDES += ro.mtk_gmo_zygote_on_demand=1

    # 1 = full preload, 0 = no preload
    PRODUCT_PROPERTY_OVERRIDES += persist.sys.mtk_zygote_preload=0

    # the time to stop secondary zygote after the last forked process has been killed, in seconds
    PRODUCT_PROPERTY_OVERRIDES += persist.sys.mtk_zygote_timeout=60

    # 1 = enable debug, 0 = disable debug
    PRODUCT_PROPERTY_OVERRIDES += persist.sys.mtk_zygote_debug=0

    # include init.zygote_on_demand.rc
    PRODUCT_COPY_FILES += device/mediatek/common/init.zygote_on_demand.rc:root/init.zygote_on_demand.rc
endif
# M: GMO Zygote64 on demand @}

# Add for (VzW) chipset test
ifneq ($(MTK_VZW_CHIPTEST_MODE_SUPPORT),)
  #we dont support yes, only support 0,1,2
  ifeq (, $(filter 0 no,$(MTK_VZW_CHIPTEST_MODE_SUPPORT)))
    PRODUCT_COPY_FILES += device/mediatek/common/init.chiptest.rc:root/init.chiptest.rc
    ifeq ($(strip $(MTK_VZW_CHIPTEST_MODE_SUPPORT)), 1)
      PRODUCT_PROPERTY_OVERRIDES += persist.chiptest.enable=1
    endif
  endif
endif

# Add for Dynamic-SBP
ifeq ($(strip $(MTK_DYNAMIC_SBP_SUPPORT)), yes)
  PRODUCT_PROPERTY_OVERRIDES += persist.radio.mtk_dsbp_support=1
endif

ifeq (yes,$(strip $(MTK_BG_POWER_SAVING_SUPPORT)))
    ifeq (true,$(strip $(MTK_ALARM_AWARE_UPLINK_SUPPORT)))
        PRODUCT_PROPERTY_OVERRIDES += persist.mtk.datashaping.support=1
        PRODUCT_PROPERTY_OVERRIDES += persist.datashaping.alarmgroup=1
    endif
endif

# Add for ModemMonitor(MDM) framework
ifeq ($(strip $(MTK_MODEM_MONITOR_SUPPORT)),yes)
  PRODUCT_PROPERTY_OVERRIDES += ro.mtk_modem_monitor_support=1
  PRODUCT_PACKAGES += \
    md_monitor \
    md_monitor_ctrl \
    MDMLSample
endif

# Add for BRM superset
ifeq ($(strip $(RELEASE_BRM)), yes)
  include device/mediatek/common/releaseBRM.mk
endif

# Add for Contacts AAS and SNE
ifneq ($(strip $(MTK_BASIC_PACKAGE)), yes)
    ifneq ($(strip $(MTK_BSP_PACKAGE)), yes)
        ifeq (OP03,$(word 1,$(subst _, ,$(OPTR_SPEC_SEG_DEF))))
            PRODUCT_PACKAGES += AasSne
        else
            ifeq (OP06,$(word 1,$(subst _, ,$(OPTR_SPEC_SEG_DEF))))
                 PRODUCT_PACKAGES += AasSne
            endif
        endif
    endif
endif
