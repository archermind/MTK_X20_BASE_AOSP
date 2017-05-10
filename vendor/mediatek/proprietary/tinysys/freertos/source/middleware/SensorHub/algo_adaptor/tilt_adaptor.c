#include "sensors.h"
#include "sensor_manager.h"
#include "stdio.h"
#include "init_learning.h"
#include "feature_setting.h"
#include "tilt.h"
#include "signal.h"
#include "extract_feature.h"
#include "algo_adaptor.h"
#include "sensor_manager.h"
#include "context.h"

#ifdef _PC_VERSION_
#define LOGE(fmt, args...)    printf("[Tilt] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    printf("[Tilt] DBG: "fmt, ##args)
#else
#define LOGE(fmt, args...)    PRINTF_D("[Tilt] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    PRINTF_D("[Tilt] DBG: "fmt, ##args)
#endif

struct input_list_t input_comp_acc;
static resampling_t tilt_resampling = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static int start_notify_tilt = 0;

struct tilt_adaptor_t {
    uint32_t time_stamp;
    uint64_t time_stamp_ns;
    uint32_t curr_set_time;
    uint32_t last_set_time;
    int state;
};

static struct tilt_adaptor_t tilt_adaptor;

// return the state of tilt data after rule-based calculation
static INT32 run_tilt(struct data_t *const output)
{
    static int tilt_output = 0;
    tilt_get_alg_result(&tilt_output);
    output->data_exist_count = 1;
    //output->data->time_stamp = tilt_adaptor.time_stamp_ns;
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->tilt_event.state = tilt_output;
    output->data->sensor_type = SENSOR_TYPE_TILT_DETECTOR;

#ifndef _PC_VERSION_
//    PRINTF_D("[RUN_TILT_O]%d\n", tilt_output);
#endif
    get_tilt_result_ptr(1); // reset current gravity

#ifdef _PC_VERSION_DEBUG_
    // printf("tilt state = %d\n", tilt_output);
#endif
    return 1;
}

static void run_tilt_algorithm(uint32_t input_time_stamp_ms)
{
    // finished initialization,
    if (tilt_adaptor.state == INPUT_STATE_INIT) {
        tilt_adaptor.last_set_time = input_time_stamp_ms;
        tilt_adaptor.state = INPUT_STATE_PREPARE_DATA;
    }

    INT32 time_diff;
    time_diff = input_time_stamp_ms - tilt_adaptor.last_set_time;

    if (((tilt_adaptor.state == INPUT_STATE_PREPARE_DATA) && (time_diff >= MIN_SAMPLES_TIME)) || \
            ((tilt_adaptor.state == INPUT_STATE_UPDATE) && (time_diff >= TILT_FEATURE_UPDATE_TIME))) {
        tilt_adaptor.state = INPUT_STATE_UPDATE;
        tilt_adaptor.last_set_time = input_time_stamp_ms;
        tilt_set_ts(input_time_stamp_ms);
        tilt_alg_enter_point();
    }
    int *tilt_output;
    tilt_output = get_tilt_result_ptr(0);
    if (*tilt_output && start_notify_tilt) {
        sensor_subsys_algorithm_notify(SENSOR_TYPE_TILT_DETECTOR);

#ifndef _PC_VERSION_
//        PRINTF_D("[NOTIFY_TILT]\n");
#endif
    }
#ifndef _PC_VERSION_
//    PRINTF_D("[SET_TILT]%d\n", *tilt_output);
#endif
}
static INT32 set_tilt_data(const struct data_t *input_list, void *reserve)
{
    //static int temp_tilt_state = 0;
    struct data_unit_t *data_start = input_list->data;
    uint32_t input_time_stamp_ms = data_start->time_stamp / 1000000; //input time stamp (ms)
    tilt_adaptor.time_stamp_ns = data_start->time_stamp; //input time stamp (ns)

    int count = input_list->data_exist_count;
    if (data_start->sensor_type == SENSOR_TYPE_ACCELEROMETER) {

        if (!tilt_resampling.init_flag) {
            tilt_resampling.last_time_stamp = input_time_stamp_ms;
            tilt_resampling.init_flag = 1;
            return 0;
        }

        Activity_Accelerometer_Ms2_Data_t acc_input = {0};

        while (count != 0) {
            input_time_stamp_ms = data_start->time_stamp / 1000000; //input time stamp (ms)
//#ifndef _PC_VERSION_
//            PRINTF_D("[TILT_Acc_I]%u, %d, %d, %d, %d\n", input_time_stamp_ms, count, data_start->accelerometer_t.x,
//                     data_start->accelerometer_t.y, data_start->accelerometer_t.z);
//#endif
            // prepare input for floating input
            // resampling
            sensor_vec_t *data_acc_t = &data_start->accelerometer_t;

            tilt_resampling.current_time_stamp = input_time_stamp_ms;
            sensor_subsys_algorithm_resampling_type(&tilt_resampling);

            acc_input.time_stamp = tilt_resampling.last_time_stamp;
            acc_input.x = data_acc_t->x;
            acc_input.y = data_acc_t->y;
            acc_input.z = data_acc_t->z;

            while (tilt_resampling.input_count > 0) {
                acc_input.time_stamp = acc_input.time_stamp + tilt_resampling.input_sample_delay;
                get_signal_acc(acc_input); /* using address acc_buff to update acc measurements */
                run_tilt_algorithm(acc_input.time_stamp);
                tilt_resampling.input_count--;
                tilt_resampling.last_time_stamp = tilt_resampling.current_time_stamp;
            }
            data_start++;
            count--;
        }
    } else {
        return 0;
    }



    return 1;
}

static INT32 tilt_operate(Sensor_Command command, void* buffer_in, INT32 size_in, \
                          void* buffer_out, INT32 size_out)
{
    int err = 0;
    int value = 0;
    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            //PRINTF_D("SMD ACTIVATE\n\r");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                //PRINTF_D("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                tilt_adaptor.state = INPUT_STATE_INIT;
                value = *(int *)buffer_in;
                if (0 == value) {
                    start_notify_tilt = 0;
                    tilt_resampling.init_flag = 0;
                } else {
                    start_notify_tilt = 1;
                    tilt_original_reinit();
                }
            }
            break;
        default:
            break;
    }
#ifndef _PC_VERSION_
//    PRINTF_D("[set tilt]%d\n", start_notify_tilt);
#endif
    return err;
}


//return: fail=-1, pass>=0, which means the count of current register algorithm
static int tilt_register(void)
{
    int ret = 0;
    input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
    input_comp_acc.sampling_delay = TILT_INPUT_SAMPLE_DELAY * TILT_ACC_FIFO_NUM;
    input_comp_acc.next_input = NULL;

    struct SensorDescriptor_t tilt_desp = {
        SENSOR_TYPE_TILT_DETECTOR, 1, one_shot, {20, 0},
        &input_comp_acc, tilt_operate, run_tilt,
        set_tilt_data, TILT_INPUT_ACCUMULATE
    };

    ret = sensor_subsys_algorithm_register_type(&tilt_desp);
    if (ret < 0)
        LOGE("fail to register Tilt check \r\n");
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_TILT_DETECTOR, 1);
    if (ret < 0)
        LOGE("fail to register buffer for tilt \r\n");

    return ret;
}

int tilt_init(void)
{
    uint32_t init_time = read_xgpt_stamp_ns() / 1000000;

    tilt_original_reinit();
    tilt_adaptor.curr_set_time = 0;
    tilt_adaptor.last_set_time = 0;
    tilt_adaptor.state = INPUT_STATE_INIT;
    tilt_resampling.init_flag = 0;
    tilt_resampling.input_sample_delay = TILT_INPUT_SAMPLE_DELAY;

    tilt_algorithm_init(init_time);
    tilt_register();
    return 1;
}
#ifdef _EVEREST_MODULE_DECLARE_
MODULE_DECLARE(virt_tilt_init, MOD_VIRT_SENSOR, tilt_init);
#endif
