#include <icm20645gy.h>
#include "sensor_manager.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <hal_i2c.h>
#include <FreeRTOSConfig.h>
#include <platform.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hwsen.h"
#include "API_sensor_calibration.h"
#include "mpe_cm4_API.h"



//#define ICM20645GY_LOG_MSG
#define GYRO_TAG                "[GYRO] "
#define GYRO_ERR(fmt, arg...)   PRINTF_D(GYRO_TAG"%d: "fmt, __LINE__, ##arg)

#ifdef ICM20645GY_LOG_MSG
#define GYRO_LOG(fmt, arg...)   PRINTF_D(GYRO_TAG fmt, ##arg)
#define GYRO_FUN(f)             PRINTF_D("%s\n\r", __FUNCTION__)
#else
#define GYRO_LOG(fmt, arg...)
#define GYRO_FUN(f)
#endif
#define TIMEBASE_PLL_CORRECT(delay_us, pll)     (delay_us * (1270ULL)/(1270ULL + pll))
#define ONE_EVENT_DELAY_PER_FIFO_LOOP 5347593
static unsigned int sensor_power = false;
static struct icm20645gy_data obj_data;
static struct data_resolution icm20645gy_data_resolution[] = {
    {ICM20645_FS_250_LSB},
    {ICM20645_FS_500_LSB},
    {ICM20645_FS_1000_LSB},
    {ICM20645_FS_2000_LSB},
};
extern unsigned int icm20645_gse_mode(void);

extern int mpu_i2c_write_block(unsigned char *data, unsigned char len);
extern int mpu_i2c_read_block(unsigned char *data, unsigned char len);

unsigned int icm20645_gyro_mode(void)
{
    return sensor_power;
}

static int icm20645gy_set_bank(unsigned char bank)
{
    int res = 0;
    unsigned char databuf[2];
    databuf[0] = REG_BANK_SEL;
    databuf[1] = bank;
    res = mpu_i2c_write_block(databuf, 2);
    if (res < 0) {
        GYRO_LOG("icm20645gy_set_bank fail at %x", bank);
        return ICM20645_ERR_I2C;
    }

    return ICM20645_SUCCESS;
}

static int icm20645gy_enable_fifo(int enable)
{
    int res = 0;
    unsigned char databuf[2];

    GYRO_FUN();
    icm20645gy_set_bank(BANK_SEL_0);
    if (enable) {
        databuf[0] = ICM20645_REG_FIFO_RST;
        databuf[1] = 0x1F;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GYRO_LOG("icm20645gy_enable_fifo fail at0 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
        databuf[0] = ICM20645_REG_FIFO_RST;
        databuf[1] = 0x1F - 1;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GYRO_LOG("icm20645gy_enable_fifo fail at0 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }

        databuf[0] = ICM20645_REG_FIFO_EN_2;
        res = mpu_i2c_read_block(databuf, 0x1);
        if (res < 0) {
            GYRO_LOG("icm20645gy_enable_fifo fail at3 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
        databuf[0] = databuf[0] | (0b00001110);
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_REG_FIFO_EN_2;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GYRO_LOG("icm20645gy_enable_fifo fail at4 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }

        databuf[0] = ICM20645_REG_USER_CTRL;
        res = mpu_i2c_read_block(databuf, 0x1);
        if (res < 0) {
            GYRO_LOG("icm20645gy_enable_fifo fail at1 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
        databuf[0] = databuf[0] | (1 << 6);
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_REG_USER_CTRL;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GYRO_LOG("icm20645gy_enable_fifo fail at2 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }

    } else {
        databuf[0] = ICM20645_REG_USER_CTRL;
        res = mpu_i2c_read_block(databuf, 0x1);
        if (res < 0) {
            GYRO_LOG("icm20645gy_enable_fifo fail at1 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
        databuf[0] = databuf[0] & ~(1 << 6);
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_REG_USER_CTRL;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GYRO_LOG("icm20645gy_enable_fifo fail at2 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }

        databuf[0] = ICM20645_REG_FIFO_EN_2;
        res = mpu_i2c_read_block(databuf, 0x1);
        if (res < 0) {
            GYRO_LOG("icm20645gy_enable_fifo fail at3 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
        databuf[0] = databuf[0] & ~(0b00001110);
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_REG_FIFO_EN_2;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GYRO_LOG("icm20645gy_enable_fifo fail at4 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
    }
    return ICM20645_SUCCESS;
}
static int icm20645gy_fifo_count(void)
{
    int res = 0;
    unsigned char databuf[2];
    unsigned char COUNTH = 0;
    unsigned char COUNTL = 0;
    unsigned int COUNT = 0;

    databuf[0] = ICM20645_REG_FIFO_COUNTH;
    if ((res = mpu_i2c_read_block(databuf, 0x1)) != 0) {
        GYRO_ERR("error: %d\n\r", res);
        return ICM20645_ERR_I2C;
    }
    COUNTH = databuf[0];

    databuf[0] = ICM20645_REG_FIFO_COUNTL;
    if ((res = mpu_i2c_read_block(databuf, 0x1)) != 0) {
        GYRO_ERR("error: %d\n\r", res);
        return ICM20645_ERR_I2C;
    }
    COUNTL = databuf[0];
    COUNT = (COUNTH << 8) | COUNTL;

    GYRO_LOG("icm20645gy_ReadData FIFO count: %d\n\r", COUNT);
    return COUNT;
}
static int icm20645gy_read_fifo(int count, struct data_t *output)
{
    int res = 0;
    unsigned char databuf[ICM20645_DATA_LEN];
    int N = 0, n = 0;
    int raw_data[GYRO_AXES_NUM];
    int remap_data[GYRO_AXES_NUM];
    struct icm20645gy_data *obj = &obj_data;
    MPE_SENSOR_DATA raw_data_input[MAX_GYROSCOPE_FIFO_SIZE], calibrated_data_output[MAX_GYROSCOPE_FIFO_SIZE];
    int dt = 0;
    int status;
    UINT64 timestamp = 0;
    static UINT64 last_timestamp = 0;
    N = count / ICM20645_DATA_LEN;
    n = count % ICM20645_DATA_LEN;
    if (N <= 0) {
        GYRO_ERR("raw data bytes 0 from fifo\n");
        return -1;
    } else if (N > GYROSCOPE_FIFO_THREASHOLD) {
        N = GYROSCOPE_FIFO_THREASHOLD;
    }
    if (n != 0)
        GYRO_LOG("raw data is not right for fifo\n");
    timestamp = read_xgpt_stamp_ns();
    output->data_exist_count = N;

    for (int i = 0; i < N; ++i) {
        databuf[0] = ICM20645_REG_FIFO_R_W;
        res = mpu_i2c_read_block(databuf, ICM20645_DATA_LEN);

        raw_data[GYRO_AXIS_X] = (short)((databuf[GYRO_AXIS_X * 2] << 8) | databuf[GYRO_AXIS_X * 2 + 1]);
        raw_data[GYRO_AXIS_Y] = (short)((databuf[GYRO_AXIS_Y * 2] << 8) | databuf[GYRO_AXIS_Y * 2 + 1]);
        raw_data[GYRO_AXIS_Z] = (short)((databuf[GYRO_AXIS_Z * 2] << 8) | databuf[GYRO_AXIS_Z * 2 + 1]);

        raw_data[GYRO_AXIS_X] = raw_data[GYRO_AXIS_X] + obj->cali_sw[GYRO_AXIS_X];
        raw_data[GYRO_AXIS_Y] = raw_data[GYRO_AXIS_Y] + obj->cali_sw[GYRO_AXIS_Y];
        raw_data[GYRO_AXIS_Z] = raw_data[GYRO_AXIS_Z] + obj->cali_sw[GYRO_AXIS_Z];

        /*remap coordinate*/
        remap_data[obj->cvt.map[GYRO_AXIS_X]] = obj->cvt.sign[GYRO_AXIS_X] * raw_data[GYRO_AXIS_X];
        remap_data[obj->cvt.map[GYRO_AXIS_Y]] = obj->cvt.sign[GYRO_AXIS_Y] * raw_data[GYRO_AXIS_Y];
        remap_data[obj->cvt.map[GYRO_AXIS_Z]] = obj->cvt.sign[GYRO_AXIS_Z] * raw_data[GYRO_AXIS_Z];

        /* Out put the degree/second(mo/s) */
        raw_data[GYRO_AXIS_X] =
            (int)((float)remap_data[GYRO_AXIS_X] * GYROSCOPE_INCREASE_NUM_SCP / obj->reso->sensitivity);
        raw_data[GYRO_AXIS_Y] =
            (int)((float)remap_data[GYRO_AXIS_Y] * GYROSCOPE_INCREASE_NUM_SCP / obj->reso->sensitivity);
        raw_data[GYRO_AXIS_Z] =
            (int)((float)remap_data[GYRO_AXIS_Z] * GYROSCOPE_INCREASE_NUM_SCP / obj->reso->sensitivity);
        //output->data[i].time_stamp = remap_timestamp(timestamp, i, N, hw_rate);
        output->data[i].sensor_type = SENSOR_TYPE_GYROSCOPE;
        output->data[i].time_stamp = obj->gyro_fifo_param.fifo_timestamp +
                                     (TIMEBASE_PLL_CORRECT(ONE_EVENT_DELAY_PER_FIFO_LOOP, obj->gyro_fifo_param.TB_PERCENT)) * i;
        raw_data_input[i].x = raw_data[GYRO_AXIS_X];
        raw_data_input[i].y = raw_data[GYRO_AXIS_Y];
        raw_data_input[i].z = raw_data[GYRO_AXIS_Z];
    }

    obj->gyro_fifo_param.fifo_timestamp = output->data[N - 1].time_stamp +
                                          (TIMEBASE_PLL_CORRECT(ONE_EVENT_DELAY_PER_FIFO_LOOP, obj->gyro_fifo_param.TB_PERCENT));
    dt = (int)(timestamp - last_timestamp);
    res = Gyro_run_calibration(dt, output->data_exist_count, raw_data_input, calibrated_data_output, &status);
    if (res < 0) {
        return res;
    }
    last_timestamp = timestamp;
    for (int i = 0; i < N; ++i) {
        output->data[i].sensor_type = SENSOR_TYPE_GYROSCOPE;
        output->data[i].gyroscope_t.x = calibrated_data_output[i].x;
        output->data[i].gyroscope_t.y = calibrated_data_output[i].y;
        output->data[i].gyroscope_t.z = calibrated_data_output[i].z;
        output->data[i].gyroscope_t.x_bias = (INT16)(calibrated_data_output[i].x - raw_data_input[i].x);
        output->data[i].gyroscope_t.y_bias = (INT16)(calibrated_data_output[i].y - raw_data_input[i].y);
        output->data[i].gyroscope_t.z_bias = (INT16)(calibrated_data_output[i].z - raw_data_input[i].z);
        output->data[i].gyroscope_t.status = (INT16)status;
        GYRO_LOG("calibrate time: %lld, x:%d, y:%d, z:%d, x_bias:%d, y_bias:%d, z_bias:%d\r\n",
                 output->data[i].time_stamp, output->data[i].gyroscope_t.x,
                 output->data[i].gyroscope_t.y, output->data[i].gyroscope_t.z,
                 output->data[i].gyroscope_t.x_bias, output->data[i].gyroscope_t.y_bias,
                 output->data[i].gyroscope_t.z_bias);
    }

    return res;
}
/*
static void clear_fifo(void)
{
    struct data_t output[256];
    int ret = 0, count = 0;
    count = icm20645gy_fifo_count();
    if (count != 0) {
        ret = icm20645gy_read_fifo(count,output);
    }

    if(ret < 0)
         GYRO_ERR("clear_fifo err! \n\r");
}
*/
static int icm20645gy_turn_on(unsigned char status, unsigned int on)
{
    int res = 0;
    unsigned char databuf[2];
    GYRO_FUN(f);
    icm20645gy_set_bank(BANK_SEL_0);
    databuf[0] = ICM20645_PWR_MGMT_2;
    res = mpu_i2c_read_block(databuf, 0x1);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    if (on == true) {
        databuf[0] &= ~status;
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_PWR_MGMT_2;
        res = mpu_i2c_write_block(databuf, 2);
        if (res < 0) {
            return ICM20645_ERR_I2C;
        }
    } else {
        databuf[0] |= status;
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_PWR_MGMT_2;
        res = mpu_i2c_write_block(databuf, 2);
        if (res < 0) {
            return ICM20645_ERR_I2C;
        }
    }
    return ICM20645_SUCCESS;

}

static int icm20645gy_lp_mode(unsigned int on)
{
    int res = 0;
    unsigned char databuf[2];

    icm20645gy_set_bank(BANK_SEL_0);
    GYRO_FUN(f);
    /* acc lp config */
    databuf[0] = ICM20645_REG_LP_CONFIG;
    res = mpu_i2c_read_block(databuf, 0x1);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    if (on == true) {
        databuf[0] |= BIT_GYRO_LP_EN;
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_REG_LP_CONFIG;
        res = mpu_i2c_write_block(databuf, 2);
        if (res < 0) {
            return ICM20645_ERR_I2C;
        }
    } else {
        databuf[0] &= ~BIT_GYRO_LP_EN;
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_REG_LP_CONFIG;
        res = mpu_i2c_write_block(databuf, 2);
        if (res < 0) {
            return ICM20645_ERR_I2C;
        }
    }
    return ICM20645_SUCCESS;

}
/*
*PWR_MGMT_1 bit le_en set to 1, then some register can not be write
*(except lp_config, pwr_mgmt_1, pwr_mgmt_2, int_pin_cfg, int_enable,
*fifo_count, fifo_r_w, set_bank), but all register can be read
*so we should first enable power, second set hw samplingrate,
*third cfg fifo, then set lowest power bit, see the operation function
*/
static int icm20645gy_lowest_power_mode(unsigned int on)
{
    int res = 0;
    unsigned char databuf[2];
    /* all chip lowest power config include accel and gyro*/

    icm20645gy_set_bank(BANK_SEL_0);
    databuf[0] = ICM20645_PWR_MGMT_1;
    res = mpu_i2c_read_block(databuf, 0x1);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    if (on == true) {
        databuf[0] |= BIT_LP_EN;
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_PWR_MGMT_1;
        res = mpu_i2c_write_block(databuf, 2);
        if (res < 0) {
            return ICM20645_ERR_I2C;
        }
    } else {
        databuf[0] &= ~BIT_LP_EN;
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_PWR_MGMT_1;
        res = mpu_i2c_write_block(databuf, 2);
        if (res < 0) {
            return ICM20645_ERR_I2C;
        }
    }
    return res;
}
// set the sample rate
static int icm20645gy_ReadTimeBase(mtk_int8 *time_percent)
{
    int res = 0;
    unsigned char databuf[2];

    icm20645gy_set_bank(BANK_SEL_1);
    databuf[0] = ICM20645_TIMEBASE_PLL;
    res = mpu_i2c_read_block(databuf, 0x1);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }

    *time_percent = databuf[0];
    return ICM20645_SUCCESS;
}

// set the sample rate
static int icm20645gy_SetSampleRate(int sample_rate)
{
    unsigned char buff[2];
    int res = 0;
    GYRO_FUN(f);

    if (sample_rate <= 4)
        buff[0] = 255;
    else if (sample_rate >= 1125)
        buff[0] = 0;
    else
        buff[0] = 1125 / sample_rate - 1;

    icm20645gy_set_bank(BANK_SEL_2);
    buff[1] = buff[0];
    buff[0] = ICM20645_REG_SAMRT_DIV;
    res = mpu_i2c_write_block(buff, 2);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    icm20645gy_set_bank(BANK_SEL_0);
    return ICM20645_SUCCESS;
}
static int icm20645gy_Setfilter(int filter_sample)
{
    unsigned char databuf[2] = {0};
    int res = 0;
    GYRO_FUN(f);

    icm20645gy_set_bank(BANK_SEL_2);

    databuf[0] = filter_sample;
    databuf[1] = databuf[0];
    databuf[0] = ICM20645_GYRO_CFG2;
    res = mpu_i2c_write_block(databuf, 2);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    icm20645gy_set_bank(BANK_SEL_0);
    return ICM20645_SUCCESS;
}

static int icm20645gy_SetPowerMode(unsigned int enable)
{

    unsigned char databuf[2];
    int res = 0;

    icm20645gy_set_bank(BANK_SEL_0);

    if (enable == sensor_power) {
        return ICM20645_SUCCESS;
    }
    databuf[0] = ICM20645_PWR_MGMT_1;
    res = mpu_i2c_read_block(databuf, 0x1);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    databuf[0] &= ~ICM20645_SLEEP;

    if (enable == false) {
        if (icm20645_gse_mode() == false) {
            databuf[0] |= ICM20645_SLEEP;
        } else {
            res = icm20645gy_lowest_power_mode(true);
            if (res != ICM20645_SUCCESS) {
                GYRO_ERR("icm20645gy_lowest_power_mode error\n\r");
                return ICM20645_ERR_I2C;
            }
        }
    }
    databuf[1] = databuf[0];
    databuf[0] = ICM20645_PWR_MGMT_1;
    res = mpu_i2c_write_block(databuf, 2);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    sensor_power = enable;
    return res;
}
static int icm20645gy_SetDataResolution(void)
{
    unsigned char databuf[2];
    int err = 0;
    unsigned char reso = 0;
    struct icm20645gy_data *obj = &obj_data;
    GYRO_FUN(f);
    databuf[0] = ICM20645_REG_CFG;
    if ((err = mpu_i2c_read_block(databuf, 1))) {
        return ICM20645_ERR_I2C;
    }
    reso = (databuf[0] & GYRO_FS_SEL_2000) >> 1;
    if (reso < sizeof(icm20645gy_data_resolution) / sizeof(icm20645gy_data_resolution[0])) {
        obj->reso = &icm20645gy_data_resolution[reso];
        GYRO_ERR("icm20645gy_data_resolution reso: %d, sensitivity: %f\n\r", reso, (double)obj->reso->sensitivity);
        return 0;
    } else {
        return -1;
    }
}

static int icm20645gy_SetDataFormat(unsigned char dataformat)
{
    unsigned char databuf[2];
    int res = 0;
    GYRO_FUN(f);

    icm20645gy_set_bank(BANK_SEL_2);

    /* write */
    databuf[0] = dataformat;
    databuf[1] = databuf[0];
    databuf[0] = ICM20645_REG_CFG;
    res = mpu_i2c_write_block(databuf, 2);

    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    res = icm20645gy_SetDataResolution();
    icm20645gy_set_bank(BANK_SEL_0);
    return res;
}
static int icm20645gy_ResetCalibration(void)
{
    struct icm20645gy_data *obj = &obj_data;

    GYRO_FUN(f);

    memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));

    return 0;
}
static int icm20645gy_write_rel_calibration(int *dat)
{
    struct icm20645gy_data *obj = &obj_data;
    obj->cali_sw[GYRO_AXIS_X] = obj->cvt.sign[GYRO_AXIS_X] * dat[obj->cvt.map[GYRO_AXIS_X]];
    obj->cali_sw[GYRO_AXIS_Y] = obj->cvt.sign[GYRO_AXIS_Y] * dat[obj->cvt.map[GYRO_AXIS_Y]];
    obj->cali_sw[GYRO_AXIS_Z] = obj->cvt.sign[GYRO_AXIS_Z] * dat[obj->cvt.map[GYRO_AXIS_Z]];
    GYRO_LOG("test  (%5d, %5d, %5d) ->(%5d, %5d, %5d)->(%5d, %5d, %5d))\n\r",
             obj->cvt.sign[GYRO_AXIS_X], obj->cvt.sign[GYRO_AXIS_Y], obj->cvt.sign[GYRO_AXIS_Z],
             dat[GYRO_AXIS_X], dat[GYRO_AXIS_Y], dat[GYRO_AXIS_Z],
             obj->cvt.map[GYRO_AXIS_X], obj->cvt.map[GYRO_AXIS_Y], obj->cvt.map[GYRO_AXIS_Z]);
    GYRO_LOG("write gyro calibration data  (%5d, %5d, %5d)\n\r",
             obj->cali_sw[GYRO_AXIS_X], obj->cali_sw[GYRO_AXIS_Y], obj->cali_sw[GYRO_AXIS_Z]);
    return 0;
}

static int icm20645gy_WriteCalibration(int *dat)
{
    int err = 0;
    int cali[GYRO_AXES_NUM];
    struct icm20645gy_data *obj = &obj_data;
    GYRO_FUN(f);
    if (!dat) {
        GYRO_ERR("null ptr!!\n\r");
        return -1;
    } else {
        dat[GYRO_AXIS_X] = dat[GYRO_AXIS_X] * obj->reso->sensitivity / GYROSCOPE_INCREASE_NUM_AP;
        dat[GYRO_AXIS_Y] = dat[GYRO_AXIS_Y] * obj->reso->sensitivity / GYROSCOPE_INCREASE_NUM_AP;
        dat[GYRO_AXIS_Z] = dat[GYRO_AXIS_Z] * obj->reso->sensitivity / GYROSCOPE_INCREASE_NUM_AP;

        cali[obj->cvt.map[GYRO_AXIS_X]] = obj->cvt.sign[GYRO_AXIS_X] * obj->cali_sw[GYRO_AXIS_X];
        cali[obj->cvt.map[GYRO_AXIS_Y]] = obj->cvt.sign[GYRO_AXIS_Y] * obj->cali_sw[GYRO_AXIS_Y];
        cali[obj->cvt.map[GYRO_AXIS_Z]] = obj->cvt.sign[GYRO_AXIS_Z] * obj->cali_sw[GYRO_AXIS_Z];

        cali[GYRO_AXIS_X] += dat[GYRO_AXIS_X];
        cali[GYRO_AXIS_Y] += dat[GYRO_AXIS_Y];
        cali[GYRO_AXIS_Z] += dat[GYRO_AXIS_Z];

        GYRO_LOG("write gyro calibration data  (%5d, %5d, %5d)-->(%5d, %5d, %5d)\n\r",
                 dat[GYRO_AXIS_X], dat[GYRO_AXIS_Y], dat[GYRO_AXIS_Z],
                 cali[GYRO_AXIS_X], cali[GYRO_AXIS_Y], cali[GYRO_AXIS_Z]);

        return icm20645gy_write_rel_calibration(cali);
    }

    return err;
}

static int icm20645gy_ReadData(int *data)
{
    unsigned char buf[ICM20645_DATA_LEN] = {0};
    int err = 0;

    GYRO_FUN(f);
    buf[0] = ICM20645_REG_GYRO_XH;
    if ((err = mpu_i2c_read_block(buf, ICM20645_DATA_LEN)) != 0) {
        GYRO_ERR("error: %d\n\r", err);
        return ICM20645_ERR_I2C;
    } else {
        GYRO_LOG("read byte : %x, %x, %x, %x, %x, %x\n\r",
                 buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        /* little and big debian  need check */
        data[GYRO_AXIS_X] = (short)((buf[GYRO_AXIS_X * 2] << 8) | (buf[GYRO_AXIS_X * 2 + 1]));
        data[GYRO_AXIS_Y] = (short)((buf[GYRO_AXIS_Y * 2] << 8) | (buf[GYRO_AXIS_Y * 2 + 1]));
        data[GYRO_AXIS_Z] = (short)((buf[GYRO_AXIS_Z * 2] << 8) | (buf[GYRO_AXIS_Z * 2 + 1]));

        GYRO_LOG("[%08X %08X %08X] => [%5d %5d %5d]\n\r", data[GYRO_AXIS_X], data[GYRO_AXIS_Y], data[GYRO_AXIS_Z],
                 data[GYRO_AXIS_X], data[GYRO_AXIS_Y], data[GYRO_AXIS_Z]);
    }
    return err;
}

int icm20645gy_ReadSensorData(int *data)
{
    struct icm20645gy_data *obj = &obj_data;
    int res = 0;
    int remap_data[GYRO_AXES_NUM];

    GYRO_FUN(f);

    if (0 != (res = icm20645gy_ReadData(data))) {
        GYRO_ERR("icm20645gy_ReadData error: ret value=%d\n\r", res);
        return ICM20645_ERR_I2C;
    } else {
        data[GYRO_AXIS_X] = data[GYRO_AXIS_X] + obj->cali_sw[GYRO_AXIS_X];
        data[GYRO_AXIS_Y] = data[GYRO_AXIS_Y] + obj->cali_sw[GYRO_AXIS_Y];
        data[GYRO_AXIS_Z] = data[GYRO_AXIS_Z] + obj->cali_sw[GYRO_AXIS_Z];
        /*remap coordinate*/
        remap_data[obj->cvt.map[GYRO_AXIS_X]] = obj->cvt.sign[GYRO_AXIS_X] * data[GYRO_AXIS_X];
        remap_data[obj->cvt.map[GYRO_AXIS_Y]] = obj->cvt.sign[GYRO_AXIS_Y] * data[GYRO_AXIS_Y];
        remap_data[obj->cvt.map[GYRO_AXIS_Z]] = obj->cvt.sign[GYRO_AXIS_Z] * data[GYRO_AXIS_Z];
        //Out put the 1000*degree/second(o/s)
        data[GYRO_AXIS_X] =
            (int)((float)remap_data[GYRO_AXIS_X] * GYROSCOPE_INCREASE_NUM_SCP / obj->reso->sensitivity);
        data[GYRO_AXIS_Y] =
            (int)((float)remap_data[GYRO_AXIS_Y] * GYROSCOPE_INCREASE_NUM_SCP / obj->reso->sensitivity);
        data[GYRO_AXIS_Z] =
            (int)((float)remap_data[GYRO_AXIS_Z] * GYROSCOPE_INCREASE_NUM_SCP / obj->reso->sensitivity);
        GYRO_LOG("output data: %d, %d, %d!\n\r", data[GYRO_AXIS_X], data[GYRO_AXIS_Y], data[GYRO_AXIS_Z]);
    }

    return 0;
}
int icm20645_gse_fifo_status(void)
{
    int err = 0;
    unsigned char databuf[2];

    err = icm20645gy_SetPowerMode(true);
    databuf[0] = ICM20645_REG_FIFO_EN_2;
    err = mpu_i2c_read_block(databuf, 0x1);
    if (err < 0) {
        GYRO_ERR("icm20645gy_enable_fifo fail at3 %x\n\r", err);
        return ICM20645_ERR_I2C;
    }
    if ((databuf[0] & (1 << 4)) != 0) //(0b00010000)
        return true;
    else
        return false;
}
static int cust_set(void *data, int len)
{
    int err = 0;
    struct icm20645gy_data *obj = &obj_data;
    CUST_SET_REQ_P req = (CUST_SET_REQ_P)data;

    GYRO_FUN(f);

    switch (req->cust.action) {
        case CUST_ACTION_SET_CUST:
            break;
        case CUST_ACTION_SET_CALI:
            GYRO_LOG("CUST_ACTION_SET_CALI\n\r");
            err = icm20645gy_WriteCalibration(req->setCali.int32_data);
            GYRO_LOG("%d, %d, %d\n\r", obj_data.cali_sw[0], obj_data.cali_sw[1], obj_data.cali_sw[2]);
            break;
        case CUST_ACTION_RESET_CALI:
            GYRO_LOG("CUST_ACTION_RESET_CALI\n\r");
            err = icm20645gy_ResetCalibration();
            GYRO_LOG("%d, %d, %d\n\r", obj_data.cali_sw[0], obj_data.cali_sw[1], obj_data.cali_sw[2]);
            break;
        case CUST_ACTION_SET_DIRECTION:
            GYRO_LOG("CUST_ACTION_SET_ORIENTATION\n\r");
            obj->hw->direction = (req->setDirection.direction);
            GYRO_LOG("set orientation: %d\n\r", obj->hw->direction);
            if (0 != (err = sensor_driver_get_convert(obj->hw->direction, &obj->cvt))) {
                GYRO_ERR("invalid direction: %d\n\r", obj->hw->direction);
            }
            break;
        case CUST_ACTION_SHOW_REG:

            break;
        case CUST_ACTION_SET_TRACE:
            obj->trace = (req->setTrace.trace);
            break;
        case CUST_ACTION_SET_FACTORY:
            obj->use_in_factory = (req->setFactory.factory);
            break;
        default:
            err = -1;
            break;
    }
    return err;
}
int icm20645gy_init_client(void)
{
    int res = 0;
    struct icm20645gy_data *obj = &obj_data;

    GYRO_FUN(f);

    res = icm20645gy_SetPowerMode(true);
    if (res != ICM20645_SUCCESS) {
        GYRO_ERR("icm20645g_SetPowerMode error\n\r");
        return res;
    }
    res = icm20645gy_turn_on(BIT_PWR_GYRO_STBY, false);
    if (res != ICM20645_SUCCESS) {
        GYRO_ERR("icm20645gy_turn_on error\n\r");
        return res;
    }

    res = icm20645gy_lp_mode(true);
    if (res != ICM20645_SUCCESS) {
        GYRO_ERR("icm20645gy_lp_mode error\n\r");
        return res;
    }

    res = icm20645gy_SetDataFormat(GYRO_FS_SEL_2000 | GYRO_FCHOICE | GYRO_DLPFCFG);
    if (res != ICM20645_SUCCESS) {
        GYRO_ERR("icm20645gy_SetDataFormat error\n\r");
        return res;
    }
    res = icm20645gy_Setfilter(GYRO_AVGCFG_2X);
    if (res != ICM20645_SUCCESS) {
        GYRO_ERR("icm20645gy_Setfilter ERR!\n\r");
        return res;
    }
    res = icm20645gy_SetSampleRate(10);
    if (res != ICM20645_SUCCESS) {
        GYRO_ERR("icm20645gy_SetSampleRate error\n\r");
        return res;
    }
    res = icm20645gy_ReadTimeBase(&obj->gyro_fifo_param.TB_PERCENT);
    if (res != ICM20645_SUCCESS) {
        GYRO_ERR("icm20645gy_ReadTimeBase error\n\r");
        return res;
    }
    res = icm20645gy_SetPowerMode(false);
    if (res != ICM20645_SUCCESS) {
        GYRO_ERR("icm20645g_SetPowerMode error\n\r");
        return res;
    }

    icm20645gy_set_bank(BANK_SEL_0);
    return res;
}

int icm20645gy_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;
    struct icm20645gy_data *obj = &obj_data;
    unsigned char databuf[2];

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            GYRO_LOG("icm20645gy_operation command ACTIVATE\n\r");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                GYRO_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (SENSOR_DISABLE == value) {
                    //clear_fifo();
                    err = icm20645gy_lowest_power_mode(false);
                    if (err != ICM20645_SUCCESS) {
                        GYRO_ERR("icm20645gy_lowest_power_mode error\n\r");
                        err = -1;
                    }
                    icm20645gy_turn_on(BIT_PWR_GYRO_STBY, false);
                    err = icm20645gy_SetPowerMode(false);
                    if (err < 0) {
                        GYRO_LOG("icm20645gy_SetPowerMode off fail\n\r");
                        err = -1;
                    }
                } else {
                    err = icm20645gy_SetPowerMode(true);
                    if (err < 0) {
                        GYRO_LOG("icm20645gy_SetPowerMode on fail\n\r");
                        err = -1;
                    }
                    icm20645gy_turn_on(BIT_PWR_GYRO_STBY, true);
                    err = icm20645gy_lowest_power_mode(true);
                    if (err != ICM20645_SUCCESS) {
                        GYRO_ERR("icm20645gy_lowest_power_mode error\n\r");
                        err = -1;
                    }
                }
            }
            break;
        case SETDELAY:
            GYRO_LOG("icm20645gy_operation command SETDELAY (%d)\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                GYRO_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (value <= GYRO_DELAY_PER_FIFO_LOOP) {
                    /* enable HW fifo */
                    obj->gyro_fifo_param.hw_rate = 1000 * GYRO_EVENT_COUNT_PER_FIFO_LOOP / value;
                } else {
                    /* disable HW fifo */
                    obj->gyro_fifo_param.hw_rate = 1000 / value;
                }
                err = icm20645gy_SetPowerMode(true);
                if (err < 0) {
                    GYRO_LOG("icm20645gy_SetPowerMode on fail\n\r");
                    err = -1;
                }
                err = icm20645gy_lowest_power_mode(false);
                if (err != ICM20645_SUCCESS) {
                    GYRO_ERR("icm20645gy_lowest_power_mode error\n\r");
                    err = -1;
                }

                if (value <= GYRO_DELAY_PER_FIFO_LOOP) {
                    err = icm20645gy_SetSampleRate(187);
                    if (err != ICM20645_SUCCESS) {
                        GYRO_ERR("icm20645gy_SetSampleRate error\n\r");
                        err = -1;
                    }
                } else {
                    err = icm20645gy_SetSampleRate(obj->gyro_fifo_param.hw_rate);
                    if (err != ICM20645_SUCCESS) {
                        GYRO_ERR("icm20645gy_SetSampleRate error\n\r");
                        err = -1;
                    }
                }
                err = icm20645gy_lowest_power_mode(true);
                if (err != ICM20645_SUCCESS) {
                    GYRO_ERR("icm20645gy_lowest_power_mode error\n\r");
                    err = -1;
                }
            }
            break;
        case SETCUST:
            GYRO_LOG("icm20645gy_operation command SETCUST\n\r");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                GYRO_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                if (err != cust_set(buffer_in, size_in)) {
                    GYRO_ERR("Set customization error : %d\n\r", err);
                }
            }
            break;
        case ENABLEFIFO:
            GYRO_LOG("icm20645gy_operation command ENABLEFIFO\n\r");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                GYRO_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (value == FIFO_ENABLE) {
                    if (obj->gyro_fifo_param.fifo_enable != FIFO_ENABLE) {
                        //GYRO_ERR("icm20645gy_operation real FIFO_ENABLE\n\r");
                        obj->gyro_fifo_param.fifo_enable = FIFO_ENABLE;
                        err = icm20645gy_SetPowerMode(true);
                        err = icm20645gy_lowest_power_mode(false);
                        err = icm20645gy_enable_fifo(FIFO_ENABLE);
                        if (err < 0)
                            GYRO_LOG("icm20645gy_enable_fifo enable fail\n\r");
                        obj->gyro_fifo_param.fifo_timestamp = read_xgpt_stamp_ns();
                        obj->gyro_fifo_param.fifo_timestamp +=
                            (UINT64)(TIMEBASE_PLL_CORRECT(ONE_EVENT_DELAY_PER_FIFO_LOOP, obj->gyro_fifo_param.TB_PERCENT));

                        err = icm20645gy_lowest_power_mode(true);
                    }
                } else {
                    if (obj->gyro_fifo_param.fifo_enable != FIFO_DISABLE) {
                        //GYRO_ERR("icm20645gy_operation real FIFO_DISABLE\n\r");
                        obj->gyro_fifo_param.fifo_enable = FIFO_DISABLE;
                        if (!icm20645_gse_fifo_status()) {
                            err = icm20645gy_SetPowerMode(true);
                            err = icm20645gy_lowest_power_mode(false);
                            err = icm20645gy_enable_fifo(FIFO_DISABLE);
                            if (err < 0)
                                GYRO_ERR("icm20645gy_enable_fifo enable fail\n\r");
                            err = icm20645gy_lowest_power_mode(true);
                        } else {
                            err = icm20645gy_SetPowerMode(true);
                            err = icm20645gy_lowest_power_mode(false);
                            databuf[0] = ICM20645_REG_FIFO_EN_2;
                            err = mpu_i2c_read_block(databuf, 0x1);
                            if (err < 0) {
                                GYRO_ERR("icm20645gy_enable_fifo fail at3 %x\n\r", err);
                                return ICM20645_ERR_I2C;
                            }
                            databuf[0] = databuf[0] & ~(0b00001110);
                            databuf[1] = databuf[0];
                            databuf[0] = ICM20645_REG_FIFO_EN_2;
                            err = mpu_i2c_write_block(databuf, 0x2);
                            if (err < 0) {
                                GYRO_ERR("icm20645gy_enable_fifo fail at4 %x\n\r", err);
                                return ICM20645_ERR_I2C;
                            }
                            err = icm20645gy_lowest_power_mode(true);
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
    return err;
}

int icm20645gy_run_algorithm(struct data_t *output)
{
    struct icm20645gy_data *obj = &obj_data;
    int ret = 0, count = 0;
    int buff[GYRO_AXES_NUM];
    int status;
    int dt;
    static UINT64 last_time_stamp;
    MPE_SENSOR_DATA raw_data_input;
    MPE_SENSOR_DATA calibrated_data_output;
    switch (obj->gyro_fifo_param.fifo_enable) {
        case FIFO_ENABLE:
            count = icm20645gy_fifo_count();
            if (count != 0) {
                ret = icm20645gy_read_fifo(count, output);
            }
            break;
        case FIFO_DISABLE:
            ret = icm20645gy_ReadSensorData(buff);
            if (ret < 0) {
                return ret;
            }
            switch (obj->use_in_factory) {
                case USE_IN_FACTORY:
                    output->data_exist_count = 1;
                    output->data->sensor_type = SENSOR_TYPE_GYROSCOPE;
                    output->data->time_stamp = read_xgpt_stamp_ns();
                    output->data->gyroscope_t.x = buff[GYRO_AXIS_X];
                    output->data->gyroscope_t.y = buff[GYRO_AXIS_Y];
                    output->data->gyroscope_t.z = buff[GYRO_AXIS_Z];
                    output->data->gyroscope_t.x_bias = 0;
                    output->data->gyroscope_t.y_bias = 0;
                    output->data->gyroscope_t.z_bias = 0;
                    GYRO_LOG("x:%d, y:%d, z:%d, x_bias:%d, y_bias:%d, z_bias:%d\r\n", output->data->gyroscope_t.x,
                             output->data->gyroscope_t.y, output->data->gyroscope_t.z,
                             output->data->gyroscope_t.x_bias, output->data->gyroscope_t.y_bias,
                             output->data->gyroscope_t.z_bias);
                    break;
                case NOT_USE_IN_FACTORY:
                    output->data_exist_count = 1;
                    output->data->sensor_type = SENSOR_TYPE_GYROSCOPE;
                    output->data->time_stamp = read_xgpt_stamp_ns();
                    raw_data_input.x = buff[GYRO_AXIS_X];
                    raw_data_input.y = buff[GYRO_AXIS_Y];
                    raw_data_input.z = buff[GYRO_AXIS_Z];
                    dt = (int)(output->data->time_stamp - last_time_stamp);
                    //GYRO_ERR("dt:%d, x:%d, y:%d, z:%d\r\n", dt, raw_data_input.x, raw_data_input.y, raw_data_input.z);
                    ret = Gyro_run_calibration(dt, output->data_exist_count, &raw_data_input, &calibrated_data_output, &status);
                    if (ret < 0) {
                        return ret;
                    }
                    output->data->gyroscope_t.x = calibrated_data_output.x;
                    output->data->gyroscope_t.y = calibrated_data_output.y;
                    output->data->gyroscope_t.z = calibrated_data_output.z;
                    output->data->gyroscope_t.x_bias = (INT16)(calibrated_data_output.x - raw_data_input.x);
                    output->data->gyroscope_t.y_bias = (INT16)(calibrated_data_output.y - raw_data_input.y);
                    output->data->gyroscope_t.z_bias = (INT16)(calibrated_data_output.z - raw_data_input.z);
                    output->data->gyroscope_t.status = (INT16)status;
                    GYRO_LOG("time: %lld, x:%d, y:%d, z:%d, x_bias:%d, y_bias:%d, z_bias:%d\r\n", output->data->time_stamp,
                             output->data->gyroscope_t.x,
                             output->data->gyroscope_t.y, output->data->gyroscope_t.z,
                             output->data->gyroscope_t.x_bias, output->data->gyroscope_t.y_bias,
                             output->data->gyroscope_t.z_bias);
                    last_time_stamp = output->data->time_stamp;
                    break;
            }
            break;
    }
    return ret;
}
//#define ICM20645GY_TESTSUITE_CASE
#ifdef ICM20645GY_TESTSUITE_CASE
static void ICM20645GYtestsample(void *pvParameters)
{
    struct icm20645gy_data *obj = &obj_data;
    while (1) {
        kal_taskENTER_CRITICAL();
        //icm20645gy_run_algorithm(obj->psensor_event_t);
        //GYRO_ERR("GYROSCOPE X: %d, Y: %d, Z: %d\n\r", obj->psensor_event_t->data->gyroscope_t.x,
        //obj->psensor_event_t->data->gyroscope_t.y, obj->psensor_event_t->data->gyroscope_t.z);
        kal_taskEXIT_CRITICAL();
        vTaskDelay(200 / portTICK_RATE_MS);
    }
}
#endif
int icm20645gy_sensor_init(void)
{
    int ret = 0;
    GYRO_FUN(f);
    struct SensorDescriptor_t  icm20645gy_descriptor_t;
    struct icm20645gy_data *obj = &obj_data;
    MPE_SENSOR_DATA bias;

    memset(obj, 0, sizeof(struct icm20645gy_data));
    obj->gyro_fifo_param.fifo_enable = FIFO_DISABLE;
    obj->use_in_factory = NOT_USE_IN_FACTORY;
    bias.x = 0;
    bias.y = 0;
    bias.z = 0;
    icm20645gy_descriptor_t.sensor_type = SENSOR_TYPE_GYROSCOPE;
    icm20645gy_descriptor_t.version =  1;
    icm20645gy_descriptor_t.report_mode = continus;
    icm20645gy_descriptor_t.hw.max_sampling_rate = 5;
    icm20645gy_descriptor_t.hw.support_HW_FIFO = true;

    icm20645gy_descriptor_t.input_list = NULL;

    icm20645gy_descriptor_t.operate = icm20645gy_operation;
    icm20645gy_descriptor_t.run_algorithm = icm20645gy_run_algorithm;
    icm20645gy_descriptor_t.set_data = NULL;

    obj->hw = get_cust_gyro_hw();
    if (NULL == obj->hw) {
        GYRO_ERR("get_cust_acc_hw fail\n\r");
        return ret;
    }
    GYRO_LOG("i2c_num: %d, i2c_addr: 0x%x\n\r", obj->hw->i2c_num, obj->hw->i2c_addr[0]);

    if (0 != (ret = sensor_driver_get_convert(obj->hw->direction, &obj->cvt))) {
        GYRO_ERR("invalid direction: %d\n\r", obj->hw->direction);
    }
    GYRO_LOG("map[0]:%d, map[1]:%d, map[2]:%d, sign[0]:%d, sign[1]:%d, sign[2]:%d\n\r",
             obj->cvt.map[GYRO_AXIS_X], obj->cvt.map[GYRO_AXIS_Y], obj->cvt.map[GYRO_AXIS_Z],
             obj->cvt.sign[GYRO_AXIS_X], obj->cvt.sign[GYRO_AXIS_Y], obj->cvt.sign[GYRO_AXIS_Z]);
    ret = icm20645gy_init_client();
    if (ret < 0) {
        GYRO_ERR("icm20645gy_init_client fail\n\r");
        return ret;
    }
    ret = icm20645gy_ResetCalibration();
    if (ret < 0) {
        GYRO_ERR("icm20645gy_ResetCalibration fail\n\r");
        return ret;
    }
    Gyro_init_calibration(bias);
    icm20645_set_timebase_correction_parameter(obj->gyro_fifo_param.TB_PERCENT);
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_GYROSCOPE, MAX_GYROSCOPE_FIFO_SIZE);
    if (ret) {
        GYRO_ERR("RegisterDataBuffer fail\n\r");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_type(&icm20645gy_descriptor_t);
    if (ret) {
        GYRO_ERR("RegisterAlgorithm fail\n\r");
        return ret;
    }
#ifdef ICM20645GY_TESTSUITE_CASE
    kal_xTaskCreate(ICM20645GYtestsample, "ICMGY", 512, (void*)NULL, 3, NULL);
#endif
    return ret;
}
MODULE_DECLARE(icm20645gy, MOD_PHY_SENSOR, icm20645gy_sensor_init);
