#include <akm09912.h>
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
#include "AkmApi.h"
#include "hwsen.h"


//#define QUICK_CALIBRATE
//#define AKM09912_LOG_MSG
#define MAG_TAG                "[MSENSOR] "
#define MAG_ERR(fmt, arg...)   PRINTF_D(MAG_TAG"%d: "fmt, __LINE__, ##arg)

#ifdef AKM09912_LOG_MSG
#define MAG_LOG(fmt, arg...)   PRINTF_D(MAG_TAG fmt, ##arg)
#define MAG_FUN(f)             PRINTF_D("%s\n", __FUNCTION__)
#else
#define MAG_LOG(fmt, arg...)
#define MAG_FUN(f)
#endif

static struct akm09912_data obj_data;

int mag_i2c_read_block(unsigned char *data, unsigned char len)
{
    int err = 0;
//#ifdef I2C_CONFIG
    struct akm09912_data *obj = &obj_data;
    struct mt_i2c_t i2c;
    i2c.id	  = obj->hw->i2c_num;
    i2c.addr  = obj->i2c_addr; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;

    err = i2c_write_read(&i2c, data, 1, len);
    if (0 != err) {
        MAG_ERR("i2c_read fail: %d\n", err);
    }
//#endif

    return err;

}
int mag_i2c_write_block(unsigned char *data, unsigned char len)
{
    int err = 0;
//#ifdef I2C_CONFIG
    struct akm09912_data *obj = &obj_data;
    struct mt_i2c_t i2c;
    i2c.id    = obj->hw->i2c_num;
    i2c.addr  = obj->i2c_addr; /* 7-bit address of EEPROM */
    i2c.mode  = FS_MODE;
    i2c.speed = 400;

    err = i2c_write(&i2c, data, len);
    if (0 != err) {
        MAG_ERR("i2c_write fail: %d\n", err);
    }
//#endif

    return err;
}

static int akm09912_SetPowerMode(unsigned char mode)
{
    unsigned char databuf[2];
    int res = 0;
    MAG_FUN(f);
    databuf[0] = AKM09912_REG_CNTL2;
    databuf[1] = mode;
    res = mag_i2c_write_block(databuf, 2);
    if (res < 0) {
        return AKM09912_ERR_I2C;
    }
    return AKM09912_SUCCESS;
}
static int akm09912_ReadData(int *data)
{
    unsigned char buf[AKM09912_DATA_LEN] = {0};
    int err = 0;

    MAG_FUN(f);
    /*buf_st[0] = AKM09912_REG_ST1;
    if ((err = mag_i2c_read_block(buf_st, 1)) != 0)
    	return AKM09912_ERR_I2C;*/

    buf[0] = AKM09912_REG_HXL;
    if ((err = mag_i2c_read_block(buf, AKM09912_DATA_LEN)) != 0) {
        return AKM09912_ERR_I2C;
    } else {
        /* little and big debian  need check */
        data[MAG_AXIS_X] = (short)((buf[MAG_AXIS_X*2+1] << 8) | (buf[MAG_AXIS_X*2]));
        data[MAG_AXIS_Y] = (short)((buf[MAG_AXIS_Y*2+1] << 8) | (buf[MAG_AXIS_Y*2]));
        data[MAG_AXIS_Z] = (short)((buf[MAG_AXIS_Z*2+1] << 8) | (buf[MAG_AXIS_Z*2]));

        MAG_LOG("[%08X %08X %08X] => [%5d %5d %5d], st2: %d\n", data[MAG_AXIS_X], data[MAG_AXIS_Y], data[MAG_AXIS_Z],
                data[MAG_AXIS_X], data[MAG_AXIS_Y], data[MAG_AXIS_Z], buf[7]);
    }
    return AKM09912_SUCCESS;
}

static int akm09912_ReadSensorData(int *data_uT)
{
    int res = 0;
    //unsigned char buff[2];
    //unsigned char i = 0;
    int data_uncali[MAG_AXES_NUM];
    int remap_data[MAG_AXES_NUM];
    struct akm09912_data *obj = &obj_data;
    MAG_FUN(f);
    /*res = akm09912_SetPowerMode(AKM09912_MODE_SNG_MEASURE);
    if (res < 0) {
        MAG_ERR("akm09912_SetPowerMode error!\n");
        return AKM09912_ERR_I2C;
    }
    for ( ; i < 3; i++) {
        buff[0] = AKM09912_REG_ST1;
        MAG_LOG("check ready bit for %d times\n", i);
        res = mag_i2c_read_block(buff, 1);
        if (res < 0) {
            MAG_ERR("akm09912 read status error!\n");
            return AKM09912_ERR_I2C;
        }
        if ((buff[0] & DATA_READY_BIT) == DATA_READY_BIT) {
            break;
        }
        vTaskDelay(5 / portTICK_RATE_MS);//TODO: ADD DELAY

    }*/
    if (0 != (res = akm09912_ReadData(data_uncali))) {
        MAG_ERR("akm09912_ReadData error: ret value=%d\n", res);
        return AKM09912_ERR_I2C;
    }

    remap_data[obj->cvt.map[MAG_AXIS_X]] = obj->cvt.sign[MAG_AXIS_X] * data_uncali[MAG_AXIS_X];
    remap_data[obj->cvt.map[MAG_AXIS_Y]] = obj->cvt.sign[MAG_AXIS_Y] * data_uncali[MAG_AXIS_Y];
    remap_data[obj->cvt.map[MAG_AXIS_Z]] = obj->cvt.sign[MAG_AXIS_Z] * data_uncali[MAG_AXIS_Z];

    MAG_LOG("msensor remap data: %d, %d, %d!\n", remap_data[MAG_AXIS_X], remap_data[MAG_AXIS_Y], remap_data[MAG_AXIS_Z]);

    data_uT[MAG_AXIS_X] = remap_data[MAG_AXIS_X];
    data_uT[MAG_AXIS_Y] = remap_data[MAG_AXIS_Y];
    data_uT[MAG_AXIS_Z] = remap_data[MAG_AXIS_Z];

    MAG_LOG("msensor output data: %d, %d, %d!\n", data_uT[MAG_AXIS_X], data_uT[MAG_AXIS_Y], data_uT[MAG_AXIS_Z]);
    return res;
}

static int cust_set(void *data, int len)
{
    int err = 0;
    struct akm09912_data *obj = &obj_data;
    CUST_SET_REQ_P req = (CUST_SET_REQ_P)data;

    MAG_FUN(f);

    switch (req->cust.action) {
        case CUST_ACTION_SET_CUST:

            break;
        case CUST_ACTION_SET_DIRECTION:
            MAG_LOG("CUST_ACTION_SET_ORIENTATION\n\r");
            obj->hw->direction = (req->setDirection.direction);
            MAG_LOG("set orientation: %d\n\r", obj->hw->direction);
            if (0 != (err = sensor_driver_get_convert(obj->hw->direction, &obj->cvt))) {
                MAG_ERR("invalid direction: %d\n\r", obj->hw->direction);
            }
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


int akm09912_init_client(void)
{
    int res = 0;
    unsigned char buff[4];
    MAG_FUN(f);
    res = akm09912_SetPowerMode(AKM09912_MODE_FUSE_ACCESS);

    if (res < 0) {
        MAG_ERR("akm09912_SetPowerMode error!\n");
        return AKM09912_ERR_I2C;
    }
    buff[0] = AKM09912_REG_ASAX;
    if ((res = mag_i2c_read_block(buff, 3)) != 0) {
        return AKM09912_ERR_I2C;
    }
    MAG_LOG("ASAX = 0X%x, ASAY = 0X%x, ASAZ = 0X%x\n\r", buff[0], buff[1], buff[2]);
    res = akm09912_SetPowerMode(AKM09912_MODE_POWERDOWN);
    return res;
}

int akm09912_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;
    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            MAG_LOG("akm09912_operation command ACTIVATE\n");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                MAG_ERR("Enable sensor parameter error!\n");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                //mutex_acquire(&mpu6515_mutex);
                if (0 == value) {
                    //akm_open();
                    err = akm09912_SetPowerMode(AKM09912_MODE_POWERDOWN);
                    if (err < 0) {
                        MAG_ERR("akm09912_SetPowerMode error!\n");
                        err = -1;
                    }

                } else {
                    //akm_close();
                    err = akm09912_SetPowerMode(AKM09912_MODE_100HZ_MEASURE);
                    if (err < 0) {
                        MAG_ERR("akm09912_SetPowerMode error!\n");
                        err = -1;
                    }
                }
                //mutex_release(&mpu6515_mutex);
            }
            break;
        case SETDELAY:
            MAG_LOG("akm09912_operation command SETDELAY\n");
            break;
        case SETCUST:
            MAG_LOG("akm09912_operation command SETCUST\n");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                MAG_ERR("Enable sensor parameter error!\n");
                err = -1;
            } else {
                if (err != cust_set(buffer_in, size_in)) {
                    MAG_ERR("Set customization error : %d\n", err);
                    err = -1;
                }
            }
            break;
        default:
            break;
    }
    return err;
}
int akm09912_run_algorithm(struct data_t *output)
{
    int ret = 0, err = 0;
    int buff[MAG_AXES_NUM];
    INT16 offset[MAG_AXES_NUM];
    double data_cali[MAG_AXES_NUM];
    double data_offset[MAG_AXES_NUM];
    int16_t status = 0;
    int64_t timestamp = 0;

    ret = akm09912_ReadSensorData(buff);
    if (ret < 0) {
        return ret;
    }

    timestamp = read_xgpt_stamp_ns();

    MAG_LOG("msensor raw data: %d, %d, %d, timestamp: %lld!\n", buff[MAG_AXIS_X], buff[MAG_AXIS_Y], buff[MAG_AXIS_Z], timestamp);
    err = AKM_SetMagData(buff[MAG_AXIS_X], buff[MAG_AXIS_Y], buff[MAG_AXIS_Z], timestamp);
    if (err < 0) {
        MAG_LOG("akm_set_mag_data fail\n\r");
    } else {
        AKM_Calibrate(data_cali, data_offset, &status);
        MAG_LOG("msensor recv data_cali: %f, %f, %f, offset: %f, %f, %f, status: %d!\n",
                data_cali[MAG_AXIS_X], data_cali[MAG_AXIS_Y], data_cali[MAG_AXIS_Z],
                data_offset[MAG_AXIS_X], data_offset[MAG_AXIS_Y], data_offset[MAG_AXIS_Z], status);
        buff[MAG_AXIS_X] = (int)(data_cali[MAG_AXIS_X] * VENDOR_DIV);
        buff[MAG_AXIS_Y] = (int)(data_cali[MAG_AXIS_Y] * VENDOR_DIV);
        buff[MAG_AXIS_Z] = (int)(data_cali[MAG_AXIS_Z] * VENDOR_DIV);
        offset[MAG_AXIS_X] = (INT16)(data_offset[MAG_AXIS_X] * VENDOR_DIV);
        offset[MAG_AXIS_Y] = (INT16)(data_offset[MAG_AXIS_Y] * VENDOR_DIV);
        offset[MAG_AXIS_Z] = (INT16)(data_offset[MAG_AXIS_Z] * VENDOR_DIV);
        MAG_LOG("msensor recv data: %d, %d, %d!\n", buff[MAG_AXIS_X], buff[MAG_AXIS_Y], buff[MAG_AXIS_Z]);
        output->data_exist_count        = 1;
        output->data->sensor_type		= SENSOR_TYPE_MAGNETIC_FIELD;
        output->data->time_stamp		= timestamp;
        output->data->magnetic_t.x      = buff[MAG_AXIS_X];
        output->data->magnetic_t.y      = buff[MAG_AXIS_Y];
        output->data->magnetic_t.z 	    = buff[MAG_AXIS_Z];
        output->data->magnetic_t.x_bias = offset[MAG_AXIS_X];
        output->data->magnetic_t.y_bias = offset[MAG_AXIS_Y];
        output->data->magnetic_t.z_bias = offset[MAG_AXIS_Z];
        output->data->magnetic_t.status = status;
        MAG_LOG("msensor time: %lld, recv data_cali: %d, %d, %d, offset: %d, %d, %d, status: %d!\n",
                output->data->time_stamp, output->data->magnetic_t.x, output->data->magnetic_t.y, output->data->magnetic_t.z,
                output->data->magnetic_t.x_bias, output->data->magnetic_t.y_bias, output->data->magnetic_t.z_bias,
                output->data->magnetic_t.status);
    }

    return ret;
}
#ifdef QUICK_CALIBRATE
int akm09912_set_data(const struct data_t *input_list, void *reserve)
{
    int ret = 0, i = 0;
    double x, y, z;

    for (i = 0; i < input_list->data_exist_count; ++i) {
        x = (double)input_list->data[i].gyroscope_t.x / 57295;
        y = (double)input_list->data[i].gyroscope_t.y / 57295;
        z = (double)input_list->data[i].gyroscope_t.z / 57295;
        MAG_LOG("msensor recv GYRO_DATA: %f, %f, %f, timestamp: %lld!\n", x, y, z, input_list->data[i].time_stamp);

        ret = AKM_SetGyroData(x, y, z, input_list->data[i].time_stamp);
    }
    return ret;
}
#endif
int akm09912_sensor_init(void)
{
    int ret = 0;
    MAG_FUN(f);
    struct SensorDescriptor_t  akm09912_descriptor_t;
    struct akm09912_data *obj = &obj_data;

    akm09912_descriptor_t.sensor_type = SENSOR_TYPE_MAGNETIC_FIELD;
    akm09912_descriptor_t.version =  1;
    akm09912_descriptor_t.report_mode = continus;
    akm09912_descriptor_t.hw.max_sampling_rate = 5;
    akm09912_descriptor_t.hw.support_HW_FIFO = 0;
#ifdef QUICK_CALIBRATE
    struct input_list_t gyro_list;
    akm09912_descriptor_t.input_list = &gyro_list;
    gyro_list.input_type = SENSOR_TYPE_GYROSCOPE;
    gyro_list.sampling_delay = 20;
    gyro_list.next_input = NULL;
#else
    akm09912_descriptor_t.input_list = NULL;
#endif
    akm09912_descriptor_t.operate = akm09912_operation;
    akm09912_descriptor_t.run_algorithm = akm09912_run_algorithm;

#ifdef QUICK_CALIBRATE
    akm09912_descriptor_t.set_data = akm09912_set_data;
#else
    akm09912_descriptor_t.set_data = NULL;
#endif
    obj->hw = get_cust_mag_hw();
    if (NULL == obj->hw) {
        MAG_ERR("get_cust_acc_hw fail\n");
        return ret;
    }
    obj->i2c_addr = obj->hw->i2c_addr[0];
    MAG_LOG("i2c_num: %d, i2c_addr: 0x%x\n", obj->hw->i2c_num, obj->i2c_addr);

    if (0 != (ret = sensor_driver_get_convert(obj->hw->direction, &obj->cvt))) {
        MAG_ERR("invalid direction: %d\n", obj->hw->direction);
    }
    MAG_LOG("map[0]:%d, map[1]:%d, map[2]:%d, sign[0]:%d, sign[1]:%d, sign[2]:%d\n\r",
            obj->cvt.map[MAG_AXIS_X], obj->cvt.map[MAG_AXIS_Y], obj->cvt.map[MAG_AXIS_Z],
            obj->cvt.sign[MAG_AXIS_X], obj->cvt.sign[MAG_AXIS_Y], obj->cvt.sign[MAG_AXIS_Z]);
    ret = akm09912_init_client();
    if (ret < 0) {
        MAG_ERR("akm09912_init_client fail\n");
        return ret;
    }
    //akm_lib_init();
    AKM_Open();
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_MAGNETIC_FIELD, 1);
    if (ret < 0) {
        MAG_ERR("RegisterDataBuffer fail\n");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_type(&akm09912_descriptor_t);
    if(ret) {
        MAG_ERR("RegisterAlgorithm fail\n");
        return ret;
    }

    return ret;
}
MODULE_DECLARE(akm09912, MOD_PHY_SENSOR, akm09912_sensor_init);
#ifdef USE_VENDOR_ORIENTATION
int orientation_set_data(struct data_t *input_list, void *reserve)
{
    int ret = 0;
    int64_t timestamp = 0;

    timestamp = read_xgpt_stamp_ns();
    ret = AKM_SetACCData(input_list->data->accelerometer_t.x, input_list->data->accelerometer_t.y,
                         input_list->data->accelerometer_t.z, timestamp);
}
static int orientation_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;
    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            MAG_LOG("orientation_operation command ACTIVATE\n");
            break;
        case SETDELAY:
            MAG_LOG("orientation_operation command SETDELAY\n");
            break;
        case SETCUST:
            MAG_LOG("orientation_operation command SETCUST\n");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                MAG_ERR("Enable sensor parameter error!\n");
                err = -1;
            } else {
                if (err != cust_set(buffer_in, size_in)) {
                    MAG_ERR("Set customization error : %d\n", err);
                    err = -1;
                }
            }
            break;
        default:
            break;
    }
    return err;
}

int orientation_run_algorithm(struct data_t *output)
{
    int ret = 0, err = 0;
    int buff[MAG_AXES_NUM];

    double data_cali[MAG_AXES_NUM];
    double data_offset[MAG_AXES_NUM];
    int16_t status = 0;
    int64_t timestamp = 0;

    ret = akm09912_ReadSensorData(buff);
    if (ret < 0) {
        return ret;
    }
    timestamp = read_xgpt_stamp_ns();
    MAG_LOG("msensor raw data: %d, %d, %d!\n", buff[MAG_AXIS_X], buff[MAG_AXIS_Y], buff[MAG_AXIS_Z]);
    err = AKM_SetMagData(buff[MAG_AXIS_X], buff[MAG_AXIS_Y], buff[MAG_AXIS_Z], timestamp);
    if (err < 0) {
        MAG_LOG("akm_set_mag_data fail\n\r");
    } else {
        //AKM_Calibrate(data_cali, data_offset, &status);
        MAG_LOG("msensor recv data_cali: %f, %f, %f, offset: %f, %f, %f!\n", data_cali[MAG_AXIS_X], data_cali[MAG_AXIS_Y], data_cali[MAG_AXIS_Z], data_offset[MAG_AXIS_X], data_offset[MAG_AXIS_Y], data_offset[MAG_AXIS_Z]);
        MAG_LOG("msensor recv status: %d!\n", status);
        data_cali[MAG_AXIS_X] = (int)data_cali[MAG_AXIS_X] * VENDOR_DIV;
        data_cali[MAG_AXIS_Y] = (int)data_cali[MAG_AXIS_Y] * VENDOR_DIV;
        data_cali[MAG_AXIS_Z] = (int)data_cali[MAG_AXIS_Z] * VENDOR_DIV;

        MAG_LOG("msensor recv data: %d, %d, %d!\n", data_cali[MAG_AXIS_X], data_cali[MAG_AXIS_Y], data_cali[MAG_AXIS_Z]);
        output->data_exist_count        = 1;
        output->data->sensor_type		= SENSOR_TYPE_ORIENTATION;
        output->data->orientation_t.x      = data_cali[MAG_AXIS_X];
        output->data->orientation_t.y      = data_cali[MAG_AXIS_Y];
        output->data->orientation_t.z 	    = data_cali[MAG_AXIS_Z];
        output->data->orientation_t.status = status;

    }

    return ret;
}

int orientation_sensor_init(void)
{
    int ret = 0;
    MAG_FUN(f);
    struct SensorDescriptor_t  orientation_descriptor_t;

    orientation_descriptor_t.sensor_type = SENSOR_TYPE_ORIENTATION;
    orientation_descriptor_t.version =  1;
    orientation_descriptor_t.report_mode = continus;
    orientation_descriptor_t.hw.max_sampling_rate = 5;
    orientation_descriptor_t.hw.support_HW_FIFO = 0;
    struct input_list_t acc_list;
    orientation_descriptor_t.input_list = &acc_list;
    acc_list.input_type = SENSOR_TYPE_ACCELEROMETER;
    acc_list.sampling_delay = 10;
    acc_list.next_input = NULL;

    orientation_descriptor_t.operate = orientation_operation;
    orientation_descriptor_t.run_algorithm = orientation_run_algorithm;

    orientation_descriptor_t.set_data = orientation_set_data;

    ret = RegisterDataBuffer(SENSOR_TYPE_ORIENTATION, 1);
    if (ret < 0) {
        MAG_ERR("RegisterDataBuffer fail\n");
        return ret;
    }
    ret = RegisterAlgorithm(&orientation_descriptor_t);
    if(ret) {
        MAG_ERR("RegisterAlgorithm fail\n");
        return ret;
    }

    return ret;
}
module_init(orientation_sensor_init);
#endif


