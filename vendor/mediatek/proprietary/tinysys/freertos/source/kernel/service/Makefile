BINPATH=~/gcc-arm-none-eabi/bin
CC=$(BINPATH)/arm-none-eabi-gcc
AR=$(BINPATH)/arm-none-eabi-ar

# Project name
TARGET_LIB=libkal_srv

DEBUG=1
GCOV=1
FLOAT_TYPE=hard
SOURCE_DIR = ../..
BUILD_DIR = Build
OUTPATH=Build

# Sources
SRC_DIR=kernel/service/common/src

#source files
FILES =	$(SRC_DIR)/kal_adm.c \
		$(SRC_DIR)/kal_afm.c \
		$(SRC_DIR)/kal_compatible.c \
		$(SRC_DIR)/utils.c

C_FILES = $(FILES)
C_OBJS = $(C_FILES:%.c=$(BUILD_DIR)/%.o)

###################################################

# Check for valid float argument
# NOTE that you have to run make clan after
# changing these as hardfloat and softfloat are not
# binary compatible
ifneq ($(FLOAT_TYPE), hard)
ifneq ($(FLOAT_TYPE), soft)
override FLOAT_TYPE = hard
#override FLOAT_TYPE = soft
endif
endif

###################################################
# CC Flags
ifeq ($(DEBUG), 1)
ALLFLAGS = -g -O0
else
ALLFLAGS = -g -Os
endif
ALLFLAGS += -Wall -Werror -mlittle-endian -mthumb -mcpu=cortex-m4
CFLAGS += $(ALLFLAGS) -flto -ffunction-sections -fdata-sections -fno-builtin

ifeq ($(FLOAT_TYPE), hard)
FPUFLAGS = -fsingle-precision-constant -Wdouble-promotion
FPUFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=hard
#CFLAGS += -mfpu=fpv4-sp-d16 -mfloat-abi=softfp
else
FPUFLAGS = -msoft-float
endif

# Definitions
CFLAGS += -D_REENT_SMALL $(FPUFLAGS)

###################################################
# include path
CFLAGS += -I$(SOURCE_DIR)/kernel/service/common/include -I$(SOURCE_DIR)/kernel/service/include
CFLAGS += -I$(SOURCE_DIR)/kernel/FreeRTOS/Source/include -I$(SOURCE_DIR)/kernel/service
CFLAGS += -I$(SOURCE_DIR)/kernel/FreeRTOS/Source/portable/GCC/ARM_CM4F

ifeq ($(GCOV), 1)
CFLAGS += -fprofile-arcs -ftest-coverage
endif

.PHONY: $(TARGET_LIB).a

all: $(TARGET_LIB).a

$(TARGET_LIB).a: $(C_OBJS) $(S_OBJS)
	@echo Gen $(TARGET_LIB).a
	@$(AR) -r $@ $(C_OBJS)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo Build... $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.d: $(SOURCE_DIR)/%.c
	@echo GenDep... $@
	@mkdir -p $(dir $@)
	@set -e; rm -f $@; \
	export D_FILE="$@"; \
	export B_NAME=`echo $$D_FILE | sed 's/\.d//g'`; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's@\(.*\)\.o@'"$$B_NAME\.o $$B_NAME\.d"'@g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -rf $(OUTPATH) $(TARGET_LIB).a

ifneq ($(MAKECMDGOALS),clean)
-include $(C_OBJS:.o=.d)
endif
