#include <bmp280.h>
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
#include "cust_baro.h"

//#define BMP280_LOG_MSG
#define BARO_TAG                "[BARO] "
#define BARO_ERR(fmt, arg...)   PRINTF_D(BARO_TAG"%d: "fmt, __LINE__, ##arg)

#ifdef BMP280_LOG_MSG
#define BARO_LOG(fmt, arg...)   PRINTF_D(BARO_TAG fmt, ##arg)
#define BARO_FUN(f)             PRINTF_D("%s\n", __FUNCTION__)
#else
#define BARO_LOG(fmt, arg...)
#define BARO_FUN(f)
#endif
static struct bmp280_data obj_data;

int bmp280_i2c_read_block(unsigned char *data, unsigned char len)
{
    int err = 0;
    struct bmp280_data *obj = &obj_data;

    struct mt_i2c_t i2c;
    i2c.id    = obj->hw->i2c_num;
    i2c.addr  = obj->i2c_addr; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;

    err = i2c_write_read(&i2c, data, 1, len);
    if (0 != err) {
        BARO_ERR("i2c_read fail: %d\n", err);
    }

    return err;

}
int bmp280_i2c_write_block(unsigned char *data, unsigned char len)
{
    int err = 0;
    struct bmp280_data *obj = &obj_data;

    struct mt_i2c_t i2c;
    i2c.id    = obj->hw->i2c_num;
    i2c.addr  = obj->i2c_addr; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;

    err = i2c_write(&i2c, data, len);
    if (0 != err) {
        BARO_ERR("i2c_write fail: %d\n", err);
    }
    return err;
}

/* get chip type */
static int bmp280_get_chip_type(void)
{
    int err = 0;
    unsigned char buf[2];
    struct bmp280_data *obj = &obj_data;
    BARO_FUN(f);
    buf[0] = BMP_CHIP_ID_REG;
    err = bmp280_i2c_read_block(buf, 0x01);
    if (err != 0)
        return BMP280_ERR_I2C;

    switch (buf[0]) {
        case BMP280_CHIP_ID1:
        case BMP280_CHIP_ID2:
        case BMP280_CHIP_ID3:
            obj->sensor_type = BMP280_TYPE;
            break;
        default:
            obj->sensor_type = INVALID_TYPE;
            break;
    }

    BARO_LOG("chip id = %#x\n", buf[0]);

    if (obj->sensor_type == INVALID_TYPE) {
        BARO_ERR("unknown pressure sensor\n");
        return -1;
    }
    return BMP280_SUCCESS;
}

static int bmp280_get_calibration_data(void)
{

    int status = 0;
    struct bmp280_data *obj = &obj_data;
    if (obj->sensor_type == BMP280_TYPE) {
        unsigned char a_data_u8r[8];
        a_data_u8r[0] = BMP280_CALIBRATION_DATA_START;
        status = bmp280_i2c_read_block(a_data_u8r, 8);
        if (status < 0)
            return BMP280_ERR_I2C;
        obj->bmp280_cali.dig_T1 = (unsigned short)(((\
                                  (unsigned short)((unsigned char)a_data_u8r[1])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[0]);
        obj->bmp280_cali.dig_T2 = (short)(((\
                                            (short)((signed char)a_data_u8r[3])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[2]);
        obj->bmp280_cali.dig_T3 = (short)(((\
                                            (short)((signed char)a_data_u8r[5])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[4]);
        obj->bmp280_cali.dig_P1 = (unsigned short)(((\
                                  (unsigned short)((unsigned char)a_data_u8r[7])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[6]);
        a_data_u8r[0] = BMP280_CALIBRATION_DATA_START+8;
        status = bmp280_i2c_read_block(a_data_u8r, 8);
        if (status < 0)
            return BMP280_ERR_I2C;
        obj->bmp280_cali.dig_P2 = (short)(((\
                                            (short)((signed char)a_data_u8r[1])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[0]);
        obj->bmp280_cali.dig_P3 = (short)(((\
                                            (short)((signed char)a_data_u8r[3])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[2]);
        obj->bmp280_cali.dig_P4 = (short)(((\
                                            (short)((signed char)a_data_u8r[5])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[4]);
        obj->bmp280_cali.dig_P5 = (short)(((\
                                            (short)((signed char)a_data_u8r[7])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[6]);
        a_data_u8r[0] = BMP280_CALIBRATION_DATA_START+16;
        status = bmp280_i2c_read_block(a_data_u8r, 8);
        if (status < 0)
            return BMP280_ERR_I2C;
        obj->bmp280_cali.dig_P6 = (short)(((\
                                            (short)((signed char)a_data_u8r[1])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[0]);
        obj->bmp280_cali.dig_P7 = (short)(((\
                                            (short)((signed char)a_data_u8r[3])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[2]);
        obj->bmp280_cali.dig_P8 = (short)(((\
                                            (short)((signed char)a_data_u8r[5])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[4]);
        obj->bmp280_cali.dig_P9 = (short)(((\
                                            (short)((signed char)a_data_u8r[7])) << SHIFT_LEFT_8_POSITION) | a_data_u8r[6]);
    }
    return 0;
}

static int bmp280_set_powermode(enum BMP_POWERMODE_ENUM power_mode)
{
    unsigned char err = 0, actual_power_mode = 0;
    unsigned char buf[2];
    struct bmp280_data *obj = &obj_data;
    BARO_LOG("power_mode = %d, old power_mode = %d\n", power_mode, obj->power_mode);
    if (power_mode == obj->power_mode)
        return 0;
    //mutex_lock(&obj->lock);

    if (obj->sensor_type == BMP280_TYPE) {/* BMP280 */
        if (power_mode == BMP_SUSPEND_MODE) {
            actual_power_mode = BMP280_SLEEP_MODE;
        } else if (power_mode == BMP_NORMAL_MODE) {
            actual_power_mode = BMP280_NORMAL_MODE;
        } else {
            err = -1;
            BARO_ERR("invalid power mode = %d\n", power_mode);
            //mutex_unlock(&obj->lock);
            return err;
        }
        buf[0] = BMP280_CTRLMEAS_REG_MODE__REG;
        err = bmp280_i2c_read_block(buf, 1);
        buf[0] = BMP_SET_BITSLICE(buf[0], BMP280_CTRLMEAS_REG_MODE, actual_power_mode);
        buf[1] = buf[0];
        buf[0] = BMP280_CTRLMEAS_REG_MODE__REG;
        err += bmp280_i2c_write_block(buf, 2);
    }

    if (err < 0)
        BARO_ERR("set power mode failed, err = %d\n", err);
    else
        obj->power_mode = power_mode;

    //mutex_unlock(&obj->lock);
    return err;
}

static int bmp280_set_filter(enum BMP_FILTER_ENUM filter)
{
    unsigned char err = 0, actual_filter = 0;
    struct bmp280_data *obj = &obj_data;
    unsigned char buf[2];
    BARO_LOG("hw filter = %d, old hw filter = %d\n", filter, obj->hw_filter);

    if (filter == obj->hw_filter)
        return 0;

    //mutex_lock(&obj->lock);

    if (obj->sensor_type == BMP280_TYPE) {/* BMP280 */
        if (filter == BMP_FILTER_OFF)
            actual_filter = BMP280_FILTERCOEFF_OFF;
        else if (filter == BMP_FILTER_2)
            actual_filter = BMP280_FILTERCOEFF_2;
        else if (filter == BMP_FILTER_4)
            actual_filter = BMP280_FILTERCOEFF_4;
        else if (filter == BMP_FILTER_8)
            actual_filter = BMP280_FILTERCOEFF_8;
        else if (filter == BMP_FILTER_16)
            actual_filter = BMP280_FILTERCOEFF_16;
        else {
            err = -1;
            BARO_ERR("invalid hw filter = %d\n", filter);
            //mutex_unlock(&obj->lock);
            return err;
        }
        buf[0] = BMP280_CONFIG_REG_FILTER__REG;
        err = bmp280_i2c_read_block(buf, 1);
        buf[0] = BMP_SET_BITSLICE(buf[0], BMP280_CONFIG_REG_FILTER, actual_filter);
        buf[1] = buf[0];
        buf[0] = BMP280_CONFIG_REG_FILTER__REG;
        err += bmp280_i2c_write_block(buf, 2);
    }

    if (err < 0)
        BARO_ERR("set hw filter failed, err = %d\n", err);
    else
        obj->hw_filter = filter;

    //mutex_unlock(&obj->lock);
    return err;
}

static int bmp280_set_oversampling_p(enum BMP_OVERSAMPLING_ENUM oversampling_p)
{
    unsigned char err = 0, actual_oversampling_p = 0;
    struct bmp280_data *obj = &obj_data;
    unsigned char buf[2];
    BARO_LOG("oversampling_p = %d, old oversampling_p = %d\n", oversampling_p, obj->oversampling_p);

    if (oversampling_p == obj->oversampling_p)
        return 0;

    //mutex_lock(&obj->lock);

    if (obj->sensor_type == BMP280_TYPE) {/* BMP280 */
        if (oversampling_p == BMP_OVERSAMPLING_SKIPPED)
            actual_oversampling_p = BMP280_OVERSAMPLING_SKIPPED;
        else if (oversampling_p == BMP_OVERSAMPLING_1X)
            actual_oversampling_p = BMP280_OVERSAMPLING_1X;
        else if (oversampling_p == BMP_OVERSAMPLING_2X)
            actual_oversampling_p = BMP280_OVERSAMPLING_2X;
        else if (oversampling_p == BMP_OVERSAMPLING_4X)
            actual_oversampling_p = BMP280_OVERSAMPLING_4X;
        else if (oversampling_p == BMP_OVERSAMPLING_8X)
            actual_oversampling_p = BMP280_OVERSAMPLING_8X;
        else if (oversampling_p == BMP_OVERSAMPLING_16X)
            actual_oversampling_p = BMP280_OVERSAMPLING_16X;
        else {
            err = -1;
            BARO_ERR("invalid oversampling_p = %d\n", oversampling_p);
            //mutex_unlock(&obj->lock);
            return err;
        }
        buf[0] = BMP280_CTRLMEAS_REG_OSRSP__REG;
        err = bmp280_i2c_read_block(buf, 1);
        buf[0] = BMP_SET_BITSLICE(buf[0], BMP280_CTRLMEAS_REG_OSRSP, actual_oversampling_p);
        buf[1] = buf[0];
        buf[0] = BMP280_CTRLMEAS_REG_OSRSP__REG;
        err += bmp280_i2c_write_block(buf, 2);
    }

    if (err < 0)
        BARO_ERR("set pressure oversampling failed, err = %d\n", err);
    else
        obj->oversampling_p = oversampling_p;

    //mutex_unlock(&obj->lock);
    return err;
}

static int bmp280_set_oversampling_t(enum BMP_OVERSAMPLING_ENUM oversampling_t)
{
    unsigned char err = 0, actual_oversampling_t = 0;
    struct bmp280_data *obj = &obj_data;
    unsigned char buf[2];
    BARO_LOG("oversampling_t = %d, old oversampling_t = %d\n", oversampling_t, obj->oversampling_t);

    if (oversampling_t == obj->oversampling_t)
        return 0;

    //mutex_lock(&obj->lock);

    if (obj->sensor_type == BMP280_TYPE) {/* BMP280 */
        if (oversampling_t == BMP_OVERSAMPLING_SKIPPED)
            actual_oversampling_t = BMP280_OVERSAMPLING_SKIPPED;
        else if (oversampling_t == BMP_OVERSAMPLING_1X)
            actual_oversampling_t = BMP280_OVERSAMPLING_1X;
        else if (oversampling_t == BMP_OVERSAMPLING_2X)
            actual_oversampling_t = BMP280_OVERSAMPLING_2X;
        else if (oversampling_t == BMP_OVERSAMPLING_4X)
            actual_oversampling_t = BMP280_OVERSAMPLING_4X;
        else if (oversampling_t == BMP_OVERSAMPLING_8X)
            actual_oversampling_t = BMP280_OVERSAMPLING_8X;
        else if (oversampling_t == BMP_OVERSAMPLING_16X)
            actual_oversampling_t = BMP280_OVERSAMPLING_16X;
        else {
            err = -1;
            BARO_ERR("invalid oversampling_t = %d\n", oversampling_t);
            //mutex_unlock(&obj->lock);
            return err;
        }
        buf[0] = BMP280_CTRLMEAS_REG_OSRST__REG;
        err = bmp280_i2c_read_block(buf, 1);
        buf[0] = BMP_SET_BITSLICE(buf[0], BMP280_CTRLMEAS_REG_OSRST, actual_oversampling_t);
        buf[1] = buf[0];
        buf[0] = BMP280_CTRLMEAS_REG_OSRST__REG;
        err += bmp280_i2c_write_block(buf, 2);
    }

    if (err < 0)
        BARO_ERR("set temperature oversampling failed\n");
    else
        obj->oversampling_t = oversampling_t;

    //mutex_unlock(&obj->lock);
    return err;
}

static int cust_set(void *data, int len)
{
    int err = 0;
    struct bmp280_data *obj = &obj_data;
    CUST_SET_REQ_P req = (CUST_SET_REQ_P)data;

    BARO_FUN(f);

    switch (req->cust.action) {
        case CUST_ACTION_SET_CUST:

            break;
        case CUST_ACTION_SHOW_REG:

            break;
        case CUST_ACTION_SET_TRACE:
            obj->trace= (req->setTrace.trace);
            break;
        default:
            err = -1;
            break;
    }

    return err;
}
static int bmp280_convert(int *x, int type)
{
    int err = 0, X = 0;
    struct bmp280_data *obj = &obj_data;
    X = *x;
    if (type == 0) {
        if (obj->sensor_type == BMP280_TYPE) {/* for pressure */
            long long v_x1_u32r = 0;
            long long v_x2_u32r = 0;
            long long p = 0;
            v_x1_u32r = ((long long)obj->t_fine) - 128000;
            v_x2_u32r = v_x1_u32r * v_x1_u32r * (long long)obj->bmp280_cali.dig_P6;
            v_x2_u32r = v_x2_u32r + ((v_x1_u32r * (long long)obj->bmp280_cali.dig_P5) << 17);
            v_x2_u32r = v_x2_u32r + (((long long)obj->bmp280_cali.dig_P4) << 35);
            v_x1_u32r = ((v_x1_u32r * v_x1_u32r * (long long)obj->bmp280_cali.dig_P3) >> 8) +
                        ((v_x1_u32r * (long long)obj->bmp280_cali.dig_P2) << 12);
            v_x1_u32r = (((((long long)1) << 47) + v_x1_u32r)) *
                        ((long long)obj->bmp280_cali.dig_P1) >> 33;
            if (v_x1_u32r == 0)
                /* Avoid exception caused by division by zero */
                return -1;
            p = 1048576 - X;
            //p = div64_s64(((p << 31) - v_x2_u32r) * 3125, v_x1_u32r);
            p = ((p << 31) - v_x2_u32r) * 3125 / v_x1_u32r;
            v_x1_u32r = (((long long)obj->bmp280_cali.dig_P9) *
                         (p >> 13) * (p >> 13)) >> 25;
            v_x2_u32r = (((long long)obj->bmp280_cali.dig_P8) * p) >> 19;
            p = ((p + v_x1_u32r + v_x2_u32r) >> 8) + (((long long)obj->bmp280_cali.dig_P7) << 4);
            *x = (int)p / 256;
        }

    } else {
        if (obj->sensor_type == BMP280_TYPE) {/*for temperature*/
            int v_x1_u32r = 0;
            int v_x2_u32r = 0;

            v_x1_u32r  = ((((X >> 3) - ((int)obj->bmp280_cali.dig_T1 << 1))) * ((int)obj->bmp280_cali.dig_T2)) >> 11;
            v_x2_u32r  = (((((X >> 4) - ((int)obj->bmp280_cali.dig_T1)) * ((X >> 4) -	((int)obj->bmp280_cali.dig_T1))) >> 12) * ((int)obj->bmp280_cali.dig_T3)) >> 14;
            obj->t_fine = v_x1_u32r + v_x2_u32r;
            *x  = ((v_x1_u32r + v_x2_u32r) * 5 + 128) >> 8;
        }
    }
    return err;
}

static int bmp280_get_temperature(int *temperature)
{
    int err = 0;
    struct bmp280_data *obj = &obj_data;
    if (obj->sensor_type == BMP280_TYPE) {/* BMP280 */
        unsigned char buff[3] = {0};
        buff[0] = BMP280_TEMPERATURE_MSB_REG;
        err = bmp280_i2c_read_block(buff, 3);
        if (err < 0) {
            return BMP280_ERR_I2C;
        }
        *temperature = (int)((((unsigned int)(buff[0])) << SHIFT_LEFT_12_POSITION) |
                             (((unsigned int)(buff[1])) << SHIFT_LEFT_4_POSITION) | ((unsigned int)buff[2] >> SHIFT_RIGHT_4_POSITION));
    }
    bmp280_convert(temperature, TEMP);
    return err;
}

static int bmp280_get_pressure(int *pressure)
{
    int err = 0;
    struct bmp280_data *obj = &obj_data;
    if (obj->sensor_type == BMP280_TYPE) {/* BMP280 */
        unsigned char buff[3] = {0};
        buff[0] = BMP280_PRESSURE_MSB_REG;
        err = bmp280_i2c_read_block(buff, 3);
        if (err < 0) {
            return BMP280_ERR_I2C;
        }
        *pressure = (int)((((unsigned int)(buff[0])) << SHIFT_LEFT_12_POSITION) |
                          (((unsigned int)(buff[1])) << SHIFT_LEFT_4_POSITION) | ((unsigned int)buff[2] >> SHIFT_RIGHT_4_POSITION));
    }
    bmp280_convert(pressure, PRESSURE);
    return err;
}

static int bmp280_ReadSensorData(int *data)
{
    int err = 0;
    int data_temp = 0, data_pressure = 0;
    BARO_FUN(f);
    err = bmp280_get_temperature(&data_temp);
    if (err < 0) {
        BARO_ERR("bmp280_get_temperature ERROR\n");
        return err;
    }
    err = bmp280_get_pressure(&data_pressure);
    if (err < 0) {
        BARO_ERR("bmp280_get_pressure ERROR\n");
        return err;
    }
    data[PRESSURE] = data_pressure;
    data[TEMP] = data_temp;
    return 0;
}

int bmp280_init_client(void)
{
    int err = 0;
    BARO_FUN();

    err = bmp280_get_chip_type();
    if (err < 0) {
        BARO_ERR("get chip type failed, err = %d\n", err);
        return err;
    }

    err = bmp280_get_calibration_data();
    if (err < 0) {
        BARO_ERR("get calibration data failed, err = %d\n", err);
        return err;
    }

    err = bmp280_set_powermode(BMP_NORMAL_MODE);
    if (err < 0) {
        BARO_ERR("set power mode failed, err = %d\n", err);
        return err;
    }

    err = bmp280_set_filter(BMP_FILTER_8);
    if (err < 0) {
        BARO_ERR("set hw filter failed, err = %d\n", err);
        return err;
    }

    err = bmp280_set_oversampling_p(BMP_OVERSAMPLING_8X);
    if (err < 0) {
        BARO_ERR("set pressure oversampling failed, err = %d\n", err);
        return err;
    }

    err = bmp280_set_oversampling_t(BMP_OVERSAMPLING_1X);
    if (err < 0) {
        BARO_ERR("set temperature oversampling failed, err = %d\n", err);
        return err;
    }

    return 0;
}

int bmp280_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;
    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            BARO_LOG("bmp280g_operation command ACTIVATE\n");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                BARO_ERR("Enable sensor parameter error!\n");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                //mutex_acquire(&mpu6515_mutex);
                if (0 == value) {
                    err = bmp280_set_powermode(BMP_SUSPEND_MODE);
                    if (err < 0) {
                        BARO_LOG("BMP280 power off fail\n");
                        err = -1;
                    }
                } else {
                    err = bmp280_set_powermode(BMP_NORMAL_MODE);
                    if (err < 0) {
                        BARO_LOG("BMP280 power on fail\n");
                        err = -1;
                    }
                }
                //mutex_release(&mpu6515_mutex);
            }
            break;
        case SETDELAY:
            BARO_LOG("bmp280g_operation command SETDELAY\n");
            break;
        case SETCUST:
            BARO_LOG("bmp280g_operation command SETCUST\n");
            \
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                BARO_ERR("Enable sensor parameter error!\n");
                err = -1;
            } else {
                if (err != cust_set(buffer_in, size_in)) {
                    BARO_ERR("Set customization error : %d\n", err);
                }
            }
            break;
        default:
            break;
    }
    return err;
}

int bmp280_run_algorithm(struct data_t *output)
{
    int ret = 0;
    int buff[DATA_SIZE];
    ret = bmp280_ReadSensorData(buff);
    if (ret < 0) {
        return ret;
    }
    output->data->sensor_type= SENSOR_TYPE_PRESSURE;
    output->data_exist_count = 1;
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->relative_humidity_t.relative_humidity = buff[PRESSURE];
    output->data->relative_humidity_t.temperature = buff[TEMP];
    return ret;
}
//#define BMP280_TESTSUITE_CASE
#ifdef BMP280_TESTSUITE_CASE
static void Bmp280testsample(void *pvParameters)
{
    //struct data_t sensor_event_t;
    //sensor_event_t.data = (struct data_unit_t *)pvPortMalloc(sizeof(struct data_unit_t));
    while(1) {
        kal_taskENTER_CRITICAL();
        //bmp280_run_algorithm(&sensor_event_t);
        //BARO_ERR("pressure: %d, temperature: %d\n", sensor_event_t.data->pressure_t.pressure,
        //sensor_event_t.data->pressure_t.temperature);
        kal_taskEXIT_CRITICAL();
        vTaskDelay(200/portTICK_RATE_MS);
    }
}
#endif

int bmp280_sensor_init(void)
{
    int ret = 0;
    BARO_FUN(f);
    struct SensorDescriptor_t  bmp280_descriptor_t;
    struct bmp280_data *obj = &obj_data;

    bmp280_descriptor_t.sensor_type = SENSOR_TYPE_PRESSURE;
    bmp280_descriptor_t.version =  1;
    bmp280_descriptor_t.report_mode = continus;
    bmp280_descriptor_t.hw.max_sampling_rate = 10;
    bmp280_descriptor_t.hw.support_HW_FIFO = 0;

    bmp280_descriptor_t.input_list = NULL;

    bmp280_descriptor_t.operate = bmp280_operation;
    bmp280_descriptor_t.run_algorithm = bmp280_run_algorithm;
    bmp280_descriptor_t.set_data = NULL;

    obj->hw = get_cust_baro_hw();
    if (NULL == obj->hw) {
        BARO_ERR("get_cust_baro_hw fail\n\r");
        return ret;
    }

    obj->i2c_addr = obj->hw->i2c_addr[0];
    BARO_LOG("i2c_num: %d, i2c_addr: 0x%x\n\r", obj->hw->i2c_num, obj->i2c_addr);

    ret = bmp280_init_client();
    if (ret < 0) {
        BARO_ERR("bmp280_init_client fail\n");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_PRESSURE, 1);
    if (ret < 0) {
        BARO_ERR("RegisterDataBuffer fail\n");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_type(&bmp280_descriptor_t);
    if(ret) {
        BARO_ERR("RegisterAlgorithm fail\n");
        return ret;
    }

    ret = bmp280_set_powermode(BMP_SUSPEND_MODE);
    if (ret < 0) {
        BARO_LOG("BMP280 power off fail\n");
        ret = -1;
    }

#ifdef BMP280_TESTSUITE_CASE
    kal_xTaskCreate(Bmp280testsample, "bmp280", 512, (void*)NULL, 3, NULL);
#endif
    return ret;
}
MODULE_DECLARE(bmp280, MOD_PHY_SENSOR, bmp280_sensor_init);
