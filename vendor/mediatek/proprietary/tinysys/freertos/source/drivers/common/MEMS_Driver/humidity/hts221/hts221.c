#include <hts221.h>
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
#include "cust_hmdy.h"

//#define HTS221_LOG_MSG
#define HMDY_TAG                "[humidity] "
#define HMDY_ERR(fmt, arg...)   PRINTF_D(HMDY_TAG"%d: "fmt, __LINE__, ##arg)

#ifdef HTS221_LOG_MSG
#define HMDY_LOG(fmt, arg...)   PRINTF_D(HMDY_TAG fmt, ##arg)
#define HMDY_FUN(f)             PRINTF_D("%s\n", __FUNCTION__)
#else
#define HMDY_LOG(fmt, arg...)
#define HMDY_FUN(f)
#endif
static struct hts221_data obj_data;

int hts221_i2c_read_block(unsigned char *data, unsigned char len)
{
    int err = 0;
    struct hts221_data *obj = &obj_data;

    struct mt_i2c_t i2c;
    i2c.id    = obj->hw->i2c_num;
    i2c.addr  = obj->i2c_addr; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;

    err = i2c_write_read(&i2c, data, 1, len);
    if (0 != err) {
        HMDY_ERR("i2c_read fail: %d\n", err);
    }
    return err;

}
int hts221_i2c_write_block(unsigned char *data, unsigned char len)
{
    int err = 0;
    struct hts221_data *obj = &obj_data;

    struct mt_i2c_t i2c;
    i2c.id    = obj->hw->i2c_num;
    i2c.addr  = obj->i2c_addr; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;

    err = i2c_write(&i2c, data, len);
    if (0 != err) {
        HMDY_ERR("i2c_write fail: %d\n", err);
    }
    return err;
}


static int hts221_check_ID(void)
{
    int res;
    unsigned char buf[2];
    HMDY_FUN();
    buf[0] = REG_WHOAMI_ADDR;
    res = hts221_i2c_read_block(buf, 1);
    if (res < 0)
        return HTS221_ERR_I2C;
    HMDY_LOG("HTS211 device ID is 0X%XH\n", buf[0]);
    return res;
}
static int hts221_set_sampling_period(unsigned char new_sampling, unsigned char BDU)
{
    int res;
    unsigned char buf[2];
    HMDY_FUN();
    buf[0] = REG_CNTRL1_ADDR;
    res = hts221_i2c_read_block(buf, 1);
    if (res < 0)
        return HTS221_ERR_I2C;
    buf[0] = (buf[0] & HTS221_ODR_MASK) | new_sampling;
    buf[0] = (buf[0] & HTS221_BDU_MASK) | BDU;
    buf[1] = buf[0];
    buf[0] = REG_CNTRL1_ADDR;
    res = hts221_i2c_write_block(buf, 2);
    if (res < 0)
        return HTS221_ERR_I2C;
    return res;
}
static int hts221_set_resolution(unsigned char new_resolution, unsigned char mask)
{
    int res;
    unsigned char buf[2];
    HMDY_FUN();
    buf[0] = REG_AVCONFIG_ADDR;
    res = hts221_i2c_read_block(buf, 1);
    if (res < 0)
        return HTS221_ERR_I2C;

    buf[0] = (buf[0] & mask) | new_resolution;
    buf[1] = buf[0];
    buf[0] = REG_AVCONFIG_ADDR;
    res = hts221_i2c_write_block(buf, 2);
    if (res < 0)
        return HTS221_ERR_I2C;
    return res;
}
static int hts221_get_calibration_data(void)
{
    int err;
    unsigned char humidity_calibration[2], temperature_calibration[2];
    struct hts221_data *obj_i2c_data = &obj_data;
    HMDY_FUN();
    humidity_calibration[0] = REG_0RH_CAL_X_H + 0x80;
    err = hts221_i2c_read_block(humidity_calibration, 2);
    if (err < 0)
        return HTS221_ERR_I2C;
    obj_i2c_data->hts221_cali.calibX0 = ((int)( (short)((humidity_calibration[1] << 8) |
                                         (humidity_calibration[0]))));
    humidity_calibration[0] = REG_1RH_CAL_X_H + 0x80;
    err = hts221_i2c_read_block(humidity_calibration, 2);
    if (err < 0)
        return HTS221_ERR_I2C;
    obj_i2c_data->hts221_cali.calibX1 = ((int)( (short)((humidity_calibration[1] << 8) |
                                         (humidity_calibration[0]))));
    humidity_calibration[0] = REG_0RH_CAL_Y_H;
    err = hts221_i2c_read_block(humidity_calibration, 1);
    if (err < 0)
        return HTS221_ERR_I2C;
    obj_i2c_data->hts221_cali.calibY0 = (unsigned short) humidity_calibration[0];

    humidity_calibration[0] = REG_1RH_CAL_Y_H;
    err = hts221_i2c_read_block(humidity_calibration, 1);
    if (err < 0)
        return HTS221_ERR_I2C;
    obj_i2c_data->hts221_cali.calibY1 = (unsigned short) humidity_calibration[0];

    obj_i2c_data->hts221_cali.h_slope=((obj_i2c_data->hts221_cali.calibY1-obj_i2c_data->hts221_cali.calibY0)*8000)/(obj_i2c_data->hts221_cali.calibX1-obj_i2c_data->hts221_cali.calibX0);
    obj_i2c_data->hts221_cali.h_b=(((obj_i2c_data->hts221_cali.calibX1*obj_i2c_data->hts221_cali.calibY0)-(obj_i2c_data->hts221_cali.calibX0*obj_i2c_data->hts221_cali.calibY1))*1000)/(obj_i2c_data->hts221_cali.calibX1-obj_i2c_data->hts221_cali.calibX0);
    obj_i2c_data->hts221_cali.h_b=obj_i2c_data->hts221_cali.h_b*8;

    temperature_calibration[0] = REG_0T_CAL_X_L + 0x80;
    err = hts221_i2c_read_block(temperature_calibration, 2);
    if (err < 0)
        return HTS221_ERR_I2C;
    obj_i2c_data->hts221_cali.calibW0 = ((int)( (short)((temperature_calibration[1] << 8) |
                                         (temperature_calibration[0]))));
    temperature_calibration[0] = REG_1T_CAL_X_L + 0x80;
    err = hts221_i2c_read_block(temperature_calibration, 2);
    if (err < 0)
        return HTS221_ERR_I2C;
    obj_i2c_data->hts221_cali.calibW1 = ((int)( (short)((temperature_calibration[1] << 8) |
                                         (temperature_calibration[0]))));

    temperature_calibration[0] = REG_0T_CAL_Y_H;
    err = hts221_i2c_read_block(temperature_calibration, 1);
    if (err < 0)
        return HTS221_ERR_I2C;
    obj_i2c_data->hts221_cali.calibZ0a = (unsigned short) temperature_calibration[0];
    temperature_calibration[0] = REG_T1_T0_CAL_Y_H;
    err = hts221_i2c_read_block(temperature_calibration, 1);
    if (err < 0)
        return HTS221_ERR_I2C;
    obj_i2c_data->hts221_cali.calibZ0b = (unsigned short) ( temperature_calibration[0] & (0x3) );
    obj_i2c_data->hts221_cali.calibZ0 = ((int)( (obj_i2c_data->hts221_cali.calibZ0b << 8) | (obj_i2c_data->hts221_cali.calibZ0a)));
    temperature_calibration[0] = REG_1T_CAL_Y_H;
    err = hts221_i2c_read_block(temperature_calibration, 1);
    if (err < 0)
        return HTS221_ERR_I2C;
    obj_i2c_data->hts221_cali.calibZ1a = (unsigned short) temperature_calibration[0];
    temperature_calibration[0] = REG_T1_T0_CAL_Y_H;
    err = hts221_i2c_read_block(temperature_calibration, 1);
    if (err < 0)
        return HTS221_ERR_I2C;
    obj_i2c_data->hts221_cali.calibZ1b = (unsigned short)( temperature_calibration[0] & (0xC) );
    obj_i2c_data->hts221_cali.calibZ1b = obj_i2c_data->hts221_cali.calibZ1b >> 2;
    obj_i2c_data->hts221_cali.calibZ1 = ((int) ( (obj_i2c_data->hts221_cali.calibZ1b << 8) | (obj_i2c_data->hts221_cali.calibZ1a)));

    obj_i2c_data->hts221_cali.t_slope=((obj_i2c_data->hts221_cali.calibZ1-obj_i2c_data->hts221_cali.calibZ0)*8000)/(obj_i2c_data->hts221_cali.calibW1-obj_i2c_data->hts221_cali.calibW0);
    obj_i2c_data->hts221_cali.t_b=(((obj_i2c_data->hts221_cali.calibW1*obj_i2c_data->hts221_cali.calibZ0)-(obj_i2c_data->hts221_cali.calibW0*obj_i2c_data->hts221_cali.calibZ1))*1000)/(obj_i2c_data->hts221_cali.calibW1-obj_i2c_data->hts221_cali.calibW0);
    obj_i2c_data->hts221_cali.t_b=obj_i2c_data->hts221_cali.t_b*8;
//#ifdef DEBUF
    HMDY_LOG(" reading calibX0=%X calibX1=%X \n", obj_i2c_data->hts221_cali.calibX0, obj_i2c_data->hts221_cali.calibX1);
    HMDY_LOG(" reading calibX0=%d calibX1=%d \n", obj_i2c_data->hts221_cali.calibX0, obj_i2c_data->hts221_cali.calibX1);
    HMDY_LOG(" reading calibW0=%X calibW1=%X \n", obj_i2c_data->hts221_cali.calibW0, obj_i2c_data->hts221_cali.calibW1);
    HMDY_LOG(" reading calibW0=%d calibW1=%d \n", obj_i2c_data->hts221_cali.calibW0, obj_i2c_data->hts221_cali.calibW1);
    HMDY_LOG(" reading calibY0=%X calibY1=%X \n", obj_i2c_data->hts221_cali.calibY0, obj_i2c_data->hts221_cali.calibY1);
    HMDY_LOG(" reading calibY0=%u calibY1=%u \n", obj_i2c_data->hts221_cali.calibY0, obj_i2c_data->hts221_cali.calibY1);
    HMDY_LOG(" reading calibZ0a=%X calibZ0b=%X calibZ0=%X \n", obj_i2c_data->hts221_cali.calibZ0a, obj_i2c_data->hts221_cali.calibZ0b, obj_i2c_data->hts221_cali.calibZ0);
    HMDY_LOG(" reading calibZ0a=%u calibZ0b=%u calibZ0=%d \n", obj_i2c_data->hts221_cali.calibZ0a, obj_i2c_data->hts221_cali.calibZ0b, obj_i2c_data->hts221_cali.calibZ0);
    HMDY_LOG(" reading calibZ1a=%X calibZ1b=%X calibZ1=%X \n", obj_i2c_data->hts221_cali.calibZ1a, obj_i2c_data->hts221_cali.calibZ1b, obj_i2c_data->hts221_cali.calibZ1);
    HMDY_LOG(" reading calibZ1a=%u calibZ1b=%u calibZ1=%d \n", obj_i2c_data->hts221_cali.calibZ1a, obj_i2c_data->hts221_cali.calibZ1b, obj_i2c_data->hts221_cali.calibZ1);
    HMDY_LOG(" reading t_slope=%X t_b=%X h_slope=%X h_b=%X \n", obj_i2c_data->hts221_cali.t_slope, obj_i2c_data->hts221_cali.t_b, obj_i2c_data->hts221_cali.h_slope, obj_i2c_data->hts221_cali.h_b );
    HMDY_LOG(" reading t_slope=%d t_b=%d h_slope=%d h_b=%d \n", obj_i2c_data->hts221_cali.t_slope, obj_i2c_data->hts221_cali.t_b, obj_i2c_data->hts221_cali.h_slope, obj_i2c_data->hts221_cali.h_b );
//#endif

    return 0;
}

static int hts221_device_power_off(void)
{
    int res;
    unsigned char buf[2];

    HMDY_FUN();
    buf[0] = REG_CNTRL1_ADDR;
    res = hts221_i2c_read_block(buf, 1);
    if (res < 0)
        return HTS221_ERR_I2C;

    buf[0] = (buf[0] & HTS221_POWER_MASK) | DISABLE_SENSOR;
    buf[1] = buf[0];
    buf[0] = REG_CNTRL1_ADDR;

    res = hts221_i2c_write_block(buf, 2);
    if (res < 0)
        return HTS221_ERR_I2C;
    return res;

}

static int hts221_device_power_on(void)
{
    int res;
    unsigned char buf[2];

    HMDY_FUN();
    buf[0] = REG_CNTRL1_ADDR;
    res = hts221_i2c_read_block(buf, 1);
    if (res < 0)
        return HTS221_ERR_I2C;

    buf[0] = (buf[0] & HTS221_POWER_MASK) | ENABLE_SENSOR;
    buf[1] = buf[0];
    buf[0] = REG_CNTRL1_ADDR;
    res = hts221_i2c_write_block(buf, 2);
    if (res < 0)
        return HTS221_ERR_I2C;
    return res;
}

static int hts221_set_powermode(enum HTS_POWERMODE_ENUM power_mode)
{
    HMDY_FUN();
    int err;
    if (power_mode == HTS_NORMAL_MODE) {
        err = hts221_device_power_on();
        if (err < 0) {
            return -1;
        }

    } else {
        err = hts221_device_power_off();
        if (err < 0) {
            return -1;
        }
    }
    return 0;
}

/*****************************************************
 * Linear interpolation: (x0,y0) (x1,y1) y = ax+b
 *
 * a = (y1-y0)/(x1-x0)
 * b = (x1*y0-x0*y1)/(x1-x0)
 *
 * result = ((y1-y0)*x+((x1*y0)-(x0*y1)))/(x1-x0)
 *
 * For Humidity
 * (x1,y1) = (H1_T0_OUT, H1_RH)
 * (x0,y0) = (H0_T0_OUT, H0_RH)
 * x       =  H_OUT
 *
 * For Temperature
 * (x1,y1) = (T1_OUT, T1_DegC)
 * (x0,y0) = (T0_OUT, T0_DegC)
 * x       =  T_OUT
******************************************************/
static int hts221_convert(int slope, int b_gen, int *x, int type)
{
    int err = 0;
    int X = 0;

    X = *x;

    *x = ((slope*X)+b_gen);


    if (type == 0)
        *x = (*x) >>4;  /*for Humidity, m RH*/

    else
        *x = (*x) >>6;  /*for Humidity, m RH*/

    return err;
}
static int hts221_start_convert(void)
{
    int err;
    unsigned char buff[2];
    HMDY_FUN();
    buff[0] = REG_CNTRL2_ADDR;
    buff[1] = START_NEW_CONVERT;
    err = hts221_i2c_write_block(buff, 2);
    if (err < 0) {
        return HTS221_ERR_I2C;
    }
    return err;
}

/*
*get compensated temperature
*unit:1000 degrees centigrade
*/
static int hts221_get_temperature(int *data_temp)
{
	int err = 0;
	unsigned char buf[2];
	struct hts221_data *obj_i2c_data = &obj_data;
	int data_t = 0;
	HMDY_FUN();
	buf[0] = REG_T_OUT_L + 0x80;
	err = hts221_i2c_read_block(buf, 2);
	if (err < 0)
		return HTS221_ERR_I2C;
	data_t = (int)((short)((buf[1] << 8) | buf[0]));
	HMDY_LOG("temperature raw data: 0x%x\n", data_t);
	err = hts221_convert(obj_i2c_data->hts221_cali.t_slope, obj_i2c_data->hts221_cali.t_b, &data_t, TEMP);
	if (err < 0)
			return err;
	HMDY_LOG("hts221 get temperature: %d rH\n", data_t);
	*data_temp = data_t;
	return 0;
}
/*
*get compensated humidity
*unit: 1000 %rH
*/
static int hts221_get_humidity(int *data_humi)
{
	int err = 0;
	unsigned char buf[2];
	struct hts221_data *obj_i2c_data = &obj_data;
	int data_h = 0;
	HMDY_FUN();
	buf[0] = REG_H_OUT_L + 0x80;
	err = hts221_i2c_read_block(buf, 2);
	if (err < 0)
	    return HTS221_ERR_I2C;
	data_h = (int)( (short)((buf[1] << 8) | buf[0])); 
	HMDY_LOG("humidity raw data: 0x%x\n", data_h);
	err = hts221_convert(obj_i2c_data->hts221_cali.h_slope, obj_i2c_data->hts221_cali.h_b, &data_h, HUMIDITY);
	if (err < 0)
			return err;

    HMDY_LOG("hts221 get humidity: %d rH\n", data_h);
    *data_humi = data_h;
    return 0;
}



static int hts221_ReadSensorData(int *data)
{
    int err = 0;
    int data_temp = 0, data_humi = 0;
    HMDY_FUN(f);
    err = hts221_start_convert();
    if (err < 0) {
        HMDY_ERR("hts221_start_convert ERROR\n");
        return err;
    }
    err = hts221_get_humidity(&data_humi);
    if (err < 0) {
        HMDY_ERR("hts221_get_humidity ERROR\n");
        return err;
    }
    err = hts221_get_temperature(&data_temp);
    if (err < 0) {
        HMDY_ERR("hts221_get_temperature ERROR\n");
        return err;
    }
    data[HUMIDITY] = data_humi;
    data[TEMP] = data_temp;
    return 0;
}

static int cust_set(void *data, int len)
{
    int err = 0;
    struct hts221_data *obj_i2c_data = &obj_data;
    CUST_SET_REQ_P req = (CUST_SET_REQ_P)data;

    HMDY_FUN(f);

    switch (req->cust.action) {
        case CUST_ACTION_SET_CUST:

            break;
        case CUST_ACTION_SHOW_REG:

            break;
        case CUST_ACTION_SET_TRACE:
            obj_i2c_data->trace= (req->setTrace.trace);
            break;
        default:
            err = -1;
            break;
    }

    return err;
}

int hts221_init_client(void)
{
    int res = 0;
    HMDY_FUN(f);
    res = hts221_check_ID();
    if (res < 0) {
        HMDY_ERR("Error reading WHO_AM_I: device is not available\n");
        return res;
    }

    res = hts221_set_sampling_period(HTS221_ODR_ONE_SHOT, HTS221_BDU);
    if (res < 0) {
        HMDY_ERR("Error hts221_set_sampling_period\n");
        return res;
    }
    res = hts221_set_powermode(HTS_NORMAL_MODE);
    if (res < 0) {
        HMDY_ERR("Error hts221_set_powermode\n");
        return res;
    }

    res = hts221_set_resolution(HTS221_H_RESOLUTION_32, HTS221_H_RESOLUTION_MASK);
    if (res < 0) {
        HMDY_ERR("Error hts221_set_resolution humidity\n");
        return res;
    }

    res = hts221_set_resolution(HTS221_T_RESOLUTION_16, HTS221_T_RESOLUTION_MASK);
    if (res < 0) {
        HMDY_ERR("Error hts221_set_resolution humidity\n");
        return res;
    }

    res = hts221_set_powermode(HTS_NORMAL_MODE);
    if (res < 0) {
        HMDY_ERR("Error hts221_set_powermode\n");
        return res;
    }
    //hts221_read_result(client, REG_CNTRL2_ADDR);

    res = hts221_get_calibration_data();
    if (res < 0) {
        HMDY_ERR("Error hts221_get_calibration_data\n");
        return res;
    }
    return res;
}

int hts221_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;
    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            HMDY_LOG("hts221g_operation command ACTIVATE\n");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                HMDY_ERR("Enable sensor parameter error!\n");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                //mutex_acquire(&mpu6515_mutex);
                if (0 == value) {
                    err = hts221_set_powermode(HTS_SUSPEND_MODE);
                    if (err < 0) {
                        HMDY_LOG("HTS221 power off fail\n");
                        err = -1;
                    }
                } else {
                    err = hts221_set_powermode(HTS_NORMAL_MODE);
                    if (err < 0) {
                        HMDY_LOG("HTS221 power on fail\n");
                        err = -1;
                    }
                }
                //mutex_release(&mpu6515_mutex);
            }
            break;
        case SETDELAY:
            HMDY_LOG("hts221g_operation command SETDELAY\n");
            break;
        case SETCUST:
            HMDY_LOG("hts221g_operation command SETCUST\n");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                HMDY_ERR("Enable sensor parameter error!\n");
                err = -1;
            } else {
                if (err != cust_set(buffer_in, size_in)) {
                    HMDY_ERR("Set customization error : %d\n", err);
                }
            }
            break;
        default:
            break;
    }
    return err;
}

int hts221_run_algorithm(struct data_t *output)
{
    int ret = 0;
    int buff[DATA_SIZE];
    ret = hts221_ReadSensorData(buff);
    if (ret < 0) {
        return ret;
    }

    output->data->sensor_type = SENSOR_TYPE_RELATIVE_HUMIDITY;
    output->data_exist_count = 1;
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->relative_humidity_t.relative_humidity = buff[HUMIDITY];
    output->data->relative_humidity_t.temperature = buff[TEMP];

    return ret;
}
//#define HTS221_TESTSUITE_CASE
#ifdef HTS221_TESTSUITE_CASE
static void Hts221testsample(void *pvParameters)
{
    //struct data_t sensor_event_t;
    //sensor_event_t.data = (struct data_unit_t *)pvPortMalloc(sizeof(struct data_unit_t));
    while(1) {
        kal_taskENTER_CRITICAL();
        //hts221_run_algorithm(&sensor_event_t);
        //HMDY_LOG("humidity: %d, temperature: %d\n", sensor_event_t.data->relative_humidity_t.relative_humidity,
        //sensor_event_t.data->relative_humidity_t.temperature);
        kal_taskEXIT_CRITICAL();
        vTaskDelay(200/portTICK_RATE_MS);
    }
}
#endif
int hts221_sensor_init(void)
{
    int ret = 0;
    HMDY_FUN(f);
    struct SensorDescriptor_t  hts221_descriptor_t;
    struct hts221_data *obj = &obj_data;

    hts221_descriptor_t.sensor_type = SENSOR_TYPE_RELATIVE_HUMIDITY;
    hts221_descriptor_t.version =  1;
    hts221_descriptor_t.report_mode = on_change;
    hts221_descriptor_t.hw.max_sampling_rate = 5;
    hts221_descriptor_t.hw.support_HW_FIFO = 0;

    hts221_descriptor_t.input_list = NULL;

    hts221_descriptor_t.operate = hts221_operation;
    hts221_descriptor_t.run_algorithm = hts221_run_algorithm;
    hts221_descriptor_t.set_data = NULL;

    obj->hw = get_cust_hmdy_hw();
    if (NULL == obj->hw) {
        HMDY_ERR("get_cust_hmdy_hw fail\n\r");
        return ret;
    }

    obj->i2c_addr = obj->hw->i2c_addr[0];
    HMDY_LOG("i2c_num: %d, i2c_addr: 0x%x\n\r", obj->hw->i2c_num, obj->i2c_addr);

    ret = hts221_init_client();
    if (ret < 0) {
        HMDY_ERR("hts221_init_client fail\n");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_RELATIVE_HUMIDITY, 1);
    if (ret < 0) {
        HMDY_ERR("RegisterDataBuffer fail\n");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_type(&hts221_descriptor_t);
    if(ret) {
        HMDY_ERR("RegisterAlgorithm fail\n");
        return ret;
    }
#ifdef HTS221_TESTSUITE_CASE
    kal_xTaskCreate(Hts221testsample, "hts221", 512, (void*)NULL, 3, NULL);
#endif
    return ret;
}
MODULE_DECLARE(hts221, MOD_PHY_SENSOR, hts221_sensor_init);
