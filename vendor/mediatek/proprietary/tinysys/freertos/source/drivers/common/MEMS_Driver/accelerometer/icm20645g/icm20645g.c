#include <icm20645g.h>
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

/*#define ICM20645G_LOG_MSG*/
#define GSE_TAG                "[gsensor] "
#define GSE_ERR(fmt, arg...)   PRINTF_D(GSE_TAG"%d: "fmt, __LINE__, ##arg)

#ifdef ICM20645G_LOG_MSG
#define GSE_LOG(fmt, arg...)   PRINTF_D(GSE_TAG fmt, ##arg)
#define GSE_FUN(f)             PRINTF_D("%s\n\r", __FUNCTION__)
#else
#define GSE_LOG(fmt, arg...)
#define GSE_FUN(f)
#endif
static unsigned int sensor_power = false;
static unsigned int use_in_factory = false;
static struct icm20645g_data obj_data;
static int icm20645g_set_bank(unsigned char bank);
int mpu_i2c_write_block(unsigned char *data, unsigned char len);
int mpu_i2c_read_block(unsigned char *data, unsigned char len);
int mpu_i2c_read_block_fifo(unsigned char *data, unsigned char len);


#define G_TIMEBASE_PLL_CORRECT(delay_us, pll)     (delay_us * (1270ULL)/(1270ULL + pll))
#define ONE_EVENT_DELAY_PER_FIFO_LOOP 20449898  //20449898: 20ms per read; 9775171: 10ms per read

extern unsigned int icm20645_gyro_mode(void);
static struct data_resolution icm20645g_data_resolution[] = {
    {{ 0,  6}, 16384},  /*+/-2g  in 16-bit resolution:  0.06 mg/LSB*/
    {{ 0, 12}, 8192},   /*+/-4g  in 16-bit resolution:  0.12 mg/LSB*/
    {{ 0, 24}, 4096},   /*+/-8g  in 16-bit resolution:  0.24 mg/LSB*/
    {{ 0, 5}, 2048},    /*+/-16g in 16-bit resolution:  0.49 mg/LSB*/
};

static int icm20645g_enable_fifo(int enable)
{
    int res = 0;
    unsigned char databuf[2];

    GSE_FUN();
    icm20645g_set_bank(BANK_SEL_0);
    if (enable) {
        databuf[0] = G_ICM20645_REG_FIFO_RST;
        databuf[1] = 0x1F;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GSE_LOG("icm20645gy_enable_fifo fail at0 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
        databuf[0] = G_ICM20645_REG_FIFO_RST;
        databuf[1] = 0x1F - 1;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GSE_LOG("icm20645gy_enable_fifo fail at0 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }

        databuf[0] = G_ICM20645_REG_FIFO_EN_2;
        res = mpu_i2c_read_block(databuf, 0x1);
        if (res < 0) {
            GSE_LOG("icm20645gy_enable_fifo fail at3 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
        databuf[0] = databuf[0] | (0b00010000);
        databuf[1] = databuf[0];
        databuf[0] = G_ICM20645_REG_FIFO_EN_2;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GSE_LOG("icm20645gy_enable_fifo fail at4 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }

        databuf[0] = G_ICM20645_REG_USER_CTRL;
        res = mpu_i2c_read_block(databuf, 0x1);
        if (res < 0) {
            GSE_LOG("icm20645gy_enable_fifo fail at1 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
        databuf[0] = databuf[0] | (1 << 6);
        databuf[1] = databuf[0];
        databuf[0] = G_ICM20645_REG_USER_CTRL;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GSE_LOG("icm20645gy_enable_fifo fail at2 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }

    } else {
        databuf[0] = G_ICM20645_REG_USER_CTRL;
        res = mpu_i2c_read_block(databuf, 0x1);
        if (res < 0) {
            GSE_LOG("icm20645gy_enable_fifo fail at1 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
        databuf[0] = databuf[0] & ~(1 << 6);
        databuf[1] = databuf[0];
        databuf[0] = G_ICM20645_REG_USER_CTRL;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GSE_LOG("icm20645gy_enable_fifo fail at2 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }

        databuf[0] = G_ICM20645_REG_FIFO_EN_2;
        res = mpu_i2c_read_block(databuf, 0x1);
        if (res < 0) {
            GSE_LOG("icm20645gy_enable_fifo fail at3 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
        databuf[0] = databuf[0] & ~(0b00010000);
        databuf[1] = databuf[0];
        databuf[0] = G_ICM20645_REG_FIFO_EN_2;
        res = mpu_i2c_write_block(databuf, 0x2);
        if (res < 0) {
            GSE_LOG("icm20645gy_enable_fifo fail at4 %x\n\r", res);
            return ICM20645_ERR_I2C;
        }
    }
    return ICM20645_SUCCESS;
}

static int icm20645g_fifo_count(void)
{
    int res = 0;
    unsigned char databuf[2];
    //unsigned char COUNTH = 0;
    //unsigned char COUNTL = 0;
    unsigned int COUNT = 0;

    /*databuf[0] = G_ICM20645_REG_FIFO_COUNTH;
    if ((res = mpu_i2c_read_block(databuf, 0x1)) != 0) {
        GSE_LOG("error: %d\n\r", res);
        return ICM20645_ERR_I2C;
    }
    COUNTH = databuf[0];*/

    databuf[0] = G_ICM20645_REG_FIFO_COUNTL;
    if ((res = mpu_i2c_read_block(databuf, 0x1)) != 0) {
        GSE_LOG("error: %d\n\r", res);
        return ICM20645_ERR_I2C;
    }
    COUNT = databuf[0];

    GSE_LOG("icm20645g_ReadData FIFO count: %d\n\r", COUNT);
    return COUNT;
}
static int icm20645g_read_fifo(int count, struct data_t *output)
{
    int res = 0;
    unsigned char databuf[ICM20645_DATA_LEN];
    int N = 0, n = 0;
    int raw_data[ACCEL_AXES_NUM];
    int remap_data[ACCEL_AXES_NUM];
    struct icm20645g_data *obj = &obj_data;
    MPE_SENSOR_DATA raw_data_input[MAX_ACCELERATION_FIFO_SIZE], calibrated_data_output[MAX_ACCELERATION_FIFO_SIZE];
    int ret = 0;
    int dt = 0;
    int status;
    UINT64 timestamp = 0;
    static UINT64 last_timestamp = 0;
    N = count / ICM20645_DATA_LEN;
    n = count % ICM20645_DATA_LEN;

    if (N <= 0) {
        GSE_LOG("raw data bytes 0 from fifo\n");
        return -1;
    } else if (N > ACCELERATION_FIFO_THREASHOLD) {
        N = ACCELERATION_FIFO_THREASHOLD;
    }
    if (n != 0)
        GSE_LOG("raw data is not right for fifo\n");
    timestamp = read_xgpt_stamp_ns();
    output->data_exist_count = N;

    for (int i = 0; i < N; ++i) {
        databuf[0] = G_ICM20645_REG_FIFO_R_W;
        res = mpu_i2c_read_block_fifo(databuf, ICM20645_DATA_LEN);

        raw_data[ACCEL_AXIS_X] = (short)((databuf[ACCEL_AXIS_X * 2] << 8) | databuf[ACCEL_AXIS_X * 2 + 1]);
        raw_data[ACCEL_AXIS_Y] = (short)((databuf[ACCEL_AXIS_Y * 2] << 8) | databuf[ACCEL_AXIS_Y * 2 + 1]);
        raw_data[ACCEL_AXIS_Z] = (short)((databuf[ACCEL_AXIS_Z * 2] << 8) | databuf[ACCEL_AXIS_Z * 2 + 1]);

        raw_data[ACCEL_AXIS_X] = raw_data[ACCEL_AXIS_X] + obj->cali_sw[ACCEL_AXIS_X];
        raw_data[ACCEL_AXIS_Y] = raw_data[ACCEL_AXIS_Y] + obj->cali_sw[ACCEL_AXIS_Y];
        raw_data[ACCEL_AXIS_Z] = raw_data[ACCEL_AXIS_Z] + obj->cali_sw[ACCEL_AXIS_Z];

        /*remap coordinate*/
        remap_data[obj->cvt.map[ACCEL_AXIS_X]] = obj->cvt.sign[ACCEL_AXIS_X] * raw_data[ACCEL_AXIS_X];
        remap_data[obj->cvt.map[ACCEL_AXIS_Y]] = obj->cvt.sign[ACCEL_AXIS_Y] * raw_data[ACCEL_AXIS_Y];
        remap_data[obj->cvt.map[ACCEL_AXIS_Z]] = obj->cvt.sign[ACCEL_AXIS_Z] * raw_data[ACCEL_AXIS_Z];

        /* Out put the degree/second(mo/s) */
        raw_data[ACCEL_AXIS_X] =
            (int)((float)remap_data[ACCEL_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity);
        raw_data[ACCEL_AXIS_Y] =
            (int)((float)remap_data[ACCEL_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity);
        raw_data[ACCEL_AXIS_Z] =
            (int)((float)remap_data[ACCEL_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity);
        //output->data[i].time_stamp = remap_timestamp(timestamp, i, N, hw_rate);
        output->data[i].sensor_type = SENSOR_TYPE_ACCELEROMETER;
        output->data[i].time_stamp = obj->g_fifo_param.fifo_timestamp +
                                     (G_TIMEBASE_PLL_CORRECT(ONE_EVENT_DELAY_PER_FIFO_LOOP, obj->g_fifo_param.TB_PERCENT)) * i;
        raw_data_input[i].x = raw_data[ACCEL_AXIS_X];
        raw_data_input[i].y = raw_data[ACCEL_AXIS_Y];
        raw_data_input[i].z = raw_data[ACCEL_AXIS_Z];
    }

    obj->g_fifo_param.fifo_timestamp = output->data[N - 1].time_stamp +
                                       (G_TIMEBASE_PLL_CORRECT(ONE_EVENT_DELAY_PER_FIFO_LOOP, obj->g_fifo_param.TB_PERCENT));
    dt = (int)(timestamp - last_timestamp);
    ret = Acc_run_calibration(dt, output->data_exist_count, raw_data_input, calibrated_data_output, &status);
    if (ret < 0) {
        return ret;
    }
    last_timestamp = timestamp;
    for (int i = 0; i < N; ++i) {
        output->data[i].sensor_type = SENSOR_TYPE_ACCELEROMETER;
        output->data[i].accelerometer_t.x = calibrated_data_output[i].x;
        output->data[i].accelerometer_t.y = calibrated_data_output[i].y;
        output->data[i].accelerometer_t.z = calibrated_data_output[i].z;
        output->data[i].accelerometer_t.x_bias = (INT16)(calibrated_data_output[i].x - raw_data_input[i].x);
        output->data[i].accelerometer_t.y_bias = (INT16)(calibrated_data_output[i].y - raw_data_input[i].y);
        output->data[i].accelerometer_t.z_bias = (INT16)(calibrated_data_output[i].z - raw_data_input[i].z);
        output->data[i].accelerometer_t.status = (INT16)status;
        GSE_LOG("calibrate time: %lld, x:%d, y:%d, z:%d, x_bias:%d, y_bias:%d, z_bias:%d\r\n",
                output->data[i].time_stamp, output->data[i].accelerometer_t.x,
                output->data[i].accelerometer_t.y, output->data[i].accelerometer_t.z,
                output->data[i].accelerometer_t.x_bias, output->data[i].accelerometer_t.y_bias,
                output->data[i].accelerometer_t.z_bias);
    }

    return res;
}


unsigned int icm20645_gse_mode(void)
{
    return sensor_power;
}

int mpu_i2c_read_block_fifo(unsigned char *data, unsigned char len)
{
    int err = 0;
//#ifdef I2C_CONFIG
    struct icm20645g_data *obj = &obj_data;
    struct mt_i2c_t i2c;
    i2c.id    = obj->hw->i2c_num;
    i2c.addr  = obj->i2c_addr >> 1; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;
    i2c.st_rs = I2C_TRANS_REPEATED_START;
    err = i2c_write_read(&i2c, data, 1, len);
    if (0 != err) {
        GSE_ERR("i2c_read fail: %d\n\r", err);
    }
//#endif
    return err;


}


int mpu_i2c_read_block(unsigned char *data, unsigned char len)
{
    int err = 0;
//#ifdef I2C_CONFIG
    struct icm20645g_data *obj = &obj_data;
    struct mt_i2c_t i2c;
    i2c.id    = obj->hw->i2c_num;
    i2c.addr  = obj->i2c_addr >> 1; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;
    //i2c.st_rs = I2C_TRANS_REPEATED_START;
    err = i2c_write_read(&i2c, data, 1, len);
    if (0 != err) {
        GSE_ERR("i2c_read fail: %d\n\r", err);
    }
//#endif
    return err;


}
int mpu_i2c_write_block(unsigned char *data, unsigned char len)
{
    int err = 0;
//#ifdef I2C_CONFIG
    struct icm20645g_data *obj = &obj_data;
    struct mt_i2c_t i2c;
    i2c.id    = obj->hw->i2c_num;
    i2c.addr  = obj->hw->i2c_addr[0] >> 1; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;
    err = i2c_write(&i2c, data, len);
    if (0 != err) {
        GSE_ERR("i2c_write fail: %d\n\r", err);
    }
//#endif

    return err;
}
static int icm20645g_set_bank(unsigned char bank)
{
    int res = 0;
    unsigned char databuf[2];
    databuf[0] = REG_BANK_SEL;
    databuf[1] = bank;
    res = mpu_i2c_write_block(databuf, 2);
    if (res < 0) {
        GSE_LOG("icm20645g_set_bank fail at %x\n\r", bank);
        return ICM20645_ERR_I2C;
    }

    return ICM20645_SUCCESS;
}
static int icm20645g_dev_reset(void)
{
    unsigned char databuf[2];
    int res = 0;
    GSE_FUN(f);
    icm20645g_set_bank(BANK_SEL_0);

    /* read */
    databuf[0] = ICM20645_PWR_MGMT_1;
    res = mpu_i2c_read_block(databuf, 0x1);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }

    /* write */
    databuf[0] = databuf[0] | ICM20645_DEV_RESET;
    databuf[1] = databuf[0];
    databuf[0] = ICM20645_PWR_MGMT_1;
    res = mpu_i2c_write_block(databuf, 2);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }

    do {
        databuf[0] = ICM20645_PWR_MGMT_1;
        res = mpu_i2c_read_block(databuf, 0x1);
        if (res < 0) {
            return ICM20645_ERR_I2C;
        }
    } while ((databuf[0] & ICM20645_DEV_RESET) != 0);
    GSE_LOG("check reset bit success\n\r");
    return ICM20645_SUCCESS;
}
static int icm20645g_turn_on(unsigned char status, unsigned int on)
{
    int res = 0;
    unsigned char databuf[2];
    memset(databuf, 0, sizeof(databuf));
    GSE_FUN(f);
    icm20645g_set_bank(BANK_SEL_0);
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

static int icm20645g_lp_mode(unsigned int on)
{
    int res = 0;
    unsigned char databuf[2];
    memset(databuf, 0, sizeof(databuf));
    icm20645g_set_bank(BANK_SEL_0);
    GSE_FUN(f);
    /* acc lp config */
    databuf[0] = ICM20645_REG_LP_CONFIG;
    res = mpu_i2c_read_block(databuf, 0x1);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    if (on == true) {
        databuf[0] |= BIT_ACC_LP_EN;
        databuf[1] = databuf[0];
        databuf[0] = ICM20645_REG_LP_CONFIG;
        res = mpu_i2c_write_block(databuf, 2);
        if (res < 0) {
            return ICM20645_ERR_I2C;
        }
    } else {
        databuf[0] &= ~BIT_ACC_LP_EN;
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
static int icm20645g_lowest_power_mode(unsigned int on)
{
    int res = 0;
    unsigned char databuf[2];
    /* all chip lowest power config include accel and gyro*/

    icm20645g_set_bank(BANK_SEL_0);
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
static int icm20645g_SetSampleRate(int sample_rate)
{
    unsigned char databuf[2];
    unsigned short rate_div = 0;
    int res = 0;
    GSE_FUN(f);
    rate_div = 1125 / sample_rate - 1;

    icm20645g_set_bank(BANK_SEL_2);

    databuf[0] = ICM20645_REG_SAMRT_DIV2;
    databuf[1] = (unsigned char)(rate_div % 256);
    res = mpu_i2c_write_block(databuf, 2);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    databuf[0] = ICM20645_REG_SAMRT_DIV1;
    databuf[1] = (unsigned char)(rate_div / 256);
    res = mpu_i2c_write_block(databuf, 2);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }

    icm20645g_set_bank(BANK_SEL_0);
    return ICM20645_SUCCESS;
}
static int icm20645g_Setfilter(int filter_sample)
{
    unsigned char databuf[2];
    int res = 0;
    GSE_FUN(f);

    icm20645g_set_bank(BANK_SEL_2);
    databuf[0] = ICM20645_ACC_CONFIG_2;
    databuf[1] = filter_sample;
    res = mpu_i2c_write_block(databuf, 2);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    icm20645g_set_bank(BANK_SEL_0);
    return ICM20645_SUCCESS;
}

static int icm20645_SelClk(void)
{
    unsigned char databuf[2];
    int res = 0;
    icm20645g_set_bank(BANK_SEL_0);
    databuf[0] = ICM20645_PWR_MGMT_1;
    res = mpu_i2c_read_block(databuf, 1);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    databuf[0] |= (BIT_CLK_PLL | BIT_TEMP_DIS);
    databuf[1] = databuf[0];
    databuf[0] = ICM20645_PWR_MGMT_1;

    res = mpu_i2c_write_block(databuf, 2);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    return ICM20645_SUCCESS;

}
static int icm20645g_SetPowerMode(unsigned int enable)
{
    unsigned char databuf[2];
    int res = 0;
    GSE_FUN(f);

    icm20645g_set_bank(BANK_SEL_0);

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
        if (icm20645_gyro_mode() == false) {
            databuf[0] |= ICM20645_SLEEP;
        } else {
            res = icm20645g_lowest_power_mode(true);
            if (res != ICM20645_SUCCESS) {
                GSE_ERR("icm20645gy_lowest_power_mode error\n\r");
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
    return ICM20645_SUCCESS;
}
static int icm20645g_CheckDeviceID(void)
{
    unsigned char databuf[2];
    int res = 0;
    GSE_FUN(f);

    icm20645g_set_bank(BANK_SEL_0);
    databuf[0] = ICM20645_REG_DEVID;
    res = mpu_i2c_read_block(databuf, 0x1);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    GSE_LOG("icm20645g_CheckDeviceID 0x%x\n\r", databuf[0]);
    return ICM20645_SUCCESS;
}
static int icm20645g_SetDataResolution(void)
{
    unsigned char databuf[2];
    int err = 0;
    unsigned char  reso = 0;
    struct icm20645g_data *obj = &obj_data;
    GSE_FUN(f);
    databuf[0] = ICM20645_ACC_CONFIG;
    if ((err = mpu_i2c_read_block(databuf, 1))) {
        return ICM20645_ERR_I2C;
    }

    reso = 0x00;
    reso = (databuf[0] & ICM20645_RANGE_16G) >> 1;

    if (reso < sizeof(icm20645g_data_resolution) / sizeof(icm20645g_data_resolution[0])) {
        obj->reso = &icm20645g_data_resolution[reso];
        GSE_ERR("icm20645g_data_resolution reso: %d, sensitivity: %d\n\r", reso, obj->reso->sensitivity);
        return 0;
    } else {
        return -1;
    }
}

static int icm20645g_SetDataFormat(unsigned char dataformat)
{
    unsigned char databuf[2];
    int res = 0;
    GSE_FUN(f);

    icm20645g_set_bank(BANK_SEL_2);

    /* write */
    databuf[0] = (0 | dataformat);
    databuf[1] = databuf[0];
    databuf[0] = ICM20645_ACC_CONFIG;
    res = mpu_i2c_write_block(databuf, 2);

    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    res = icm20645g_SetDataResolution();
    return res;
}
static int icm20645g_SetIntEnable(unsigned char intenable)
{
    unsigned char databuf[2];
    int res = 0;
    GSE_FUN(f);
    databuf[0] = intenable;
    databuf[1] = databuf[0];
    databuf[0] = ICM20645_REG_INT_ENABLE;
    res = mpu_i2c_write_block(databuf, 2);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }
    return ICM20645_SUCCESS;
}
static int icm20645g_ResetCalibration()
{
    struct icm20645g_data *obj = &obj_data;

    GSE_FUN(f);

    memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
    memset(obj->offset, 0x00, sizeof(obj->offset));

    return 0;
}

static int icm20645g_ReadCalibrationEx(int *act, int *raw)
{
    /*raw: the raw calibration data; act: the actual calibration data*/
    struct icm20645g_data *obj = &obj_data;
    int mul = 0;

    GSE_FUN(f);

    mul = 0;

    raw[ACCEL_AXIS_X] = obj->offset[ACCEL_AXIS_X] * mul + obj->cali_sw[ACCEL_AXIS_X];
    raw[ACCEL_AXIS_Y] = obj->offset[ACCEL_AXIS_Y] * mul + obj->cali_sw[ACCEL_AXIS_Y];
    raw[ACCEL_AXIS_Z] = obj->offset[ACCEL_AXIS_Z] * mul + obj->cali_sw[ACCEL_AXIS_Z];

    act[obj->cvt.map[ACCEL_AXIS_X]] = obj->cvt.sign[ACCEL_AXIS_X] * raw[ACCEL_AXIS_X];
    act[obj->cvt.map[ACCEL_AXIS_Y]] = obj->cvt.sign[ACCEL_AXIS_Y] * raw[ACCEL_AXIS_Y];
    act[obj->cvt.map[ACCEL_AXIS_Z]] = obj->cvt.sign[ACCEL_AXIS_Z] * raw[ACCEL_AXIS_Z];

    return 0;
}
static int icm20645g_WriteCalibration(int *dat)
{
    struct icm20645g_data *obj = &obj_data;
    int err = 0;
    int cali[ACCEL_AXES_NUM], raw[ACCEL_AXES_NUM];

    GSE_FUN(f);
    dat[ACCEL_AXIS_X] = dat[ACCEL_AXIS_X] * obj->reso->sensitivity / GRAVITY_EARTH_1000;
    dat[ACCEL_AXIS_Y] = dat[ACCEL_AXIS_Y] * obj->reso->sensitivity / GRAVITY_EARTH_1000;
    dat[ACCEL_AXIS_Z] = dat[ACCEL_AXIS_Z] * obj->reso->sensitivity / GRAVITY_EARTH_1000;
    if (0 != (err = icm20645g_ReadCalibrationEx(cali, raw))) {
        GSE_ERR("read offset fail, %d\n\r", err);
        return err;
    }

    GSE_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n\r",
            raw[ACCEL_AXIS_X], raw[ACCEL_AXIS_Y], raw[ACCEL_AXIS_Z],
            obj->offset[ACCEL_AXIS_X], obj->offset[ACCEL_AXIS_Y], obj->offset[ACCEL_AXIS_Z],
            obj->cali_sw[ACCEL_AXIS_X], obj->cali_sw[ACCEL_AXIS_Y], obj->cali_sw[ACCEL_AXIS_Z]);

    /*calculate the real offset expected by caller*/
    cali[ACCEL_AXIS_X] += dat[ACCEL_AXIS_X];
    cali[ACCEL_AXIS_Y] += dat[ACCEL_AXIS_Y];
    cali[ACCEL_AXIS_Z] += dat[ACCEL_AXIS_Z];

    GSE_LOG("UPDATE: (%+3d %+3d %+3d)\n\r",
            dat[ACCEL_AXIS_X], dat[ACCEL_AXIS_Y], dat[ACCEL_AXIS_Z]);

    obj->cali_sw[ACCEL_AXIS_X] = obj->cvt.sign[ACCEL_AXIS_X] * (cali[obj->cvt.map[ACCEL_AXIS_X]]);
    obj->cali_sw[ACCEL_AXIS_Y] = obj->cvt.sign[ACCEL_AXIS_Y] * (cali[obj->cvt.map[ACCEL_AXIS_Y]]);
    obj->cali_sw[ACCEL_AXIS_Z] = obj->cvt.sign[ACCEL_AXIS_Z] * (cali[obj->cvt.map[ACCEL_AXIS_Z]]);

    GSE_LOG("cali_sw: (%+3d %+3d %+3d)\n\r",
            obj->cali_sw[ACCEL_AXIS_X], obj->cali_sw[ACCEL_AXIS_Y], obj->cali_sw[ACCEL_AXIS_Z]);

    return err;
}
static int icm20645g_ReadData(int *data)
{
    unsigned char buf[ICM20645_DATA_LEN] = {0};
    struct icm20645g_data *obj = &obj_data;
    int err = 0;

    GSE_FUN(f);
    buf[0] = ICM20645_REG_DATAX0;
    if ((err = mpu_i2c_read_block(buf, ICM20645_DATA_LEN)) != 0) {
        GSE_ERR("error: %d\n\r", err);
        return ICM20645_ERR_I2C;
    }
    data[ACCEL_AXIS_X] = (short)((buf[ACCEL_AXIS_X * 2] << 8) | (buf[ACCEL_AXIS_X * 2 + 1]));
    data[ACCEL_AXIS_Y] = (short)((buf[ACCEL_AXIS_Y * 2] << 8) | (buf[ACCEL_AXIS_Y * 2 + 1]));
    data[ACCEL_AXIS_Z] = (short)((buf[ACCEL_AXIS_Z * 2] << 8) | (buf[ACCEL_AXIS_Z * 2 + 1]));

    if (ICM20645_TRC_RAWDATA & obj->trace)
        GSE_ERR("raw data: [%08X %08X %08X] => [%5d %5d %5d]\n\r", data[ACCEL_AXIS_X], data[ACCEL_AXIS_Y], data[ACCEL_AXIS_Z],
                data[ACCEL_AXIS_X], data[ACCEL_AXIS_Y], data[ACCEL_AXIS_Z]);
    return err;
}

static int icm20645g_ReadSensorData(int *data)
{
    struct icm20645g_data *obj = &obj_data;
    int remap_data[ACCEL_AXES_NUM];
    int res = 0;

    GSE_FUN(f);

    if (0 != (res = icm20645g_ReadData(data))) {
        GSE_ERR("I2C error: ret value=%d\n\r", res);
        return ICM20645_ERR_I2C;
    }
    data[ACCEL_AXIS_X] += obj->cali_sw[ACCEL_AXIS_X];
    data[ACCEL_AXIS_Y] += obj->cali_sw[ACCEL_AXIS_Y];
    data[ACCEL_AXIS_Z] += obj->cali_sw[ACCEL_AXIS_Z];

    if (ICM20645_TRC_CALI & obj->trace)
        GSE_ERR("cali data: %d, %d, %d!\n\r", data[ACCEL_AXIS_X], data[ACCEL_AXIS_Y], data[ACCEL_AXIS_Z]);

    /*remap coordinate*/
    remap_data[obj->cvt.map[ACCEL_AXIS_X]] = obj->cvt.sign[ACCEL_AXIS_X] * data[ACCEL_AXIS_X];
    remap_data[obj->cvt.map[ACCEL_AXIS_Y]] = obj->cvt.sign[ACCEL_AXIS_Y] * data[ACCEL_AXIS_Y];
    remap_data[obj->cvt.map[ACCEL_AXIS_Z]] = obj->cvt.sign[ACCEL_AXIS_Z] * data[ACCEL_AXIS_Z];

    //Out put the mg
    data[ACCEL_AXIS_X] = remap_data[ACCEL_AXIS_X] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
    data[ACCEL_AXIS_Y] = remap_data[ACCEL_AXIS_Y] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;
    data[ACCEL_AXIS_Z] = remap_data[ACCEL_AXIS_Z] * GRAVITY_EARTH_1000 / obj->reso->sensitivity;

    if (ICM20645_TRC_RAWDATA & obj->trace)
        GSE_ERR("gsensor data: %d, %d, %d!\n\r", data[ACCEL_AXIS_X], data[ACCEL_AXIS_Y], data[ACCEL_AXIS_Z]);

    return 0;
}
static int icm20645g_dump_register(void)
{
    int ret = 0;

    return ret;
}

static int cust_set(void *data, int len)
{
    int err = 0;
    CUST_SET_REQ_P req = (CUST_SET_REQ_P)data;
    struct icm20645g_data *obj = &obj_data;


    GSE_FUN(f);
    GSE_LOG("CUST ACTION :%d\n\r", req->cust.action);
    switch (req->cust.action) {
        case CUST_ACTION_SET_CUST:
            GSE_LOG("CUST_ACTION_SET_CUST\n\r");
            break;
        case CUST_ACTION_SET_CALI:
            GSE_LOG("CUST_ACTION_SET_CALI\n\r");
            err = icm20645g_WriteCalibration(req->setCali.int32_data);
            GSE_LOG("%d, %d, %d\n\r", obj_data.cali_sw[0], obj_data.cali_sw[1], obj_data.cali_sw[2]);
            break;
        case CUST_ACTION_RESET_CALI:
            GSE_LOG("CUST_ACTION_RESET_CALI\n\r");
            err = icm20645g_ResetCalibration();
            GSE_LOG("%d, %d, %d\n\r", obj_data.cali_sw[0], obj_data.cali_sw[1], obj_data.cali_sw[2]);
            break;
        case CUST_ACTION_SET_DIRECTION:
            GSE_LOG("CUST_ACTION_SET_ORIENTATION: %d\n\r", req->setDirection.direction);
            obj->hw->direction = (req->setDirection.direction);
            GSE_LOG("set orientation: %d\n\r", obj->hw->direction);
            if (0 != (err = sensor_driver_get_convert(obj->hw->direction, &obj->cvt))) {
                GSE_ERR("invalid direction: %d\n\r", obj->hw->direction);
            }
            break;
        case CUST_ACTION_SHOW_REG:
            GSE_LOG("CUST_ACTION_SHOW_REG\n\r");
            icm20645g_dump_register();
            break;
        case CUST_ACTION_SET_TRACE:
            GSE_LOG("CUST_ACTION_SET_TRACE;: %d\n\r", req->setTrace.trace);
            obj->trace = (req->setTrace.trace);
            break;
        case CUST_ACTION_SET_FACTORY:
            use_in_factory = (req->setFactory.factory);
            break;
        default:
            GSE_LOG("default\n\r");
            err = -1;
            break;
    }

    return err;
}
static int icm20645g_ReadTimeBase(mtk_int8 *time_percent)
{
    int res = 0;
    unsigned char databuf[2];

    icm20645g_set_bank(BANK_SEL_1);
    databuf[0] = G_ICM20645_TIMEBASE_PLL;
    res = mpu_i2c_read_block(databuf, 0x1);
    if (res < 0) {
        return ICM20645_ERR_I2C;
    }

    *time_percent = databuf[0];
    return ICM20645_SUCCESS;
}

int icm20645g_init_client(void)
{
    int res = 0;
    struct icm20645g_data *obj = &obj_data;

    res = icm20645g_CheckDeviceID();
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_CheckDeviceID error\n\r");
        return res;
    }
    res = icm20645_SelClk();
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_selclk error\n\r");
        return res;
    }
    res = icm20645g_SetPowerMode(false);
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_SetPowerMode error\n\r");
        return res;
    }
    res = icm20645g_SetPowerMode(true);
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_SetPowerMode error\n\r");
        return res;
    }
    res = icm20645g_turn_on(BIT_PWR_ACCEL_STBY, false);
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_turn_on error\n\r");
        return res;
    }
    res = icm20645g_SetIntEnable(0x00);
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_SetIntEnable error\n\r");
        return res;
    }
    res = icm20645g_lp_mode(true);
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_lp_mode error\n\r");
        return res;
    }
    res = icm20645g_SetDataFormat(ICM20645_RANGE_4G | ACCEL_DLPFCFG | ACCEL_FCHOICE);
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_SetDataFormat error\n\r");
        return res;
    }
    res = icm20645g_Setfilter(ACCEL_AVGCFG_8X);
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_Setfilter ERR!\n\r");
        return res;
    }
    res = icm20645g_SetSampleRate(125);
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_SetSampleRate error\n\r");
        return res;
    }
    res = icm20645g_ReadTimeBase(&obj->g_fifo_param.TB_PERCENT);
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_ReadTimeBase error\n\r");
        return res;
    }

    res = icm20645g_SetPowerMode(false);
    if (res != ICM20645_SUCCESS) {
        GSE_ERR("icm20645g_SetPowerMode error\n\r");
        return res;
    }
    return res;
}
int icm20645_gyro_fifo_status(void)
{
    int err = 0;
    unsigned char databuf[2];

    err = icm20645g_SetPowerMode(true);
    databuf[0] = G_ICM20645_REG_FIFO_EN_2;
    err = mpu_i2c_read_block(databuf, 0x1);
    if (err < 0) {
        GSE_LOG("icm20645gy_enable_fifo fail at3 %x\n\r", err);
        return ICM20645_ERR_I2C;
    }
    if ((databuf[0] & (7 << 1)) != 0) //(//(0b00001110))
        return true;
    else
        return false;
}
int icm20645g_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;
    //unsigned int hw_rate;
    struct icm20645g_data *obj = &obj_data;
    unsigned char databuf[2];

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            //GSE_ERR("icm20645gg_operation command ACTIVATE %d\n\r",*(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                GSE_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (SENSOR_DISABLE == value) {
                    icm20645g_turn_on(BIT_PWR_ACCEL_STBY, false);
                    err = icm20645g_lowest_power_mode(false);
                    if (err != ICM20645_SUCCESS) {
                        GSE_ERR("icm20645gy_lowest_power_mode error\n\r");
                        err = -1;
                    }
                    err = icm20645g_SetPowerMode(false);
                    if (err < 0) {
                        GSE_ERR("icm20645g_SetPowerMode off fail\n\r");
                        err = -1;
                    }
                } else {
                    err = icm20645g_SetPowerMode(true);
                    if (err < 0) {
                        GSE_ERR("icm20645g_SetPowerMode  on fail\n\r");
                        err = -1;
                    }
                    icm20645g_turn_on(BIT_PWR_ACCEL_STBY, true);
                    err = icm20645g_lowest_power_mode(true);
                    if (err != ICM20645_SUCCESS) {
                        GSE_ERR("icm20645gy_lowest_power_mode error\n\r");
                        err = -1;
                    }
                }
            }
            break;
        case SETDELAY:
            //GSE_ERR("icm20645gg_operation command SETDELAY %d\n\r",*(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                GSE_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;

                if (value == ACC_DELAY_PER_FIFO_LOOP) {
                    /* enable HW fifo */
                    //obj->g_fifo_param.fifo_enable = FIFO_ENABLE;
                    obj->g_fifo_param.hw_rate = 1000 * ACC_EVENT_COUNT_PER_FIFO_LOOP / value;
                } else {
                    /* disable HW fifo */
                    //obj->g_fifo_param.fifo_enable = FIFO_DISABLE;
                    obj->g_fifo_param.hw_rate = 1000 / value;
                }

                err = icm20645g_SetPowerMode(true);
                if (err < 0) {
                    GSE_ERR("icm20645g_SetPowerMode  on fail\n\r");
                    err = -1;
                }
                err = icm20645g_lowest_power_mode(false);
                if (err != ICM20645_SUCCESS) {
                    GSE_ERR("icm20645gy_lowest_power_mode error\n\r");
                    err = -1;
                }
                err = icm20645g_SetSampleRate(obj->g_fifo_param.hw_rate);
                if (err != ICM20645_SUCCESS) {
                    GSE_ERR("icm20645gy_SetSampleRate error\n\r");
                    err = -1;
                }
                err = icm20645g_lowest_power_mode(true);
                if (err != ICM20645_SUCCESS) {
                    GSE_ERR("icm20645gy_lowest_power_mode error\n\r");
                    err = -1;
                }
            }
            break;
        case SETCUST:
            GSE_LOG("icm20645g_operation command SETCUST\n\r");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                GSE_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                err = cust_set(buffer_in, size_in);
                if (err < 0) {
                    GSE_ERR("Set customization error : %d\n\r", err);
                }
            }
            break;
        case ENABLEFIFO:
            GSE_ERR("icm20645g_operation command ENABLEFIFO:%d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                GSE_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (value == FIFO_ENABLE) {
                    if (obj->g_fifo_param.fifo_enable != FIFO_ENABLE) {
                        //GSE_ERR("icm20645g_operation real FIFO_ENABLE\n\r");
                        obj->g_fifo_param.fifo_enable = FIFO_ENABLE;
                        err = icm20645g_SetPowerMode(true);
                        err = icm20645g_lowest_power_mode(false);
                        err = icm20645g_enable_fifo(FIFO_ENABLE);
                        if (err < 0)
                            GSE_LOG("icm20645g_enable_fifo enable fail\n\r");
                        err = icm20645g_lowest_power_mode(true);
                        obj->g_fifo_param.fifo_timestamp = read_xgpt_stamp_ns();
                        obj->g_fifo_param.fifo_timestamp +=
                            (UINT64)(G_TIMEBASE_PLL_CORRECT(ONE_EVENT_DELAY_PER_FIFO_LOOP, obj->g_fifo_param.TB_PERCENT));
                        // 1000000000 /
                    }
                } else {
                    if (obj->g_fifo_param.fifo_enable != FIFO_DISABLE) {
                        //GSE_ERR("icm20645g_operation real FIFO_DISABLE\n\r");
                        obj->g_fifo_param.fifo_enable = FIFO_DISABLE;
                        if (!icm20645_gyro_fifo_status()) {
                            //GSE_ERR("icm20645g_operation gyro fifo is disabled \n\r");
                            err = icm20645g_SetPowerMode(true);
                            err = icm20645g_lowest_power_mode(false);
                            err = icm20645g_enable_fifo(FIFO_DISABLE);
                            if (err < 0)
                                GSE_LOG("icm20645g_enable_fifo enable fail\n\r");
                            err = icm20645g_lowest_power_mode(true);
                        } else {
                            //GSE_ERR("icm20645g_operation gyro fifo is enabled \n\r");
                            err = icm20645g_SetPowerMode(true);
                            err = icm20645g_lowest_power_mode(false);
                            databuf[0] = G_ICM20645_REG_FIFO_EN_2;
                            err = mpu_i2c_read_block(databuf, 0x1);
                            if (err < 0) {
                                GSE_LOG("icm20645gy_enable_fifo fail at3 %x\n\r", err);
                                return ICM20645_ERR_I2C;
                            }
                            databuf[0] = databuf[0] & ~(0b00010000);
                            databuf[1] = databuf[0];
                            databuf[0] = G_ICM20645_REG_FIFO_EN_2;
                            err = mpu_i2c_write_block(databuf, 0x2);
                            if (err < 0) {
                                GSE_LOG("icm20645gy_enable_fifo fail at4 %x\n\r", err);
                                return ICM20645_ERR_I2C;
                            }
                            err = icm20645g_lowest_power_mode(true);
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

int icm20645g_run_algorithm(struct data_t *output)
{
    int ret = 0;
    int buff[ACCEL_AXES_NUM] = {0};
    int status;
    int count = 0;
    int dt;
    static UINT64 last_time_stamp;
    MPE_SENSOR_DATA raw_data_input;
    MPE_SENSOR_DATA calibrated_data_output;
    struct icm20645g_data *obj = &obj_data;
    switch (obj->g_fifo_param.fifo_enable) {
        case FIFO_ENABLE:
            count = icm20645g_fifo_count();
            if (count != 0) {
                ret = icm20645g_read_fifo(count, output);
            }
            break;
        case FIFO_DISABLE:
            ret = icm20645g_ReadSensorData(buff);
            if (ret < 0) {
                return ret;
            }
            switch (obj->use_in_factory) {
                case USE_IN_FACTORY:
                    output->data->sensor_type = SENSOR_TYPE_ACCELEROMETER;
                    output->data_exist_count = 1;
                    output->data->time_stamp = read_xgpt_stamp_ns();
                    output->data->accelerometer_t.x = buff[ACCEL_AXIS_X];
                    output->data->accelerometer_t.y = buff[ACCEL_AXIS_Y];
                    output->data->accelerometer_t.z = buff[ACCEL_AXIS_Z];
                    output->data->accelerometer_t.x_bias = 0;
                    output->data->accelerometer_t.y_bias = 0;
                    output->data->accelerometer_t.z_bias = 0;
                    GSE_LOG("x:%d, y:%d, z:%d, x_bias:%d, y_bias:%d, z_bias:%d\r\n", output->data->accelerometer_t.x,
                            output->data->accelerometer_t.y, output->data->accelerometer_t.z,
                            output->data->accelerometer_t.x_bias, output->data->accelerometer_t.y_bias,
                            output->data->accelerometer_t.z_bias);
                    break;
                case NOT_USE_IN_FACTORY:
                    output->data->sensor_type = SENSOR_TYPE_ACCELEROMETER;
                    output->data_exist_count = 1;
                    output->data->time_stamp = read_xgpt_stamp_ns();
                    raw_data_input.x = buff[ACCEL_AXIS_X];
                    raw_data_input.y = buff[ACCEL_AXIS_Y];
                    raw_data_input.z = buff[ACCEL_AXIS_Z];
                    dt = (int)(output->data->time_stamp - last_time_stamp);
                    //GSE_LOG("dt:%d, x:%d, y:%d, z:%d\r\n", dt, raw_data_input.x, raw_data_input.y, raw_data_input.z);
                    ret = Acc_run_calibration(dt, output->data_exist_count, &raw_data_input, &calibrated_data_output, &status);
                    if (ret < 0) {
                        return ret;
                    }
                    output->data->accelerometer_t.x = calibrated_data_output.x;
                    output->data->accelerometer_t.y = calibrated_data_output.y;
                    output->data->accelerometer_t.z = calibrated_data_output.z;
                    output->data->accelerometer_t.x_bias = (INT16)(calibrated_data_output.x - raw_data_input.x);
                    output->data->accelerometer_t.y_bias = (INT16)(calibrated_data_output.y - raw_data_input.y);
                    output->data->accelerometer_t.z_bias = (INT16)(calibrated_data_output.z - raw_data_input.z);
                    output->data->accelerometer_t.status = (INT16)status;
                    GSE_LOG("x:%d, y:%d, z:%d, x_bias:%d, y_bias:%d, z_bias:%d\r\n", output->data->accelerometer_t.x,
                            output->data->accelerometer_t.y, output->data->accelerometer_t.z,
                            output->data->accelerometer_t.x_bias, output->data->accelerometer_t.y_bias,
                            output->data->accelerometer_t.z_bias);
                    last_time_stamp = output->data->time_stamp;
                    break;
            }
            break;
    }
    return ret;
}

int icm20645g_sensor_init(void)
{
    int ret = 0, i = 0;
    GSE_FUN(f);
    struct SensorDescriptor_t  icm20645g_descriptor_t;
    struct icm20645g_data *obj = &obj_data;
    MPE_SENSOR_DATA bias;


    memset(obj, 0, sizeof(struct icm20645g_data));
    obj->use_in_factory = NOT_USE_IN_FACTORY;
    obj->g_fifo_param.fifo_enable = FIFO_DISABLE;
    bias.x = 0;
    bias.y = 0;
    bias.z = 0;
    icm20645g_descriptor_t.sensor_type = SENSOR_TYPE_ACCELEROMETER;
    icm20645g_descriptor_t.version =  1;
    icm20645g_descriptor_t.report_mode = continus;
    icm20645g_descriptor_t.hw.max_sampling_rate = 5;
    icm20645g_descriptor_t.hw.support_HW_FIFO = 0;

    icm20645g_descriptor_t.input_list = NULL;

    icm20645g_descriptor_t.operate = icm20645g_operation;
    icm20645g_descriptor_t.run_algorithm = icm20645g_run_algorithm;
    icm20645g_descriptor_t.set_data = NULL;

    obj->hw = get_cust_acc_hw();
    if (NULL == obj->hw) {
        GSE_ERR("get_cust_acc_hw fail\n\r");
        return ret;
    }
    obj->i2c_addr = obj->hw->i2c_addr[0];
    GSE_LOG("i2c_num: %d, i2c_addr: 0x%x, direction: %d\n\r", obj->hw->i2c_num, obj->i2c_addr, obj->hw->direction);

    if (0 != (ret = sensor_driver_get_convert(obj->hw->direction, &obj->cvt))) {
        GSE_ERR("invalid direction: %d\n\r", obj->hw->direction);
    }
    GSE_LOG("map[0]:%d, map[1]:%d, map[2]:%d, sign[0]:%d, sign[1]:%d, sign[2]:%d\n\r",
            obj->cvt.map[ACCEL_AXIS_X], obj->cvt.map[ACCEL_AXIS_Y], obj->cvt.map[ACCEL_AXIS_Z],
            obj->cvt.sign[ACCEL_AXIS_X], obj->cvt.sign[ACCEL_AXIS_Y], obj->cvt.sign[ACCEL_AXIS_Z]);
    for (i = 0; i < 3; ++i) {
        ret = icm20645g_dev_reset();
        if (ret < 0) {
            GSE_ERR("icm20645g_dev_reset fail\n\r");
            continue;
        }
        break;
    }
    ret = icm20645g_init_client();
    if (ret < 0) {
        GSE_ERR("icm20645g_init_client fail\n\r");
        return ret;
    }
    ret = icm20645g_ResetCalibration();
    if (ret < 0) {
        GSE_ERR("icm20645g_ResetCalibration fail\n\r");
        return ret;
    }
    Acc_init_calibration(bias);
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_ACCELEROMETER, MAX_ACCELERATION_FIFO_SIZE);
    if (ret) {
        GSE_ERR("RegisterDataBuffer fail\n\r");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_type(&icm20645g_descriptor_t);
    if (ret) {
        GSE_ERR("RegisterAlgorithm fail\n\r");
        return ret;
    }
    return ret;
}
MODULE_DECLARE(icm20645g, MOD_PHY_SENSOR, icm20645g_sensor_init);
