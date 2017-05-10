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


//#define UNCALI_MAG_LOG_MSG
#define UNMAG_TAG                "[UNCALI_MAG] "
#define UNMAG_ERR(fmt, arg...)   PRINTF_D(UNMAG_TAG"%d: "fmt, __LINE__, ##arg)

#ifdef UNCALI_MAG_LOG_MSG
#define UNMAG_LOG(fmt, arg...)   PRINTF_D(UNMAG_TAG fmt, ##arg)
#define UNMAG_FUN(f)             PRINTF_D("%s\n", __FUNCTION__)
#else
#define UNMAG_LOG(fmt, arg...)
#define UNMAG_FUN(f)
#endif
struct data_unit_t unmag_data;
static int data_exist_count;
int uncali_mag_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            UNMAG_ERR("uncali_mag_operation command ACTIVATE :%d\n", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                UNMAG_ERR("Enable sensor parameter error!\n");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    memset(&unmag_data, 0, sizeof(struct data_unit_t));
                } else {

                }
            }
            break;
        case SETDELAY:
            UNMAG_ERR("uncali_mag_operation command SETDELAY:%d\n", *(int *)buffer_in);
            break;
        case SETCUST:
            UNMAG_LOG("uncali_mag_operation command SETCUST\n");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                UNMAG_ERR("Enable sensor parameter error!\n");
                err = -1;
            } else {

            }
            break;
        default:
            break;
    }
    return err;
}
int uncali_mag_run_algorithm(struct data_t *output)
{
    int ret = 0;

    output->data_exist_count = data_exist_count;
    if (output->data_exist_count <= 0)
        return 0;
    output->data->sensor_type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
    output->data->time_stamp = unmag_data.time_stamp;
    output->data->uncalibrated_mag_t.x = unmag_data.uncalibrated_mag_t.x +
                                         unmag_data.uncalibrated_mag_t.x_bias;
    output->data->uncalibrated_mag_t.y = unmag_data.uncalibrated_mag_t.y +
                                         unmag_data.uncalibrated_mag_t.y_bias;
    output->data->uncalibrated_mag_t.z = unmag_data.uncalibrated_mag_t.z +
                                         unmag_data.uncalibrated_mag_t.z_bias;
    output->data->uncalibrated_mag_t.x_bias = unmag_data.uncalibrated_mag_t.x_bias;
    output->data->uncalibrated_mag_t.y_bias = unmag_data.uncalibrated_mag_t.y_bias;
    output->data->uncalibrated_mag_t.z_bias = unmag_data.uncalibrated_mag_t.z_bias;
    UNMAG_LOG("unmag time: %lld, recv data_cali: %d, %d, %d, offset: %d, %d, %d, status: %d!\n",
              output->data->time_stamp, output->data->uncalibrated_mag_t.x, output->data->uncalibrated_mag_t.y,
              output->data->uncalibrated_mag_t.z,
              output->data->uncalibrated_mag_t.x_bias, output->data->uncalibrated_mag_t.y_bias,
              output->data->uncalibrated_mag_t.z_bias,
              output->data->uncalibrated_mag_t.status);
    return ret;
}
int uncali_mag_set_data(const struct data_t *input_list, void *reserve)
{
    int ret = 0;
    data_exist_count = input_list->data_exist_count;
    unmag_data.time_stamp = input_list->data->time_stamp;
    unmag_data.uncalibrated_mag_t.x = input_list->data->magnetic_t.x;
    unmag_data.uncalibrated_mag_t.y = input_list->data->magnetic_t.y;
    unmag_data.uncalibrated_mag_t.z = input_list->data->magnetic_t.z;
    unmag_data.uncalibrated_mag_t.x_bias = input_list->data->magnetic_t.x_bias;
    unmag_data.uncalibrated_mag_t.y_bias = input_list->data->magnetic_t.y_bias;
    unmag_data.uncalibrated_mag_t.z_bias = input_list->data->magnetic_t.z_bias;
    return ret;
}
int uncali_mag_sensor_init(void)
{
    int ret = 0;
    UNMAG_FUN(f);
    struct SensorDescriptor_t  uncali_mag_descriptor_t;

    uncali_mag_descriptor_t.sensor_type = SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
    uncali_mag_descriptor_t.version =  1;
    uncali_mag_descriptor_t.report_mode = continus;
    uncali_mag_descriptor_t.hw.max_sampling_rate = 5;
    uncali_mag_descriptor_t.hw.support_HW_FIFO = 0;

    struct input_list_t mag_list;
    uncali_mag_descriptor_t.input_list = &mag_list;
    mag_list.input_type = SENSOR_TYPE_MAGNETIC_FIELD;
    mag_list.sampling_delay = 20;
    mag_list.next_input = NULL;

    uncali_mag_descriptor_t.operate = uncali_mag_operation;
    uncali_mag_descriptor_t.run_algorithm = uncali_mag_run_algorithm;
    uncali_mag_descriptor_t.set_data = uncali_mag_set_data;


    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED, 1);
    if (ret < 0) {
        UNMAG_ERR("RegisterDataBuffer fail\n");
        return ret;
    }
    ret = sensor_subsys_algorithm_register_type(&uncali_mag_descriptor_t);
    if (ret) {
        UNMAG_ERR("RegisterAlgorithm fail\n");
        return ret;
    }

    return ret;
}
MODULE_DECLARE(uncali_mag, MOD_PHY_SENSOR, uncali_mag_sensor_init);
