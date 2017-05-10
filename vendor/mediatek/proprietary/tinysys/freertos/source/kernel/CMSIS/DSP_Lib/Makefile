BINPATH=~/gcc-arm-none-eabi/bin
CC=$(BINPATH)/arm-none-eabi-gcc
AR=$(BINPATH)/arm-none-eabi-ar

TARGET_LIB=libarm_cortexM4lf_math

DEBUG=0
FLOAT_TYPE=hard
SOURCE_DIR = ../../..
BUILD_DIR = Build

# Project name
TARGET_LIB=libarm_cortexM4lf_math
OUTPATH=Build

# Sources
SRC_DIR=kernel/CMSIS/DSP_Lib/Source

# Basic Math source files
BASIC_DIR = $(SRC_DIR)/BasicMathFunctions
BASIC_FILES =	$(BASIC_DIR)/arm_abs_f32.c \
				$(BASIC_DIR)/arm_abs_q15.c \
				$(BASIC_DIR)/arm_abs_q31.c \
				$(BASIC_DIR)/arm_abs_q7.c \
				$(BASIC_DIR)/arm_add_f32.c \
				$(BASIC_DIR)/arm_add_q15.c \
				$(BASIC_DIR)/arm_add_q31.c \
				$(BASIC_DIR)/arm_add_q7.c \
				$(BASIC_DIR)/arm_dot_prod_f32.c \
				$(BASIC_DIR)/arm_dot_prod_q15.c \
				$(BASIC_DIR)/arm_dot_prod_q31.c \
				$(BASIC_DIR)/arm_dot_prod_q7.c \
				$(BASIC_DIR)/arm_mult_f32.c \
				$(BASIC_DIR)/arm_mult_q15.c \
				$(BASIC_DIR)/arm_mult_q31.c \
				$(BASIC_DIR)/arm_mult_q7.c \
				$(BASIC_DIR)/arm_negate_f32.c \
				$(BASIC_DIR)/arm_negate_q15.c \
				$(BASIC_DIR)/arm_negate_q31.c \
				$(BASIC_DIR)/arm_negate_q7.c \
				$(BASIC_DIR)/arm_offset_f32.c \
				$(BASIC_DIR)/arm_offset_q15.c \
				$(BASIC_DIR)/arm_offset_q31.c \
				$(BASIC_DIR)/arm_offset_q7.c \
				$(BASIC_DIR)/arm_scale_f32.c \
				$(BASIC_DIR)/arm_scale_q15.c \
				$(BASIC_DIR)/arm_scale_q31.c \
				$(BASIC_DIR)/arm_scale_q7.c \
				$(BASIC_DIR)/arm_shift_q15.c \
				$(BASIC_DIR)/arm_shift_q31.c \
				$(BASIC_DIR)/arm_shift_q7.c \
				$(BASIC_DIR)/arm_sub_f32.c \
				$(BASIC_DIR)/arm_sub_q15.c \
				$(BASIC_DIR)/arm_sub_q31.c \
				$(BASIC_DIR)/arm_sub_q7.c

# Common Tables source files
COMMON_DIR = $(SRC_DIR)/CommonTables
COMMON_FILES =	$(COMMON_DIR)/arm_common_tables.c \
				$(COMMON_DIR)/arm_const_structs.c 

# Complex Math source files
COMPLX_DIR = $(SRC_DIR)/ComplexMathFunctions
COMPLX_FILES =	$(COMPLX_DIR)/arm_cmplx_conj_f32.c \
				$(COMPLX_DIR)/arm_cmplx_conj_q15.c \
				$(COMPLX_DIR)/arm_cmplx_conj_q31.c \
				$(COMPLX_DIR)/arm_cmplx_dot_prod_f32.c \
				$(COMPLX_DIR)/arm_cmplx_dot_prod_q15.c \
				$(COMPLX_DIR)/arm_cmplx_dot_prod_q31.c \
				$(COMPLX_DIR)/arm_cmplx_mag_f32.c \
				$(COMPLX_DIR)/arm_cmplx_mag_q15.c \
				$(COMPLX_DIR)/arm_cmplx_mag_q31.c \
				$(COMPLX_DIR)/arm_cmplx_mag_squared_f32.c \
				$(COMPLX_DIR)/arm_cmplx_mag_squared_q15.c \
				$(COMPLX_DIR)/arm_cmplx_mag_squared_q31.c \
				$(COMPLX_DIR)/arm_cmplx_mult_cmplx_f32.c \
				$(COMPLX_DIR)/arm_cmplx_mult_cmplx_q15.c \
				$(COMPLX_DIR)/arm_cmplx_mult_cmplx_q31.c \
				$(COMPLX_DIR)/arm_cmplx_mult_real_f32.c \
				$(COMPLX_DIR)/arm_cmplx_mult_real_q15.c \
				$(COMPLX_DIR)/arm_cmplx_mult_real_q31.c   

# Controller source files
CTRL_DIR = $(SRC_DIR)/ControllerFunctions
CTRL_FILES =	$(CTRL_DIR)/arm_pid_init_f32.c \
				$(CTRL_DIR)/arm_pid_init_q15.c \
				$(CTRL_DIR)/arm_pid_init_q31.c \
				$(CTRL_DIR)/arm_pid_reset_f32.c \
				$(CTRL_DIR)/arm_pid_reset_q15.c \
				$(CTRL_DIR)/arm_pid_reset_q31.c \
				$(CTRL_DIR)/arm_sin_cos_f32.c \
				$(CTRL_DIR)/arm_sin_cos_q31.c

# Fast Math source files
FAST_DIR = $(SRC_DIR)/FastMathFunctions
FAST_FILES =	$(FAST_DIR)/arm_cos_f32.c \
				$(FAST_DIR)/arm_cos_q15.c \
				$(FAST_DIR)/arm_cos_q31.c \
				$(FAST_DIR)/arm_sin_f32.c \
				$(FAST_DIR)/arm_sin_q15.c \
				$(FAST_DIR)/arm_sin_q31.c \
				$(FAST_DIR)/arm_sqrt_q15.c \
				$(FAST_DIR)/arm_sqrt_q31.c

# Filtering source files
FILTR_DIR = $(SRC_DIR)/FilteringFunctions
FILTR_FILES =	$(FILTR_DIR)/arm_biquad_cascade_df1_32x64_init_q31.c \
				$(FILTR_DIR)/arm_biquad_cascade_df1_32x64_q31.c \
				$(FILTR_DIR)/arm_biquad_cascade_df1_f32.c \
				$(FILTR_DIR)/arm_biquad_cascade_df1_fast_q15.c \
				$(FILTR_DIR)/arm_biquad_cascade_df1_fast_q31.c \
				$(FILTR_DIR)/arm_biquad_cascade_df1_init_f32.c \
				$(FILTR_DIR)/arm_biquad_cascade_df1_init_q15.c \
				$(FILTR_DIR)/arm_biquad_cascade_df1_init_q31.c \
				$(FILTR_DIR)/arm_biquad_cascade_df1_q15.c \
				$(FILTR_DIR)/arm_biquad_cascade_df1_q31.c \
				$(FILTR_DIR)/arm_biquad_cascade_df2T_f32.c \
				$(FILTR_DIR)/arm_biquad_cascade_df2T_f64.c \
				$(FILTR_DIR)/arm_biquad_cascade_df2T_init_f32.c \
				$(FILTR_DIR)/arm_biquad_cascade_df2T_init_f64.c \
				$(FILTR_DIR)/arm_biquad_cascade_stereo_df2T_f32.c \
				$(FILTR_DIR)/arm_biquad_cascade_stereo_df2T_init_f32.c \
				$(FILTR_DIR)/arm_conv_f32.c \
				$(FILTR_DIR)/arm_conv_fast_opt_q15.c \
				$(FILTR_DIR)/arm_conv_fast_q15.c \
				$(FILTR_DIR)/arm_conv_fast_q31.c \
				$(FILTR_DIR)/arm_conv_opt_q15.c \
				$(FILTR_DIR)/arm_conv_opt_q7.c \
				$(FILTR_DIR)/arm_conv_partial_f32.c \
				$(FILTR_DIR)/arm_conv_partial_fast_opt_q15.c \
				$(FILTR_DIR)/arm_conv_partial_fast_q15.c \
				$(FILTR_DIR)/arm_conv_partial_fast_q31.c \
				$(FILTR_DIR)/arm_conv_partial_opt_q15.c \
				$(FILTR_DIR)/arm_conv_partial_opt_q7.c \
				$(FILTR_DIR)/arm_conv_partial_q15.c \
				$(FILTR_DIR)/arm_conv_partial_q31.c \
				$(FILTR_DIR)/arm_conv_partial_q7.c \
				$(FILTR_DIR)/arm_conv_q15.c \
				$(FILTR_DIR)/arm_conv_q31.c \
				$(FILTR_DIR)/arm_conv_q7.c \
				$(FILTR_DIR)/arm_correlate_f32.c \
				$(FILTR_DIR)/arm_correlate_fast_opt_q15.c \
				$(FILTR_DIR)/arm_correlate_fast_q15.c \
				$(FILTR_DIR)/arm_correlate_fast_q31.c \
				$(FILTR_DIR)/arm_correlate_opt_q15.c \
				$(FILTR_DIR)/arm_correlate_opt_q7.c \
				$(FILTR_DIR)/arm_correlate_q15.c \
				$(FILTR_DIR)/arm_correlate_q31.c \
				$(FILTR_DIR)/arm_correlate_q7.c \
				$(FILTR_DIR)/arm_fir_decimate_f32.c \
				$(FILTR_DIR)/arm_fir_decimate_fast_q15.c \
				$(FILTR_DIR)/arm_fir_decimate_fast_q31.c \
				$(FILTR_DIR)/arm_fir_decimate_init_f32.c \
				$(FILTR_DIR)/arm_fir_decimate_init_q15.c \
				$(FILTR_DIR)/arm_fir_decimate_init_q31.c \
				$(FILTR_DIR)/arm_fir_decimate_q15.c \
				$(FILTR_DIR)/arm_fir_decimate_q31.c \
				$(FILTR_DIR)/arm_fir_f32.c \
				$(FILTR_DIR)/arm_fir_fast_q15.c \
				$(FILTR_DIR)/arm_fir_fast_q31.c \
				$(FILTR_DIR)/arm_fir_init_f32.c \
				$(FILTR_DIR)/arm_fir_init_q15.c \
				$(FILTR_DIR)/arm_fir_init_q31.c \
				$(FILTR_DIR)/arm_fir_init_q7.c \
				$(FILTR_DIR)/arm_fir_interpolate_f32.c \
				$(FILTR_DIR)/arm_fir_interpolate_init_f32.c \
				$(FILTR_DIR)/arm_fir_interpolate_init_q15.c \
				$(FILTR_DIR)/arm_fir_interpolate_init_q31.c \
				$(FILTR_DIR)/arm_fir_interpolate_q15.c \
				$(FILTR_DIR)/arm_fir_interpolate_q31.c \
				$(FILTR_DIR)/arm_fir_lattice_f32.c \
				$(FILTR_DIR)/arm_fir_lattice_init_f32.c \
				$(FILTR_DIR)/arm_fir_lattice_init_q15.c \
				$(FILTR_DIR)/arm_fir_lattice_init_q31.c \
				$(FILTR_DIR)/arm_fir_lattice_q15.c \
				$(FILTR_DIR)/arm_fir_lattice_q31.c \
				$(FILTR_DIR)/arm_fir_q15.c \
				$(FILTR_DIR)/arm_fir_q31.c \
				$(FILTR_DIR)/arm_fir_q7.c \
				$(FILTR_DIR)/arm_fir_sparse_f32.c \
				$(FILTR_DIR)/arm_fir_sparse_init_f32.c \
				$(FILTR_DIR)/arm_fir_sparse_init_q15.c \
				$(FILTR_DIR)/arm_fir_sparse_init_q31.c \
				$(FILTR_DIR)/arm_fir_sparse_init_q7.c \
				$(FILTR_DIR)/arm_fir_sparse_q15.c \
				$(FILTR_DIR)/arm_fir_sparse_q31.c \
				$(FILTR_DIR)/arm_fir_sparse_q7.c \
				$(FILTR_DIR)/arm_iir_lattice_f32.c \
				$(FILTR_DIR)/arm_iir_lattice_init_f32.c \
				$(FILTR_DIR)/arm_iir_lattice_init_q15.c \
				$(FILTR_DIR)/arm_iir_lattice_init_q31.c \
				$(FILTR_DIR)/arm_iir_lattice_q15.c \
				$(FILTR_DIR)/arm_iir_lattice_q31.c \
				$(FILTR_DIR)/arm_lms_f32.c \
				$(FILTR_DIR)/arm_lms_init_f32.c \
				$(FILTR_DIR)/arm_lms_init_q15.c \
				$(FILTR_DIR)/arm_lms_init_q31.c \
				$(FILTR_DIR)/arm_lms_norm_f32.c \
				$(FILTR_DIR)/arm_lms_norm_init_f32.c \
				$(FILTR_DIR)/arm_lms_norm_init_q15.c \
				$(FILTR_DIR)/arm_lms_norm_init_q31.c \
				$(FILTR_DIR)/arm_lms_norm_q15.c \
				$(FILTR_DIR)/arm_lms_norm_q31.c \
				$(FILTR_DIR)/arm_lms_q15.c \
				$(FILTR_DIR)/arm_lms_q31.c

# Matrix source files
MATRIX_DIR = $(SRC_DIR)/MatrixFunctions
MATRIX_FILES =	$(MATRIX_DIR)/arm_mat_add_f32.c \
				$(MATRIX_DIR)/arm_mat_add_q15.c \
				$(MATRIX_DIR)/arm_mat_add_q31.c \
				$(MATRIX_DIR)/arm_mat_cmplx_mult_f32.c \
				$(MATRIX_DIR)/arm_mat_cmplx_mult_q15.c \
				$(MATRIX_DIR)/arm_mat_cmplx_mult_q31.c \
				$(MATRIX_DIR)/arm_mat_init_f32.c \
				$(MATRIX_DIR)/arm_mat_init_q15.c \
				$(MATRIX_DIR)/arm_mat_init_q31.c \
				$(MATRIX_DIR)/arm_mat_inverse_f32.c \
				$(MATRIX_DIR)/arm_mat_inverse_f64.c \
				$(MATRIX_DIR)/arm_mat_mult_f32.c \
				$(MATRIX_DIR)/arm_mat_mult_fast_q15.c \
				$(MATRIX_DIR)/arm_mat_mult_fast_q31.c \
				$(MATRIX_DIR)/arm_mat_mult_q15.c \
				$(MATRIX_DIR)/arm_mat_mult_q31.c \
				$(MATRIX_DIR)/arm_mat_scale_f32.c \
				$(MATRIX_DIR)/arm_mat_scale_q15.c \
				$(MATRIX_DIR)/arm_mat_scale_q31.c \
				$(MATRIX_DIR)/arm_mat_sub_f32.c \
				$(MATRIX_DIR)/arm_mat_sub_q15.c \
				$(MATRIX_DIR)/arm_mat_sub_q31.c \
				$(MATRIX_DIR)/arm_mat_trans_f32.c \
				$(MATRIX_DIR)/arm_mat_trans_q15.c \
				$(MATRIX_DIR)/arm_mat_trans_q31.c 

# Statistics source files
STATI_DIR = $(SRC_DIR)/StatisticsFunctions
STATI_FILES = 	$(STATI_DIR)/arm_max_f32.c \
				$(STATI_DIR)/arm_max_q15.c \
				$(STATI_DIR)/arm_max_q31.c \
				$(STATI_DIR)/arm_max_q7.c \
				$(STATI_DIR)/arm_mean_f32.c \
				$(STATI_DIR)/arm_mean_q15.c \
				$(STATI_DIR)/arm_mean_q31.c \
				$(STATI_DIR)/arm_mean_q7.c \
				$(STATI_DIR)/arm_min_f32.c \
				$(STATI_DIR)/arm_min_q15.c \
				$(STATI_DIR)/arm_min_q31.c \
				$(STATI_DIR)/arm_min_q7.c \
				$(STATI_DIR)/arm_power_f32.c \
				$(STATI_DIR)/arm_power_q15.c \
				$(STATI_DIR)/arm_power_q31.c \
				$(STATI_DIR)/arm_power_q7.c \
				$(STATI_DIR)/arm_rms_f32.c \
				$(STATI_DIR)/arm_rms_q15.c \
				$(STATI_DIR)/arm_rms_q31.c \
				$(STATI_DIR)/arm_std_f32.c \
				$(STATI_DIR)/arm_std_q15.c \
				$(STATI_DIR)/arm_std_q31.c \
				$(STATI_DIR)/arm_var_f32.c \
				$(STATI_DIR)/arm_var_q15.c \
				$(STATI_DIR)/arm_var_q31.c

# Statistics source files
SUPRT_DIR = $(SRC_DIR)/SupportFunctions
SUPRT_FILES = 	$(SUPRT_DIR)/arm_copy_f32.c \
				$(SUPRT_DIR)/arm_copy_q15.c \
				$(SUPRT_DIR)/arm_copy_q31.c \
				$(SUPRT_DIR)/arm_copy_q7.c \
				$(SUPRT_DIR)/arm_fill_f32.c \
				$(SUPRT_DIR)/arm_fill_q15.c \
				$(SUPRT_DIR)/arm_fill_q31.c \
				$(SUPRT_DIR)/arm_fill_q7.c \
				$(SUPRT_DIR)/arm_float_to_q15.c \
				$(SUPRT_DIR)/arm_float_to_q31.c \
				$(SUPRT_DIR)/arm_float_to_q7.c \
				$(SUPRT_DIR)/arm_q15_to_float.c \
				$(SUPRT_DIR)/arm_q15_to_q31.c \
				$(SUPRT_DIR)/arm_q15_to_q7.c \
				$(SUPRT_DIR)/arm_q31_to_float.c \
				$(SUPRT_DIR)/arm_q31_to_q15.c \
				$(SUPRT_DIR)/arm_q31_to_q7.c \
				$(SUPRT_DIR)/arm_q7_to_float.c \
				$(SUPRT_DIR)/arm_q7_to_q15.c \
				$(SUPRT_DIR)/arm_q7_to_q31.c

# Transform source files
TRANS_DIR = $(SRC_DIR)/TransformFunctions
TRANS_FILES =	$(TRANS_DIR)/arm_bitreversal.c \
				$(TRANS_DIR)/arm_cfft_f32.c \
				$(TRANS_DIR)/arm_cfft_q15.c \
				$(TRANS_DIR)/arm_cfft_q31.c \
				$(TRANS_DIR)/arm_cfft_radix2_f32.c \
				$(TRANS_DIR)/arm_cfft_radix2_init_f32.c \
				$(TRANS_DIR)/arm_cfft_radix2_init_q15.c \
				$(TRANS_DIR)/arm_cfft_radix2_init_q31.c \
				$(TRANS_DIR)/arm_cfft_radix2_q15.c \
				$(TRANS_DIR)/arm_cfft_radix2_q31.c \
				$(TRANS_DIR)/arm_cfft_radix4_f32.c \
				$(TRANS_DIR)/arm_cfft_radix4_init_f32.c \
				$(TRANS_DIR)/arm_cfft_radix4_init_q15.c \
				$(TRANS_DIR)/arm_cfft_radix4_init_q31.c \
				$(TRANS_DIR)/arm_cfft_radix4_q15.c \
				$(TRANS_DIR)/arm_cfft_radix4_q31.c \
				$(TRANS_DIR)/arm_cfft_radix8_f32.c \
				$(TRANS_DIR)/arm_dct4_f32.c \
				$(TRANS_DIR)/arm_dct4_init_f32.c \
				$(TRANS_DIR)/arm_dct4_init_q15.c \
				$(TRANS_DIR)/arm_dct4_init_q31.c \
				$(TRANS_DIR)/arm_dct4_q15.c \
				$(TRANS_DIR)/arm_dct4_q31.c \
				$(TRANS_DIR)/arm_rfft_f32.c \
				$(TRANS_DIR)/arm_rfft_fast_f32.c \
				$(TRANS_DIR)/arm_rfft_fast_init_f32.c \
				$(TRANS_DIR)/arm_rfft_init_f32.c \
				$(TRANS_DIR)/arm_rfft_init_q15.c \
				$(TRANS_DIR)/arm_rfft_init_q31.c \
				$(TRANS_DIR)/arm_rfft_q15.c \
				$(TRANS_DIR)/arm_rfft_q31.c


#C_FILES=$(BASIC_DIR)/arm_abs_f32.c
C_FILES = $(BASIC_FILES) $(COMMON_FILES) $(COMPLX_FILES) $(CTRL_FILES) $(FAST_FILES) \
          $(FILTR_FILES) $(MATRIX_FILES) $(STATI_FILES) $(SUPRT_FILES) $(TRANS_FILES)
C_OBJS = $(C_FILES:%.c=$(BUILD_DIR)/%.o)
S_FILES = $(TRANS_DIR)/arm_bitreversal2.S
S_OBJS = $(S_FILES:%.S=$(BUILD_DIR)/%.o)

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
ALLFLAGS = -g -O0 -DDEBUG_IO
C_FILES += $(APP_PATH)/test_hostio.c
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
CFLAGS += -D_REENT_SMALL -DARM_MATH_CM4

###################################################
# include path
CFLAGS += -I$(SOURCE_DIR)/kernel/CMSIS/Include

.PHONY: $(TARGET_LIB).a

all: $(TARGET_LIB).a
	cp ./$(TARGET_LIB).a ../Lib/$(TARGET_LIB).a 

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

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.S
	@mkdir -p $(dir $@)
	@echo Build... $@
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OUTPATH) $(TARGET_LIB).a

ifneq ($(MAKECMDGOALS),clean)
-include $(C_OBJS:.o=.d)
endif
