# Add for ANT+
ifeq ($(strip $(MTK_ANT_SUPPORT)), yes)
  PRODUCT_PACKAGES += com.dsi.ant.antradio_library \
                      AntHalService \
                      libantradio \
                      antradio_app

  ant_patch_folder := vendor/mediatek/proprietary/hardware/connectivity/ANT/RAM
ifneq ($(filter MT6630,$(MTK_COMBO_CHIP)),)
  PRODUCT_COPY_FILES += $(ant_patch_folder)/ANT_RAM_CODE_E1.BIN:system/etc/firmware/ANT_RAM_CODE_E1.BIN
  PRODUCT_COPY_FILES += $(ant_patch_folder)/ANT_RAM_CODE_E2.BIN:system/etc/firmware/ANT_RAM_CODE_E2.BIN
endif
ifneq ($(filter CONSYS_6797,$(MTK_COMBO_CHIP)),)
  PRODUCT_COPY_FILES += $(ant_patch_folder)/ANT_RAM_CODE_CONN_V1.BIN:system/etc/firmware/ANT_RAM_CODE_CONN_V1.BIN
endif

  ant_radio_folder := vendor/mediatek/proprietary/external/ant-wireless/antradio-library
  PRODUCT_COPY_FILES += $(ant_radio_folder)/com.dsi.ant.antradio_library.xml:system/etc/permissions/com.dsi.ant.antradio_library.xml
endif

