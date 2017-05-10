LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH    := arm
ARM_CPU := cortex-a53
CPU     := generic

MMC_SLOT         := 1

# choose one of following value -> 1: disabled/ 2: permissive /3: enforcing
SELINUX_STATUS := 3

# overwrite SELINUX_STATUS value with PRJ_SELINUX_STATUS, if defined. it's by project variable.
ifdef PRJ_SELINUX_STATUS
	SELINUX_STATUS := $(PRJ_SELINUX_STATUS)
endif

ifeq (yes,$(strip $(MTK_BUILD_ROOT)))
SELINUX_STATUS := 2
DEFINES += MTK_BUILD_ROOT
endif

DEFINES += PLATFORM=\"$(PLATFORM)\"

DEFINES += SELINUX_STATUS=$(SELINUX_STATUS)

DEFINES += PERIPH_BLK_BLSP=1
DEFINES += WITH_CPU_EARLY_INIT=0 WITH_CPU_WARM_BOOT=0 \
	   MMC_SLOT=$(MMC_SLOT)

DEFINES += MTK_CAN_FROM_CLUSTER1=1

CFG_DTB_EARLY_LOADER_SUPPORT := yes
ifeq ($(CFG_DTB_EARLY_LOADER_SUPPORT), yes)
	DEFINES += CFG_DTB_EARLY_LOADER_SUPPORT=1
endif

MBLOCK_LIB_SUPPORT := yes
ifeq ($(MBLOCK_LIB_SUPPORT), yes)
	DEFINES += MBLOCK_LIB_SUPPORT
	MODULES += $(LOCAL_DIR)/../../lib/mblock
	INCLUDES += -I$(LOCAL_DIR)/../../lib/mblock
endif

ifeq ($(MTK_SECURITY_SW_SUPPORT), yes)
	DEFINES += MTK_SECURITY_SW_SUPPORT
endif

ifeq ($(MTK_GOOGLE_TRUSTY_SUPPORT), yes)
DEFINES += MTK_LM_2LEVEL_PAGETABLE_MODE
endif

ifeq ($(PLATFORM_FASTBOOT_EMPTY_STORAGE), yes)
	DEFINES += PLATFORM_FASTBOOT_EMPTY_STORAGE
#FASTBOOT_DA_MODE 1: factory mode, 2: user mode
	FASTBOOT_DA_MODE := 1
endif

# choose one of following value -> 0: disabled/1: enable

MTK_POWER_ON_WRITE_PROTECT := 1
ifdef PRJ_MTK_POWER_ON_WRITE_PROTECT
	MTK_POWER_ON_WRITE_PROTECT := $(PRJ_MTK_POWER_ON_WRITE_PROTECT)
endif

ifeq ($(MTK_POWER_ON_WRITE_PROTECT), 1)
DEFINES += MTK_POWER_ON_WRITE_PROTECT=$(MTK_POWER_ON_WRITE_PROTECT)
ifeq ($(MTK_SIM_LOCK_POWER_ON_WRITE_PROTECT), yes)
	DEFINES += MTK_SIM_LOCK_POWER_ON_WRITE_PROTECT
endif
endif

MTK_PERSIST_PARTITION_SUPPORT := 0
ifdef PRJ_MTK_PERSIST_PARTITION_SUPPORT
	MTK_PERSIST_PARTITION_SUPPORT := $(PRJ_MTK_PERSIST_PARTITION_SUPPORT)
endif
ifeq ($(MTK_PERSIST_PARTITION_SUPPORT), 1)
DEFINES += MTK_PERSIST_PARTITION_SUPPORT=$(MTK_PERSIST_PARTITION_SUPPORT)
endif

ifeq ($(MTK_SEC_FASTBOOT_UNLOCK_SUPPORT), yes)
	DEFINES += MTK_SEC_FASTBOOT_UNLOCK_SUPPORT
ifeq ($(MTK_SEC_FASTBOOT_UNLOCK_KEY_SUPPORT), yes)
	DEFINES += MTK_SEC_FASTBOOT_UNLOCK_KEY_SUPPORT
endif
endif

ifeq ($(MTK_FORCE_MODEL), yes)
	DEFINES += MTK_FORCE_MODEL=2
endif

ifeq ($(MTK_KERNEL_POWER_OFF_CHARGING),yes)
#Fastboot support off-mode-charge 0/1
#1: charging mode, 0:skip charging mode
DEFINES += MTK_OFF_MODE_CHARGE_SUPPORT
endif

KEDUMP_MINI := yes

ARCH_HAVE_MT_RAMDUMP := yes

DEFINES += $(shell echo $(BOOT_LOGO) | tr a-z A-Z)

MTK_EMMC_POWER_ON_WP := yes
ifeq ($(MTK_EMMC_SUPPORT),yes)
ifeq ($(MTK_EMMC_POWER_ON_WP),yes)
        DEFINES += MTK_EMMC_POWER_ON_WP
endif
endif

$(info libshowlogo new path ------- $(LOCAL_DIR)/../../lib/libshowlogo)
INCLUDES += -I$(LOCAL_DIR)/include \
            -I$(LOCAL_DIR)/include/platform \
            -I$(LOCAL_DIR)/../../lib/libshowlogo \
            -Icustom/$(FULL_PROJECT)/lk/include/target \
            -Icustom/$(FULL_PROJECT)/lk/lcm/inc \
            -Icustom/$(FULL_PROJECT)/lk/inc \
            -Icustom/$(FULL_PROJECT)/common \
            -Icustom/$(FULL_PROJECT)/kernel/dct/ \
            -I$(BUILDDIR)/include/dfo \
            -I$(LOCAL_DIR)/../../dev/lcm/inc

INCLUDES += -I$(DRVGEN_OUT)/inc

OBJS += \
	$(LOCAL_DIR)/bitops.o \
	$(LOCAL_DIR)/mt_gpio.o \
	$(LOCAL_DIR)/log_store_lk.o \
	$(LOCAL_DIR)/mt_disp_drv.o \
	$(LOCAL_DIR)/lastpc.o \
	$(LOCAL_DIR)/mt_gpio_init.o \
	$(LOCAL_DIR)/mt_i2c.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/uart.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/boot_mode.o \
	$(LOCAL_DIR)/load_image.o \
	$(LOCAL_DIR)/atags.o \
	$(LOCAL_DIR)/partition.o \
	$(LOCAL_DIR)/partition_setting.o \
	$(LOCAL_DIR)/mt_get_dl_info.o \
	$(LOCAL_DIR)/addr_trans.o \
	$(LOCAL_DIR)/factory.o \
	$(LOCAL_DIR)/mt_gpt.o\
	$(LOCAL_DIR)/mtk_key.o \
	$(LOCAL_DIR)/efi.o\
	$(LOCAL_DIR)/recovery.o\
	$(LOCAL_DIR)/meta.o\
	$(LOCAL_DIR)/mt_logo.o\
	$(LOCAL_DIR)/boot_mode_menu.o\
	$(LOCAL_DIR)/env.o\
	$(LOCAL_DIR)/mt_pmic_wrap_init.o\
	$(LOCAL_DIR)/mmc_common_inter.o \
	$(LOCAL_DIR)/mmc_core.o\
	$(LOCAL_DIR)/mmc_test.o\
	$(LOCAL_DIR)/msdc.o\
	$(LOCAL_DIR)/msdc_io.o\
	$(LOCAL_DIR)/mmc_rpmb.o\
	$(LOCAL_DIR)/msdc_dma.o\
	$(LOCAL_DIR)/msdc_utils.o\
	$(LOCAL_DIR)/msdc_irq.o\
	$(LOCAL_DIR)/upmu_common.o\
	$(LOCAL_DIR)/mt_pmic.o\
	$(LOCAL_DIR)/mtk_wdt.o\
	$(LOCAL_DIR)/mt_rtc.o\
	$(LOCAL_DIR)/mt_usbphy.o\
	$(LOCAL_DIR)/mt_usb.o\
	$(LOCAL_DIR)/mt_ssusb_qmu.o\
	$(LOCAL_DIR)/mt_mu3d_hal_qmu_drv.o\
	$(LOCAL_DIR)/ddp_manager.o\
	$(LOCAL_DIR)/ddp_path.o\
	$(LOCAL_DIR)/ddp_ovl.o\
	$(LOCAL_DIR)/ddp_rdma.o\
	$(LOCAL_DIR)/ddp_misc.o\
	$(LOCAL_DIR)/ddp_info.o\
	$(LOCAL_DIR)/ddp_dither.o\
	$(LOCAL_DIR)/ddp_dump.o\
	$(LOCAL_DIR)/ddp_dsi.o\
	$(LOCAL_DIR)/primary_display.o\
	$(LOCAL_DIR)/disp_lcm.o\
	$(LOCAL_DIR)/ddp_pwm.o\
	$(LOCAL_DIR)/pwm.o \
	$(LOCAL_DIR)/fpc_sw_repair2sw.o\
	$(LOCAL_DIR)/emi_mpu.o\
	$(LOCAL_DIR)/sec_policy.o\
	$(LOCAL_DIR)/mt_efuse.o\
	$(LOCAL_DIR)/aee_platform_debug.o\
	$(LOCAL_DIR)/ddp_ufoe.o \
	$(LOCAL_DIR)/ddp_split.o \
        $(LOCAL_DIR)/mt_dummy_read.o\
	$(LOCAL_DIR)/write_protect.o \
	$(LOCAL_DIR)/atf_ramdump.o

ifeq ($(MTK_TINYSYS_SCP_SUPPORT), yes)
OBJS += $(LOCAL_DIR)/mt_scp.o
DEFINES += MTK_TINYSYS_SCP_SUPPORT
endif

ifeq ($(MTK_SECURITY_SW_SUPPORT), yes)
OBJS +=	$(LOCAL_DIR)/sec_logo_auth.o
endif

ifneq ($(MACH_FPGA_LED_SUPPORT), yes)
OBJS += $(LOCAL_DIR)/mt_leds.o
else
DEFINES += MACH_FPGA_LED_SUPPORT
endif

# SETTING of USBPHY type
#OBJS += $(LOCAL_DIR)/mt_usbphy_d60802.o
#OBJS += $(LOCAL_DIR)/mt_usbphy_e60802.o
OBJS += $(LOCAL_DIR)/mt_usbphy_a60810.o

OBJS += $(LOCAL_DIR)/mt_battery.o

ifneq ($(MTK_EMMC_SUPPORT),yes)
#	OBJS +=$(LOCAL_DIR)/mtk_nand.o
#	OBJS +=$(LOCAL_DIR)/bmt.o
endif

ifeq ($(MTK_USB2JTAG_SUPPORT),yes)
OBJS += $(LOCAL_DIR)/mt_usb2jtag.o
DEFINES += MTK_USB2JTAG_SUPPORT
endif

ifeq ($(MTK_MT8193_SUPPORT),yes)
#OBJS +=$(LOCAL_DIR)/mt8193_init.o
#OBJS +=$(LOCAL_DIR)/mt8193_ckgen.o
#OBJS +=$(LOCAL_DIR)/mt8193_i2c.o
endif

ifeq ($(MTK_KERNEL_POWER_OFF_CHARGING),yes)
OBJS +=$(LOCAL_DIR)/mt_kernel_power_off_charging.o
DEFINES += MTK_KERNEL_POWER_OFF_CHARGING
endif


MTK_BQ25896_SUPPORT := yes
ifeq ($(MTK_BQ24261_SUPPORT),yes)
OBJS +=$(LOCAL_DIR)/bq24261.o
else
  ifeq ($(MTK_BQ24196_SUPPORT),yes)
    OBJS +=$(LOCAL_DIR)/bq24196.o
  else
    ifeq ($(MTK_BQ25896_SUPPORT),yes)
      OBJS +=$(LOCAL_DIR)/bq25890.o
    endif
  endif
endif

ifeq ($(MTK_NCP1854_SUPPORT),yes)
OBJS +=$(LOCAL_DIR)/ncp1854.o
endif

ifeq ($(DUMMY_AP),yes)
OBJS +=$(LOCAL_DIR)/dummy_ap.o
OBJS +=$(LOCAL_DIR)/spm_md_mtcmos.o
endif
OBJS +=$(LOCAL_DIR)/ccci_lk_load_img.o

ifeq ($(MTK_EMMC_POWER_ON_WP),yes)
ifeq ($(MTK_EMMC_SUPPORT),yes)
    OBJS +=$(LOCAL_DIR)/partition_wp.o
endif
endif

ifeq ($(MTK_EFUSE_WRITER_SUPPORT), yes)
    DEFINES += MTK_EFUSE_WRITER_SUPPORT
endif

ifeq ($(TRUSTONIC_TEE_SUPPORT),yes)
DEFINES += TRUSTONIC_TEE_SUPPORT
endif

ifeq ($(MTK_GOOGLE_TRUSTY_SUPPORT),yes)
DEFINES += MTK_GOOGLE_TRUSTY_SUPPORT
endif

ifeq ($(MTK_SECURITY_SW_SUPPORT), yes)
ifeq ($(CUSTOM_SEC_AUTH_SUPPORT), yes)
LIBSEC := -L$(LOCAL_DIR)/lib --start-group -lsec
else
LIBSEC := -L$(LOCAL_DIR)/lib --start-group -lsec -lauth
endif
LIBSEC_PLAT := -lsplat -ldevinfo  -lscp --end-group
else
LIBSEC := -L$(LOCAL_DIR)/lib
LIBSEC_PLAT := -ldevinfo -lscp
endif

MTK_GIC_VERSION := v3

MTK_LK_REGISTER_WDT := yes
ifeq ($(MTK_LK_REGISTER_WDT),yes)
	DEFINES += MTK_LK_REGISTER_WDT
	OBJS += $(LOCAL_DIR)/mt_debug_dump.o
endif

ifeq ($(MTK_NO_BIGCORES),yes)
	DEFINES += MTK_NO_BIGCORES
endif

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld
