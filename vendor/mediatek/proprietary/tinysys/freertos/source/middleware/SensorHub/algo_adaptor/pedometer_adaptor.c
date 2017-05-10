#include "sensors.h"
#include "stdio.h"
#include "pedometer.h"
#include "algo_adaptor.h"
#include "sensor_manager.h"
#include "math_method.h"
#include "FreeRTOS.h"

#include <stdint.h>

struct pedometer_adaptor_t {
    uint32_t time_stamp;
    uint64_t time_stamp_ns;
};

struct pedometer_acc_t {
    uint32_t time_stamp;
    int32_t acc_X;
    int32_t acc_Y;
    int32_t acc_Z;
};

static struct pedometer_adaptor_t pedometer_adaptor = {0, 0};
static struct pedometer_acc_t pedometer_acc = {0, 0, 0, 0};
static resampling_t pedometer_resampling = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static INT32 run_pedometer(struct data_t * const output)
{
    //calculate output;
    struct data_unit_t *pedometer_data = output->data;
    output->data_exist_count = 1;
    pedometer_data->sensor_type = SENSOR_TYPE_PEDOMETER;
    //pedometer_data->time_stamp = pedometer_adaptor.time_stamp_ns;
    pedometer_data->time_stamp = read_xgpt_stamp_ns();
    pedometer_data->pedometer_t.accumulated_step_count = get_pedometer_acc_step_count();
    pedometer_data->pedometer_t.accumulated_step_length = get_pedometer_acc_step_length();
    pedometer_data->pedometer_t.step_frequency = get_pedometer_step_frequency();
    pedometer_data->pedometer_t.step_length = get_pedometer_step_length();
    //PRINTF_D("pedo_result:%d\r\n", pedometer_data->pedometer_t.accumulated_step_count);
    return 1;
}

static INT32 set_pedometer_data(const struct data_t *input_list, void *reserve)
{
    //store input data

    struct data_unit_t *data_start = input_list->data;
    uint32_t input_time_stamp_ms = input_list->data->time_stamp / 1000000;
    pedometer_adaptor.time_stamp_ns = input_list->data->time_stamp;
    int count = input_list->data_exist_count;
    if (data_start->sensor_type == SENSOR_TYPE_ACCELEROMETER) {

        if (!pedometer_resampling.init_flag) {
            pedometer_resampling.last_time_stamp = input_time_stamp_ms;
            pedometer_resampling.init_flag = 1;
            //return 0;
        }

        while (count != 0) {
            input_time_stamp_ms = data_start->time_stamp / 1000000; //input time stamp (ms)
#ifndef _PC_VERSION_
            PRINTF_D("pedo_acc:%u, %d, %d, %d\n", input_time_stamp_ms, data_start->accelerometer_t.x,
                     data_start->accelerometer_t.y, data_start->accelerometer_t.z);
#endif
            // prepare input for floating input
            // resampling
            pedometer_resampling.current_time_stamp = input_time_stamp_ms;
            sensor_subsys_algorithm_resampling_type(&pedometer_resampling);
            sensor_vec_t *data_acc_t = &data_start->accelerometer_t;
            pedometer_acc.time_stamp = pedometer_resampling.last_time_stamp;
            pedometer_acc.acc_X = data_acc_t->x;
            pedometer_acc.acc_Y = data_acc_t->y;
            pedometer_acc.acc_Z = data_acc_t->z;
            while (pedometer_resampling.input_count > 0) {
                pedometer_acc.time_stamp += pedometer_resampling.input_sample_delay;
                pedometer_detector(pedometer_acc.time_stamp, pedometer_acc.acc_X, pedometer_acc.acc_Y, pedometer_acc.acc_Z);
                pedometer_resampling.input_count--;
                pedometer_resampling.last_time_stamp = pedometer_resampling.current_time_stamp;
            }
            data_start++;
            count--;
        }
    }

    return 1;
}
static int pedometer_operate(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PRINTF_D("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (SENSOR_DISABLE == value) {
                    pedometer_resampling.init_flag = 0;
                }
            }
            break;
        default:
            break;
    }
    return err;
}


int pedometer_register(void)
{
    int ret; /*return: fail=-1, pass>=0, which means the count of current register algorithm */

    struct SensorDescriptor_t  pedometer_desp;
    struct input_list_t gsensor_list;

    pedometer_desp.sensor_type = SENSOR_TYPE_PEDOMETER;
    pedometer_desp.version =  1;
    pedometer_desp.report_mode = on_change;
    pedometer_desp.hw.max_sampling_rate = 40;
    pedometer_desp.hw.support_HW_FIFO = 0;

    pedometer_desp.input_list = &gsensor_list;
    gsensor_list.input_type = SENSOR_TYPE_ACCELEROMETER;
    gsensor_list.sampling_delay = PEDOMETER_INPUT_SAMPLE_INTERVAL * PEDOMETER_ACC_FIFO_NUM;
    gsensor_list.next_input = NULL;

    pedometer_desp.operate = pedometer_operate;
    pedometer_desp.run_algorithm = run_pedometer;
    pedometer_desp.set_data = set_pedometer_data;

    pedometer_desp.accumulate = 250;

    ret = sensor_subsys_algorithm_register_type(&pedometer_desp);
    if (ret < 0) {
        PRINTF_D("fail to register Pedometer \r\n");
    }
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_PEDOMETER, 1);
    if (ret < 0) {
        PRINTF_D("fail to register buffer \r\n");
    }
    return ret;
}

int pedometer_init(void)
{
    int ret = 0;
    ret = pedometer_register();
    if (ret < 0) {
        PRINTF_D("fail to register pedometr\r\n");
    }
    pedometer_algorithm_init();
    pedometer_adaptor.time_stamp = 0;
    pedometer_resampling.init_flag = 0;
    pedometer_resampling.input_sample_delay = PEDOMETER_INPUT_SAMPLE_INTERVAL;

    return 1;
}
#ifdef _EVEREST_MODULE_DECLARE_
MODULE_DECLARE(virt_pedo_init, MOD_VIRT_SENSOR, pedometer_init);
#endif