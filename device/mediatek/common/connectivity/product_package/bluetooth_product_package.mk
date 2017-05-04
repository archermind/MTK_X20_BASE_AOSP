# Bluetooth Configuration
ifeq ($(strip $(MTK_BT_SUPPORT)), yes)
  PRODUCT_PACKAGES += libbluetooth_mtk
  PRODUCT_PACKAGES += libbluetooth_mtk_pure
  PRODUCT_PACKAGES += libbluetoothem_mtk
  PRODUCT_PACKAGES += libbluetooth_relayer
  PRODUCT_PACKAGES += libbluetooth_hw_test
  PRODUCT_PACKAGES += autobt
endif

