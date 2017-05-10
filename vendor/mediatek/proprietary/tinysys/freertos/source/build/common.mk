###########################################################
## Check for valid float argument
##
## NOTE that you have to run make clean after changing
## these since hardfloat and softfloat are not binary
## compatible
###########################################################
ifneq ($(FLOAT_TYPE), hard)
  ifneq ($(FLOAT_TYPE), soft)
    override FLOAT_TYPE = hard
    #override FLOAT_TYPE = soft
  endif
endif

ifeq ($(FLOAT_TYPE), hard)
  FPUFLAGS = -fsingle-precision-constant -Wdouble-promotion
  FPUFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
  #CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=softfp
else
  FPUFLAGS = -msoft-float
endif

ALLFLAGS := -g -Os -Wall -Werror -mlittle-endian -mthumb
CFLAGS := \
  $(ALLFLAGS) \
  -flto -ffunction-sections -fdata-sections -fno-builtin \
  -D_REENT_SMALL $(FPUFLAGS) -gdwarf-2 -std=c99

# Add to support program counter backtrace dump
CFLAGS   += -funwind-tables

# Define build type. Default to release
BUILD_TYPE ?= release

ifeq (1,$(V))
  $(info $(TINYSYS_SCP): BUILD_TYPE=$(BUILD_TYPE))
endif

ifeq (debug,$(strip $(BUILD_TYPE)))
  CFLAGS   += -DTINYSYS_DEBUG_BUILD
endif

###########################################################
## LD flags
###########################################################
LDFLAGS := \
  $(ALLFLAGS) \
  $(FPUFLAGS) \
  --specs=nano.specs -lc -lnosys -lm \
  -nostartfiles --specs=rdimon.specs -lrdimon -Wl,--gc-sections

# Add to support printf wrapper function
LDFLAGS  += -Wl,-wrap,printf

###########################################################
## Processor-specific common instructions
###########################################################
ifneq ($(filter CM4_%,$(PROCESSOR)),)
  PROCESSOR_FLAGS := -mcpu=cortex-m4
  ALLFLAGS        += $(PROCESSOR_FLAGS)
  CFLAGS          += $(ALLFLAGS)
  LDFLAGS         += $(ALLFLAGS)
endif

###########################################################
## Common source codes
###########################################################
RTOS_FILES := \
  $(RTOS_SRC_DIR)/tasks.c \
  $(RTOS_SRC_DIR)/list.c \
  $(RTOS_SRC_DIR)/queue.c \
  $(RTOS_SRC_DIR)/timers.c \
  $(RTOS_SRC_DIR)/portable/MemMang/heap_4.c

###########################################################
## Include path
###########################################################
INCLUDES := \
  -I$(RTOS_SRC_DIR)/include \
  -I$(SOURCE_DIR)/$(APP_PATH) \
  -I$(SOURCE_DIR)/kernel/CMSIS/Include

###########################################################
## Processor-specific common instructions
###########################################################
ifneq ($(filter CM4_%,$(PROCESSOR)),)
  RTOS_FILES += $(RTOS_SRC_DIR)/portable/GCC/ARM_CM4F/port.c
  INCLUDES   += -I$(RTOS_SRC_DIR)/portable/GCC/ARM_CM4F
endif

C_FILES := $(RTOS_FILES)
