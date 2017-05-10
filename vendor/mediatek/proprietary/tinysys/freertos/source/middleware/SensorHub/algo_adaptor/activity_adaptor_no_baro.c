#include "sensors.h"
#include "stdio.h"
#include "init_learning.h"
#include "context.h"
#include "activity_algorithm.h"
#include "signal.h"
#include "algo_adaptor.h"
#include "sensor_manager.h"

//#define _ACT_DEBUG_

#ifdef _PC_VERSION_
#define LOGE(fmt, args...)    printf("[Activity] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    printf("[Activity] DBG: "fmt, ##args)
#else
#define LOGE(fmt, args...)    PRINTF_D("[Activity] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    PRINTF_D("[Activity] DBG: "fmt, ##args)
#endif

struct input_list_t input_comp_acc;
struct input_list_t input_comp_tilt;
static resampling_t activity_acc_resampling = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
Activity_algorithm_output_t act_output;
activity_type_t act_last_state, act_temp_state;

//#define _PC_VERSION_DEBUG_
struct activity_adaptor_t {
    uint32_t notifity_data_count;
    uint32_t time_stamp;
    uint64_t time_stamp_ns;
    uint32_t prev_set_data_time;
    uint32_t set_data_timing;
    int input_state;

    // tilt part
    uint32_t last_tilt_time;
    INT32  tilt_dominate_activity; // will hold for TILT_HOLD_TIME
    INT32  tilt_start_unknown;
    //struct data_unit_t *tilt_data_ptr;
    uint32_t tilt_event_state;
};

static struct activity_adaptor_t activity_adaptor;

static INT32 run_activity(struct data_t * const output)
{
    context_get_alg_result(&act_output);

    struct data_unit_t *activity_data = output->data;
    output->data_exist_count = 1;
    activity_data->sensor_type = SENSOR_TYPE_ACTIVITY;
    //activity_data->time_stamp = activity_adaptor.time_stamp_ns;
    activity_data->time_stamp = read_xgpt_stamp_ns();
    uint8_t *output_prob = (uint8_t *) activity_data->activity_data_t.probability;
    output_prob[TILTING] = (uint8_t) act_output.tilt;
    output_prob[UNKNOWN] = (uint8_t) act_output.unknown;

    output_prob[SITTING] = 0;
    output_prob[LYING] = 0;
    output_prob[STANDING] = 0;

    if (act_output.tilt || act_output.unknown) {
        output_prob[STILL] = output_prob[WALKING] = output_prob[RUNNING] = output_prob[CLIMBING] = \
                             output_prob[IN_VEHICLE] = output_prob[ON_BICYCLE] = output_prob[ON_FOOT] = 0;
    } else {
        output_prob[STILL]      = (uint8_t) act_output.still;
        output_prob[WALKING]    = (uint8_t) act_output.walk;
        output_prob[RUNNING]    = (uint8_t) act_output.run;
        output_prob[CLIMBING]   = (uint8_t) act_output.stairs;
        output_prob[IN_VEHICLE] = (uint8_t) act_output.veh;
        output_prob[ON_BICYCLE] = (uint8_t) act_output.cycle;
        output_prob[ON_FOOT]    = (uint8_t) max3(act_output.walk, act_output.run, act_output.stairs);
    }

    //PRINTF_D("[ACT_O]%lld: %d, %d, %d, %d, %d, %d, %d, %d, %d\n", activity_data->time_stamp, output_prob[STILL], output_prob[WALKING],output_prob[RUNNING],output_prob[CLIMBING], output_prob[IN_VEHICLE], output_prob[ON_BICYCLE], output_prob[ON_FOOT], output_prob[TILTING], output_prob[UNKNOWN]);

    return 1;
}

static void run_activity_algorithm(uint32_t input_time_stamp_ms)
{
    activity_adaptor.time_stamp = input_time_stamp_ms;

    if (activity_adaptor.input_state == INPUT_STATE_INIT) {
        activity_adaptor.prev_set_data_time = input_time_stamp_ms;
        activity_adaptor.input_state = INPUT_STATE_PREPARE_DATA;
    }

    INT32 time_diff;
    time_diff = input_time_stamp_ms - activity_adaptor.prev_set_data_time;

    if (((activity_adaptor.input_state == INPUT_STATE_PREPARE_DATA) && \
            (time_diff >= MIN_SAMPLES_TIME)) ||
            ((activity_adaptor.input_state == INPUT_STATE_UPDATE) && \
             (time_diff >= activity_adaptor.set_data_timing))) {
        activity_adaptor.input_state = INPUT_STATE_UPDATE;
        activity_adaptor.prev_set_data_time = input_time_stamp_ms;
        context_alg_enter_point();
    }
}
static void set_tilt_data()
{
    // tilt event handler, tilt data ptr is assigned.
    // add the pre-condition before context_alg_enter_point
    // tilt event trigger tilt_dominate_activity hold for TILT_HOLD_TIME
    //if (activity_adaptor.tilt_data_ptr->tilt_event.state) {
    if (activity_adaptor.tilt_event_state) {
        activity_adaptor.tilt_dominate_activity = 1;
        activity_adaptor.tilt_start_unknown = 0;
        activity_adaptor.last_tilt_time = activity_adaptor.time_stamp;
    }
    // if tilt hold and exceed the latency
    else if (activity_adaptor.tilt_dominate_activity && \
             (activity_adaptor.time_stamp - activity_adaptor.last_tilt_time) > TILT_HOLD_TIME) {
        activity_adaptor.tilt_dominate_activity = 0;
        activity_adaptor.tilt_start_unknown = 1;
    }
    // after tilt hold, the state remains unknown for TIL_TO_KNOWN ms
    else if (activity_adaptor.tilt_start_unknown && \
             (activity_adaptor.time_stamp - activity_adaptor.last_tilt_time) > TILT_HOLD_TIME + UNKNOWN_HOLD_TIME) {
        activity_adaptor.tilt_start_unknown = 0;
    }

    // if tilt is detected and within a latency, act_output.tilt
    act_output.tilt = activity_adaptor.tilt_dominate_activity * 100;
    act_output.unknown = activity_adaptor.tilt_start_unknown * 100;

}

static INT32 set_activity_data(const struct data_t *input_list, void *reserve)
{
    //store input data
    struct data_unit_t *data_start = input_list->data;
    uint32_t input_time_stamp_ms = input_list->data->time_stamp / 1000000; //input time stamp (ms)
    activity_adaptor.time_stamp_ns = input_list->data->time_stamp; // input time stamp (ns)
    int count = input_list->data_exist_count;

    // tilt input, only set the pointer in activity adaptor
    if (data_start->sensor_type == SENSOR_TYPE_TILT_DETECTOR) {
#ifndef _PC_VERSION_
//        PRINTF_D("[ACT_Acc_I]tilt, %lld, %u, %d\n", activity_adaptor.time_stamp_ns, input_time_stamp_ms, data_start->tilt_event.state);
#endif

        //activity_adaptor.tilt_data_ptr = data_start;
        activity_adaptor.tilt_event_state = data_start->tilt_event.state;
        set_tilt_data();
        return 1;
    }

    if (data_start->sensor_type == SENSOR_TYPE_ACCELEROMETER) {
        if (!activity_acc_resampling.init_flag) {
            activity_acc_resampling.last_time_stamp = input_time_stamp_ms;
            activity_acc_resampling.init_flag = 1;
            return 0;
        }

        Activity_Accelerometer_Ms2_Data_t acc_input = {0};

        while (count != 0) {
            input_time_stamp_ms = data_start->time_stamp / 1000000; //input time stamp (ms)
#ifndef _PC_VERSION_
            PRINTF_D("[ACT_Acc_I]%u, %d, %d, %d, %d\n", input_time_stamp_ms, count, data_start->accelerometer_t.x,
                     data_start->accelerometer_t.y, data_start->accelerometer_t.z);
#endif
            // prepare input for floating input
            // resampling
            sensor_vec_t *data_acc_t = &data_start->accelerometer_t;

            activity_acc_resampling.current_time_stamp = input_time_stamp_ms;
            sensor_subsys_algorithm_resampling_type(&activity_acc_resampling);

            acc_input.time_stamp = activity_acc_resampling.last_time_stamp;
            acc_input.x = data_acc_t->x;
            acc_input.y = data_acc_t->y;
            acc_input.z = data_acc_t->z;

            while (activity_acc_resampling.input_count > 0) {
                acc_input.time_stamp = acc_input.time_stamp + activity_acc_resampling.input_sample_delay;
                get_signal_acc(acc_input); /* using address acc_buff to update acc measurements */
                run_activity_algorithm(acc_input.time_stamp);
                activity_acc_resampling.input_count--;
                activity_acc_resampling.last_time_stamp = activity_acc_resampling.current_time_stamp;
            }
            data_start++;
            count--;
        }

    }

#ifdef _PC_VERSION_
    int ACTIVITY_NOTIFY_DATA_NUM = 15;
    // continuously notify sensor manager
    if (activity_adaptor.notifity_data_count >= ACTIVITY_NOTIFY_DATA_NUM) {
        sensor_subsys_algorithm_notify(SENSOR_TYPE_ACTIVITY);
        activity_adaptor.notifity_data_count = 0;
    } else activity_adaptor.notifity_data_count++;
#endif

    return 1;
}

static INT32 activity_operate(Sensor_Command command, void* buffer_in, INT32 size_in, \
                              void* buffer_out, INT32 size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PRINTF_D("Enable sensor activity error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (SENSOR_DISABLE == value) {
                    activity_acc_resampling.init_flag = 0;
                }
            }
            break;
        default:
            break;
    }
    return err;
}

static int activity_register(void)
{
    int ret = 0;//return: fail=-1, pass>=0, which means the count of current register algorithm

    input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
    input_comp_acc.sampling_delay = ACTIVITY_INPUT_SAMPLE_DELAY * ACTIVITY_ACC_FIFO_NUM;

    input_comp_tilt.input_type = SENSOR_TYPE_TILT_DETECTOR;
    input_comp_tilt.sampling_delay = ACTIVITY_INPUT_SAMPLE_DELAY * ACTIVITY_ACC_FIFO_NUM;

    input_comp_acc.next_input = &input_comp_tilt;
    input_comp_tilt.next_input = NULL;

    struct SensorDescriptor_t activity_desp = {
        SENSOR_TYPE_ACTIVITY, 1, continus, {0, 0},
        &input_comp_acc, activity_operate, run_activity,
        set_activity_data, ACTIVITY_INPUT_ACCUMULATE
    };

    ret = sensor_subsys_algorithm_register_type(&activity_desp);
    if (ret < 0) {
        LOGE("fail to register activity \r\n");
    }

    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_ACTIVITY, 1);
    if (ret < 0) {
        LOGE("fail to register buffer for activity \r\n");
    }
    return ret;
}

int activity_init(void)
{
    uint32_t init_time = read_xgpt_stamp_ns() / 1000000;

    act_last_state = UNKNOWN;
    act_temp_state = UNKNOWN;
    act_output.still = act_output.cycle = act_output.walk = act_output.run = \
                                          act_output.stairs = act_output.veh = act_output.tilt = act_output.unknown = 0;

    // init activity_adaptor struct
    activity_adaptor.notifity_data_count    = 0;
    activity_adaptor.time_stamp             = 0;
    activity_adaptor.prev_set_data_time     = 0;
    activity_adaptor.set_data_timing        = ACTIVITY_FEATURE_UPDATE_TIME;
    activity_adaptor.input_state            = INPUT_STATE_INIT;
    activity_adaptor.last_tilt_time         = 0;
    activity_adaptor.tilt_dominate_activity = 0;
    activity_adaptor.tilt_start_unknown     = 0;
    //activity_adaptor.tilt_data_ptr          = NULL;
    activity_adaptor.tilt_event_state       = 0;

    activity_acc_resampling.init_flag = 0;
    activity_acc_resampling.input_sample_delay = ACTIVITY_INPUT_SAMPLE_DELAY;

    activity_algorithm_init(init_time);
#ifdef _PC_VERSION_DEBUG_
    printf("feature update timing = %d\n", activity_adaptor.set_data_timing);
#endif

    activity_register();
    return 1;
}
#ifdef _EVEREST_MODULE_DECLARE_
MODULE_DECLARE(virt_act_init, MOD_VIRT_SENSOR, activity_init);
#endif
