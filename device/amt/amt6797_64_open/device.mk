include device/amt/$(MTK_TARGET_PROJECT)/ProjectConfig.mk

# Example: Re-use system.prop from base project
TARGET_SYSTEM_PROP := device/amt/amt6797_64_open/system.prop

######################################################

# PRODUCT_COPY_FILES overwrite
# Please add flavor project's PRODUCT_COPY_FILES here.
# It will overwrite base project's PRODUCT_COPY_FILES.
PRODUCT_COPY_FILES += device/amt/$(MTK_TARGET_PROJECT)/fstab.mt6797:root/fstab.mt6797

# overlay has priorities. high <-> low.
DEVICE_PACKAGE_OVERLAYS += device/amt/$(MTK_TARGET_PROJECT)/overlay

#media_profiles.xml for media profile support
PRODUCT_COPY_FILES += device/amt/amt6797_64_open/media_profiles.xml:system/etc/media_profiles.xml


PRODUCT_COPY_FILES += device/amt/amt6797_64_open/factory_init.project.rc:root/factory_init.project.rc
PRODUCT_COPY_FILES += device/amt/amt6797_64_open/init.project.rc:root/init.project.rc
PRODUCT_COPY_FILES += device/amt/amt6797_64_open/meta_init.project.rc:root/meta_init.project.rc


# alps/vendor/mediatek/proprietary/frameworks-ext/native/etc/Android.mk
# sensor related xml files for CTS
ifneq ($(strip $(CUSTOM_KERNEL_ACCELEROMETER)),)
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml
endif

ifneq ($(strip $(CUSTOM_KERNEL_MAGNETOMETER)),)
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml
endif

ifneq ($(strip $(CUSTOM_KERNEL_ALSPS)),)
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml
else
  ifneq ($(strip $(CUSTOM_KERNEL_PS)),)
    PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml
  endif
  ifneq ($(strip $(CUSTOM_KERNEL_ALS)),)
    PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml
  endif
endif

ifneq ($(strip $(CUSTOM_KERNEL_GYROSCOPE)),)
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml
endif

ifneq ($(strip $(CUSTOM_KERNEL_BAROMETER)),)
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.sensor.barometer.xml:system/etc/permissions/android.hardware.sensor.barometer.xml
endif

# touch related file for CTS
ifeq ($(strip $(CUSTOM_KERNEL_TOUCHPANEL)),generic)
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.touchscreen.xml:system/etc/permissions/android.hardware.touchscreen.xml
else
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.faketouch.xml:system/etc/permissions/android.hardware.faketouch.xml
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.touchscreen.multitouch.distinct.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.distinct.xml
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.xml
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.touchscreen.xml:system/etc/permissions/android.hardware.touchscreen.xml
endif

# USB OTG
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml

# GPS relative file
ifeq ($(MTK_GPS_SUPPORT),yes)
  PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml
endif

# alps/frameworks/av/media/libeffects/factory/Android.mk
PRODUCT_COPY_FILES += frameworks/av/media/libeffects/data/audio_effects.conf:system/etc/audio_effects.conf

# alps/mediatek/config/$project
PRODUCT_COPY_FILES += device/amt/amt6797_64_open/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml


# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.sys.usb.config=mtp
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.service.acm.enable=0
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += ro.mount.fs=EXT4

PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.heapgrowthlimit=128m
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.heapsize=256m

# meta tool
PRODUCT_PROPERTY_OVERRIDES += ro.mediatek.chip_ver=S01
PRODUCT_PROPERTY_OVERRIDES += ro.mediatek.platform=MT6797

# set Telephony property - SIM count
SIM_COUNT := 2
PRODUCT_PROPERTY_OVERRIDES += ro.telephony.sim.count=$(SIM_COUNT)
PRODUCT_PROPERTY_OVERRIDES += persist.radio.default.sim=0

ifeq ($(GEMINI),yes)
  ifeq ($(MTK_DT_SUPPORT),yes)
    PRODUCT_PROPERTY_OVERRIDES += persist.radio.multisim.config=dsda
  else
    ifeq ($(MTK_SVLTE_SUPPORT),yes)
      PRODUCT_PROPERTY_OVERRIDES += persist.radio.multisim.config=dsda
    else
      PRODUCT_PROPERTY_OVERRIDES += persist.radio.multisim.config=dsds
    endif
  endif
else
  PRODUCT_PROPERTY_OVERRIDES += persist.radio.multisim.config=ss
endif


# Keyboard layout
PRODUCT_COPY_FILES += device/mediatek/mt6797/ACCDET.kl:system/usr/keylayout/ACCDET.kl

# Microphone
PRODUCT_COPY_FILES += device/amt/amt6797_64_open/android.hardware.microphone.xml:system/etc/permissions/android.hardware.microphone.xml

# Camera
PRODUCT_COPY_FILES += device/amt/amt6797_64_open/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml

# Audio Policy
PRODUCT_COPY_FILES += device/amt/amt6797_64_open/audio_policy.conf:system/etc/audio_policy.conf


# overlay has priorities. high <-> low.

DEVICE_PACKAGE_OVERLAYS += device/mediatek/common/overlay/sd_in_ex_otg

DEVICE_PACKAGE_OVERLAYS += device/amt/amt6797_64_open/overlay

ifneq (yes,$(strip $(MTK_TABLET_PLATFORM)))
  ifeq (480,$(strip $(LCM_WIDTH)))
    ifeq (854,$(strip $(LCM_HEIGHT)))
      DEVICE_PACKAGE_OVERLAYS += device/mediatek/common/overlay/FWVGA
    endif
  endif
  ifeq (540,$(strip $(LCM_WIDTH)))
    ifeq (960,$(strip $(LCM_HEIGHT)))
      DEVICE_PACKAGE_OVERLAYS += device/mediatek/common/overlay/qHD
    endif
  endif
endif

DEVICE_PACKAGE_OVERLAYS += device/mediatek/common/overlay/navbar

ifeq ($(strip $(OPTR_SPEC_SEG_DEF)),NONE)
    PRODUCT_PACKAGES += DangerDash
endif

#SPM loader
PRODUCT_PACKAGES += spm_loader

#SPM binary
PRODUCT_PACKAGES += pcm_deepidle.bin
PRODUCT_PACKAGES += pcm_deepidle_by_mp1.bin
PRODUCT_PACKAGES += pcm_suspend.bin
PRODUCT_PACKAGES += pcm_suspend_by_mp1.bin
PRODUCT_PACKAGES += pcm_sodi.bin
PRODUCT_PACKAGES += pcm_sodi_by_mp1.bin
PRODUCT_PACKAGES += pcm_vcorefs_hpm.bin
PRODUCT_PACKAGES += pcm_vcorefs_lpm.bin
PRODUCT_PACKAGES += pcm_vcorefs_ultra.bin

#Connectivity combo_tool
PRODUCT_PACKAGES += 6620_launcher
PRODUCT_PACKAGES += 6620_wmt_concurrency
PRODUCT_PACKAGES += 6620_wmt_lpbk
PRODUCT_PACKAGES += wmt_loader
PRODUCT_PACKAGES += stp_dump3

#Copy Audio Parameter file
PRODUCT_COPY_FILES += device/amt/amt6797_64_open/AudioParamOptions.xml:system/etc/audio_param/AudioParamOptions.xml

#Copy Scatter file
PRODUCT_COPY_FILES += device/amt/amt6797_64_open/MT6797_Android_scatter.txt:MT6797_Android_scatter.txt
PRODUCT_COPY_FILES += device/amt/amt6797_64_open/PGPT:PGPT

mtk_audio_param_xml_list := $(wildcard device/mediatek/common/audio_param/*.xml)

$(foreach var, $(mtk_audio_param_xml_list),\
	$(eval audio_param_file := $(notdir $(var)))\
	$(eval _src := $(var))\
	$(eval _dest := system/etc/audio_param/$(audio_param_file))\
	$(eval PRODUCT_COPY_FILES += $(_src):$(_dest))\
)

# inherit 6797 platform
$(call inherit-product, device/mediatek/mt6797/device.mk)
$(call inherit-product-if-exists, vendor/amt/libs/$(MTK_TARGET_PROJECT)/device-vendor.mk)
