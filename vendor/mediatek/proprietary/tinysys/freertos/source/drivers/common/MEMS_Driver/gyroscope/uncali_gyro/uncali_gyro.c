#include "sensor_manager.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <FreeRTOSConfig.h>
#include <platform.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hwsen.h"
#include "API_sensor_calibration.h"
#include "mpe_cm4_API.h"

//#define UNCALI_GYRO_LOG_MSG
#define UNGYRO_TAG                "[UNGYRO] "
#define UNGYRO_ERR(fmt, arg...)   PRINTF_D(UNGYRO_TAG"%d: "fmt, __LINE__, ##arg)

#ifdef UNCALI_GYRO_LOG_MSG
#define UNGYRO_LOG(fmt, arg...)   PRINTF_D(UNGYRO_TAG fmt, ##arg)
#define UNGYRO_FUN(f)             PRINTF_D("%s\n\r", __FUNCTION__)
#else
#define UNGYRO_LOG(fmt, arg...)
#define UNGYRO_FUN(f)
#endif

static int data_exist_count;
struct data_unit_t ungyro_data_t[MAX_GYROSCOPE_FIFO_SIZE];

int uncali_gyro_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            UNGYRO_ERR("uncali_gyro_operation command ACTIVATE: %d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                UNGYRO_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    memset(ungyro_data_t, 0 , sizeof(struct data_unit_t) * MAX_GYROSCOPE_FIFO_SIZE);
                } else {

                }
            }
            break;
        case SETDELAY:
            UNGYRO_ERR("uncali_gyro_operation command SETDELAY: %d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                UNGYRO_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {

            }
            break;
        case SETCUST:
            UNGYRO_LOG("uncali_gyro_operation command SETCUST\n\r");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                UNGYRO_ERR("Enable sensor parameter error!\n\r");
                err = -1;
            } else {

            }
            break;
        default:
            break;
    }
    return err;
}

int uncali_gyro_run_algorithm(struct data_t *output)
{
    int i = 0;
    output->data_exist_count = data_exist_count;
    if (output->data_exist_count <= 0)
        return 0;
    for (; i < data_exist_count; ++i) {
        output->data[i].sensor_type = SENSOR_TYPE_GYROSCOPE_UNCALIBRATED;
        output->data[i].time_stamp = ungyro_data_t[i].time_stamp;
        output->data[i].uncalibrated_gyro_t.x = ungyro_data_t[i].uncalibrated_gyro_t.x +
                                                ungyro_data_t[i].uncalibrated_gyro_t.x_bias;
        output->data[i].uncalibrated_gyro_t.y = ungyro_data_t[i].uncalibrated_gyro_t.y +
                                                ungyro_data_t[i].uncalibrated_gyro_t.y_bias;
        output->data[i].uncalibrated_gyro_t.z = ungyro_data_t[i].uncalibrated_gyro_t.z +
                                                ungyro_data_t[i].uncalibrated_gyro_t.z_bias;
        output->data[i].uncalibrated_gyro_t.x_bias = ungyro_data_t[i].uncalibrated_gyro_t.x_bias;
        output->data[i].uncalibrated_gyro_t.y_bias = ungyro_data_t[i].uncalibrated_gyro_t.y_bias;
        output->data[i].uncalibrated_gyro_t.z_bias = ungyro_data_t[i].uncalibrated_gyro_t.z_bias;
        UNGYRO_LOG("calibrate time: %lld, x:%d, y:%d, z:%d, x_bias:%d, y_bias:%d, z_bias:%d\r\n",
                   output->data[i].time_stamp, output->data[i].uncalibrated_gyro_t.x,
                   output->data[i].uncalibrated_gyro_t.y, output->data[i].uncalibrated_gyro_t.z,
                   output->data[i].uncalibrated_gyro_t.x_bias, output->data[i].uncalibrated_gyro_t.y_bias,
                   output->data[i].uncalibrated_gyro_t.z_bias);
    }
    return 0;
}
int uncali_gyro_set_data(const struct data_t *input_list, void *reserve)
{
    int ret = 0, i = 0;

    data_exist_count = input_list->data_exist_count;
    for (i = 0; i < input_list->data_exist_count; ++i) {
        ungyro_data_t[i].time_stamp = input_list->data[i].time_stamp;
        ungyro_data_t[i].uncalibrated_gyro_t.x = input_list->data[i].uncalibrated_gyro_t.x;
        ungyro_data_t[i].uncalibrated_gyro_t.y = input_list->data[i].uncalibrated_gyro_t.y;
        ungyro_data_t[i].uncalibrated_gyro_t.z = input_list->data[i].uncalibrated_gyro_t.z;
        ungyro_data_t[i].uncalibrated_gyro_t.x_bias = input_list->data[i].uncalibrated_gyro_t.x_bias;
        ungyro_data_t[i].uncalibrated_gyro_t.y_bias = input_list->data[i].uncalibrated_gyro_t.y_bias;
        ungyro_data_t[i].uncalibrated_gyro_t.z_bias = input_list->data[i].uncalibrated_gyro_t.z_bias;
    }
    return ret;
}

int uncali_gyro_sensor_init(void)
{
    int ret = 0;
    UNGYRO_FUN(f);
    struct SensorDescriptor_t  uncali_gyro_descriptor_t;

    uncali_gyro_descriptor_t.sensor_type = SENSOR_TYPE_GYROSCOPE_UNCALIBRATED;
    uncali_gyro_descriptor_t.version =  1;
    uncali_gyro_descriptor_t.report_mode = continus;
    uncali_gyro_descriptor_t.hw.max_sampling_rate = 5;
    uncali_gyro_descriptor_t.hw.support_HW_FIFO = 0;

    struct input_list_t gyro_list;
    uncali_gyro_descriptor_t.input_list = &gyro_list;
    gyro_list.input_type = SENSOR_TYPE_GYROSCOPE;
    gyro_list.sampling_delay = 20;
    gyro_list.next_input = NULL;

    uncali_gyro_descriptor_t.operate = uncali_gyro_operation;
    uncali_gyro_descriptor_t.run_algorithm = uncali_gyro_run_algorithm;
    uncali_gyro_descriptor_t.set_data = uncali_gyro_set_data;

    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_GYROSCOPE_UNCALIBRATED, MAX_GYROSCOPE_FIFO_SIZE);
    if (ret) {
        UNGYRO_ERR("RegisterDataBuffer fail\n\r");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_type(&uncali_gyro_descriptor_t);
    if (ret) {
        UNGYRO_ERR("RegisterAlgorithm fail\n\r");
        return ret;
    }
    return ret;
}
MODULE_DECLARE(uncali_gyro, MOD_PHY_SENSOR, uncali_gyro_sensor_init);
