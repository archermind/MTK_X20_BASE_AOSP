###################################################################
# Default Platform Feautre
###################################################################
CFG_TESTSUITE_SUPPORT = no
CFG_MODULE_INIT_SUPPORT = yes
CFG_XGPT_SUPPORT = yes
CFG_UART_SUPPORT = yes
CFG_SEM_SUPPORT = yes
CFG_IPC_SUPPORT = yes
CFG_LOGGER_SUPPORT = yes
CFG_WDT_SUPPORT = yes
CFG_DMA_SUPPORT = yes
CFG_PMIC_WRAP_SUPPORT = yes
CFG_ETM_SUPPORT = yes
CFG_I2C_SUPPORT = yes
CFG_CTP_SUPPORT = no
CFG_EINT_SUPPORT = yes
CFG_HEAP_GUARD_SUPPORT = no
CFG_SENSORHUB_TEST_SUPPORT = no
CFG_VCORE_DVFS_SUPPORT = yes
CFG_SENSORHUB_SUPPORT = no
CFG_MPU_DEBUG_SUPPORT = yes
CFG_MTK_VOW_SUPPORT = no
CFG_MTK_SCPUART_SUPPORT = no
CFG_RAMDUMP_SUPPORT = yes
CFG_AUDIO_SUPPORT = no
CFG_DWT_SUPPORT = no
CFG_DRAMC_MONITOR_SUPPORT = no

###################################################################
# Optional ProjectConfig.mk used by project
###################################################################
-include $(PROJECT_DIR)/ProjectConfig.mk

###################################################################
# Mandatory platform-specific resources
###################################################################
INCLUDES += \
  -I$(PLATFORM_DIR)/inc \
  -I$(SOURCE_DIR)/kernel/service/common/include \
  -I$(SOURCE_DIR)/kernel/CMSIS/Device/MTK/$(PLATFORM)/Include \
  -I$(SOURCE_DIR)/middleware/SensorHub \
  -I$(DRIVERS_PLATFORM_DIR)/feature_manager/inc

C_FILES += \
  $(PLATFORM_DIR)/src/main.c \
  $(PLATFORM_DIR)/src/platform.c \
  $(PLATFORM_DIR)/src/interrupt.c \
  $(SOURCE_DIR)/kernel/service/common/src/mt_printf.c \
  $(PLATFORM_DIR)/src/$(PLATFORM)_it.c \
  $(DRIVERS_PLATFORM_DIR)/feature_manager/src/feature_manager.c

# Add startup files to build
C_FILES += $(PLATFORM_DIR)/CMSIS/system.c
S_FILES += $(PLATFORM_DIR)/CMSIS/startup.S

# Add dramc (gating auto save) files to build, and please DO NOT remove.
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/dramc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/dramc/dramc.c




###################################################################
# Resources determined by configuration options
###################################################################
ifeq ($(CFG_TESTSUITE_SUPPORT),yes)
INCLUDES += -I$(TINYSYS_SECURE_DIR)/$(PROJECT_DIR)/testsuite/inc
INCLUDES += -I$(SOURCE_DIR)/middleware/lib/console/include
C_FILES  += $(TINYSYS_SECURE_DIR)/$(PROJECT_DIR)/testsuite/src/ts_sample/src/EINT_testsuite.c
C_FILES  += $(TINYSYS_SECURE_DIR)/$(PROJECT_DIR)/testsuite/src/ts_sample/src/sample.c
C_FILES  += $(TINYSYS_SECURE_DIR)/$(PROJECT_DIR)/testsuite/src/ts_platform/src/platform.c
C_FILES  += $(TINYSYS_SECURE_DIR)/$(PROJECT_DIR)/testsuite/src/ts_vcoredvfs/src/dvfs_test.c
C_FILES  += $(DRIVERS_PLATFORM_DIR)/eint/src/test/eint_test.c
C_FILES  += middleware/lib/console/console.c
endif

ifeq ($(CFG_MODULE_INIT_SUPPORT),yes)
C_FILES  += $(SOURCE_DIR)/kernel/service/common/src/module_init.c
endif

ifeq ($(CFG_XGPT_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/xgpt/inc/
C_FILES  += $(DRIVERS_PLATFORM_DIR)/xgpt/src/xgpt.c
C_FILES  += $(SOURCE_DIR)/kernel/service/common/src/utils.c
endif

ifeq ($(CFG_PMIC_WRAP_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/pmic_wrap/inc/
C_FILES  += $(DRIVERS_PLATFORM_DIR)/pmic_wrap/pmic_wrap.c
endif
ifeq ($(CFG_UART_SUPPORT),yes)
C_FILES  += $(DRIVERS_PLATFORM_DIR)/uart/uart.c
endif

ifeq ($(CFG_SEM_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/sem/inc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/sem/src/scp_sem.c
endif

ifeq ($(CFG_IPC_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/ipi/inc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/ipi/src/scp_ipi.c
endif

ifeq ($(CFG_LOGGER_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/logger/inc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/logger/src/scp_logger.c
endif

ifeq ($(CFG_WDT_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/wdt/inc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/wdt/src/wdt.c
endif

ifeq ($(CFG_DMA_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/dma
C_FILES  += $(DRIVERS_PLATFORM_DIR)/dma/dma.c
C_FILES  += $(DRIVERS_PLATFORM_DIR)/dma/dma_api.c
endif

ifeq ($(CFG_ETM_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/etm
C_FILES  += $(DRIVERS_PLATFORM_DIR)/etm/etm.c
endif

ifeq ($(CFG_I2C_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/i2c/inc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/i2c/src/hal_i2c.c
endif

#ifeq ($(CFG_CTP_SUPPORT),yes)
#endif

ifeq ($(CFG_EINT_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/eint/inc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/eint/src/eint.c
endif

ifeq ($(CFG_HEAP_GUARD_SUPPORT),yes)
C_FILES  += $(RTOS_SRC_DIR)/portable/MemMang/mtk_HeapGuard.c
LDFLAGS += -Wl, -wrap=pvPortMalloc -Wl, -wrap=vPortFree
endif

ifeq ($(CFG_VCORE_DVFS_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/kernel/service/common/include/
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/vcore_dvfs/inc
INCLUDES += -I$(TINYSYS_SECURE_DIR)/$(PROJECT_DIR)/testsuite/inc/
C_FILES  += $(DRIVERS_PLATFORM_DIR)/vcore_dvfs/src/vcore_dvfs.c
endif

ifeq ($(CFG_SENSORHUB_SUPPORT), yes)
INCLUDES += -I$(HAL_PLATFORM_DIR)/driver/ipi/inc
INCLUDES += -I$(SOURCE_DIR)/middleware/lib/console/include
INCLUDES += -I$(SOURCE_DIR)/middleware/SensorHub
INCLUDES += -I$(SOURCE_DIR)/kernel/FreeRTOS/Source/include
INCLUDES += -I$(PLATFORM_DIR)/inc
INCLUDES += -I$(SOURCE_DIR)/kernel/FreeRTOS/Source/portable/GCC/ARM_CM4F
INCLUDES += -I$(SOURCE_DIR)/kernel/service/common/include
C_FILES  += $(SOURCE_DIR)/middleware/SensorHub/sensor_manager.c
C_FILES  += $(SOURCE_DIR)/middleware/SensorHub/sensor_manager_fw.c
endif

ifeq ($(CFG_SENSORHUB_TEST_SUPPORT), yes)
C_FILES  += $(SOURCE_DIR)/middleware/SensorHub/sensorframeworktest.c
C_FILES  += $(SOURCE_DIR)/middleware/SensorHub/FakeAccelSensorDriver.c
endif

ifeq ($(CFG_FLP_SUPPORT), yes)
CFG_CCCI_SUPPORT = yes
else
    ifeq ($(CFG_MTK_AURISYS_PHONE_CALL_SUPPORT), yes)
    CFG_CCCI_SUPPORT = yes
    else
    CFG_CCCI_SUPPORT = no
    endif
endif
ifeq ($(CFG_CCCI_SUPPORT), yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/ccci
C_FILES  += $(DRIVERS_PLATFORM_DIR)/ccci/ccci.c
C_FILES  += $(DRIVERS_PLATFORM_DIR)/ccci/ccism_ringbuf.c
C_FILES  += $(DRIVERS_PLATFORM_DIR)/ccci/sensor_modem.c
endif

ifeq ($(CFG_MPU_DEBUG_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/mpu/inc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/mpu/src/mpu.c
endif

ifeq ($(CFG_MTK_VOW_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/VOW
INCLUDES += -I$(SOURCE_DIR)/middleware/VOW/lib
LIBFLAGS += -L$(SOURCE_DIR)/middleware/VOW/lib -lvow
C_FILES  += $(SOURCE_DIR)/middleware/VOW/vow_service.c
C_FILES  += $(SOURCE_DIR)/middleware/VOW/vow_ipi_message.c
C_FILES  += $(SOURCE_DIR)/middleware/VOW/swVAD.c
endif

ifeq ($(CFG_MTK_AURISYS_PHONE_CALL_SUPPORT),yes)
CFG_AUDIO_SUPPORT = yes
endif

ifeq ($(CFG_MTK_AUDIO_TUNNELING_SUPPORT),yes)
CFG_AUDIO_SUPPORT = yes
endif

# add for proximity
ifeq ($(CFG_MTK_VOICE_ULTRASOUND_SUPPORT),yes)
CFG_AUDIO_SUPPORT = yes
endif

ifeq ($(CFG_AUDIO_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/audio/inc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/audio/src/audio.c
C_FILES  += $(DRIVERS_PLATFORM_DIR)/audio/src/audio_irq.c
C_FILES  += $(DRIVERS_PLATFORM_DIR)/audio/src/audio_task_factory.c
C_FILES  += $(DRIVERS_PLATFORM_DIR)/audio/src/audio_messenger_ipi.c

###################################################################
# Debug wrapper function
###################################################################
LDFLAGS  += -Wl,-wrap,vPortEnterCritical
LDFLAGS  += -Wl,-wrap,vPortExitCritical
endif

ifeq ($(CFG_MTK_AURISYS_PHONE_CALL_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/lib/aurisys
#C_FILES  += $(SOURCE_DIR)/middleware/lib/aurisys/arsi_api.c
LIBFLAGS += -Wl,-L$(SOURCE_DIR)/middleware/lib/aurisys,-lFV-SAM,-lCMSIS
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/audio/inc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/audio/src/audio_task_phone_call.c
endif

ifeq ($(CFG_MTK_AUDIO_TUNNELING_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/lib/aurisys
C_FILES  += $(DRIVERS_PLATFORM_DIR)/audio/src/audio_task_offload_mp3.c
INCLUDES += -I$(SOURCE_DIR)/middleware/lib/mp3offload/inc
LIBFLAGS += -L$(SOURCE_DIR)/middleware/lib/mp3offload
LIBFLAGS += -lmp3dec
C_FILES  += $(SOURCE_DIR)/middleware/lib/mp3offload/RingBuf.c
INCLUDES += -I$(SOURCE_DIR)/middleware/lib/blisrc/inc
LIBFLAGS += -L$(SOURCE_DIR)/middleware/lib/blisrc
LIBFLAGS += -lblisrc
endif

# add for proximity
ifeq ($(CFG_MTK_VOICE_ULTRASOUND_SUPPORT),yes)
INCLUDES += -I$(SOURCE_DIR)/middleware/lib/aurisys
INCLUDES += -I$(SOURCE_DIR)/middleware/ultraSound
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/audio/inc
C_FILES  += $(SOURCE_DIR)/middleware/ultraSound/uSnd_ipi_message.c
C_FILES  += $(DRIVERS_PLATFORM_DIR)/audio/src/audio_task_ultrasound_proximity.c
endif

ifeq ($(CFG_MTK_SENSOR_HUB_SUPPORT), yes)
# CFLAGS += -DCFG_MTK_SENSOR_HUB_SUPPORT="$(CFG_MTK_SENSOR_HUB_SUPPORT)"
INCLUDES += middleware/shf/
C_FILES  += middleware/shf/shf_types.c
C_FILES  += middleware/shf/shf_debug.c
C_FILES  += middleware/shf/shf_data_pool.c
C_FILES  += middleware/shf/shf_action.c
C_FILES  += middleware/shf/shf_condition.c
C_FILES  += middleware/shf/shf_sensor.c
C_FILES  += middleware/shf/shf_process.c
C_FILES  += middleware/shf/shf_communicator.c
C_FILES  += middleware/shf/shf_configurator.c
C_FILES  += middleware/shf/shf_scheduler.c
C_FILES  += middleware/shf/shf_main.c
INCLUDES += -I$(SOURCE_DIR)/middleware/SensorHub
INCLUDES += -I$(SOURCE_DIR)/middleware/shf
endif
ifeq ($(CFG_DWT_SUPPORT),yes)
INCLUDES += -I$(DRIVERS_PLATFORM_DIR)/dwt/inc
C_FILES  += $(DRIVERS_PLATFORM_DIR)/dwt/src/dwt.c
endif

ifeq ($(CFG_FLP_SUPPORT),yes)
C_FILES += $(SOURCE_DIR)/middleware/FlpService/flp_service.c
INCLUDES += -I$(SOURCE_DIR)/middleware/FlpService/
endif

###################################################################
# Optional CompilerOption.mk used by project
###################################################################
-include $(PROJECT_DIR)/CompilerOption.mk
