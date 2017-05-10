#include "sensors.h"
#include "stdio.h"
#include "init_learning.h"
#include "gesture.h"
#include "signal.h"
#include "algo_adaptor.h"
#include "sensor_manager.h"

//#define _GESTURE_DEBUG_

#define LOGE(fmt, args...)    printf("[Gesture] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    printf("[Gesture] DBG: "fmt, ##args)

struct input_list_t input_comp_acc;
struct input_list_t input_comp_pro;
static resampling_t gesture_resampling = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const gesture_initializer_t enable_gesture_alg = {1, 1, 1, 1, 1};
static gesture_initializer_t start_notify = {0, 0, 0, 0, 0};
static int start_notify_freefall = 0;
//#define _PC_VERSION_DEBUG_

struct gesture_adaptor_t {
    uint8_t flip_notifity_data_count;
    uint8_t snapshot_notifity_data_count;
    uint8_t pickup_notifity_data_count;
    uint8_t shake_notifity_data_count;
    uint8_t answercall_notifity_data_count;
    uint8_t freefall_notifity_data_count;

    uint32_t time_stamp;
    uint64_t time_stamp_ns;
    uint32_t prox_time_stamp;
    uint32_t prev_set_data_time;
    uint32_t set_data_timing;
    int input_state;
};

int get_start_notify_freefall()
{
    return start_notify_freefall;
}

static struct gesture_adaptor_t gesture_adaptor;

uint32_t get_gesture_time_stamp()
{
    return gesture_adaptor.time_stamp;
}


static uint8_t check_notify_or_not(gesture_notify_result_t *results, uint8_t *notify_count)
{
    if (*notify_count < GESTURE_NOTIFY_DATA_NUM) {
        *notify_count += 1;
        return 0;
    } else {
        if ((results->last_result <= results->threshold) && \
                (results->current_result > results->threshold)) {
            *notify_count = 0;
            return 1;
        } else {
            return 0;
        }
    }
}

static INT32 run_gesture_shake(struct data_t* const output)
{
    UINT8 output_prob = 0;
    gesture_get_alg_result(&output_prob, SHAKE);
    output->data_exist_count = 1;
    output->data->time_stamp = gesture_adaptor.time_stamp_ns;
    output->data->sensor_type = SENSOR_TYPE_SHAKE;
    output->data->gesture_data_t.probability = output_prob;

#ifdef _GESTURE_DEBUG_
    PRINTF_D("[GES_O]%d, %d\n", SENSOR_TYPE_SHAKE, output_prob);
#endif
    return 1;
}


static INT32 run_gesture_flip(struct data_t* const output)
{
    UINT8 output_prob = 0;
    gesture_get_alg_result(&output_prob, FLIP);
    output->data_exist_count = 1;
    output->data->time_stamp = gesture_adaptor.time_stamp_ns;
    output->data->sensor_type = SENSOR_TYPE_FLIP;
    output->data->gesture_data_t.probability = output_prob;

#ifdef _GESTURE_DEBUG_
    PRINTF_D("[GES_O]%d, %d\n", SENSOR_TYPE_FLIP, output_prob);
#endif
    return 1;
}

static INT32 run_gesture_snapshot(struct data_t* const output)
{
    UINT8 output_prob = 0;
    gesture_get_alg_result(&output_prob, SNAPSHOT);
    output->data_exist_count = 1;
    output->data->time_stamp = gesture_adaptor.time_stamp_ns;
    output->data->sensor_type = SENSOR_TYPE_SNAPSHOT;
    output->data->gesture_data_t.probability = output_prob;

#ifdef _GESTURE_DEBUG_
    PRINTF_D("[GES_O]%d, %d\n", SENSOR_TYPE_SNAPSHOT, output_prob);
#endif
    return 1;
}

static INT32 run_gesture_pick_up(struct data_t* const output)
{
    UINT8 output_prob = 0;
    gesture_get_alg_result(&output_prob, PICKUP);
    output->data_exist_count = 1;
    output->data->time_stamp = gesture_adaptor.time_stamp_ns;
    output->data->sensor_type = SENSOR_TYPE_PICK_UP;
    output->data->gesture_data_t.probability = output_prob;

#ifdef _GESTURE_DEBUG_
    PRINTF_D("[GES_O]%d, %d\n", SENSOR_TYPE_PICK_UP, output_prob);
#endif
    return 1;
}

static INT32 run_gesture_freefall(struct data_t* const output)
{
    UINT8 output_prob = 0;
    gesture_get_alg_result(&output_prob, FREEFALL);
    output->data_exist_count = 1;
    output->data->time_stamp = gesture_adaptor.time_stamp_ns;
    output->data->sensor_type = SENSOR_TYPE_FREEFALL;
    output->data->gesture_data_t.probability = output_prob;
    output->data->fall_data_t.probability[FALL] = output_prob;

#ifdef _GESTURE_DEBUG_
    PRINTF_D("[GES_O]%d, %d\n", SENSOR_TYPE_FREEFALL, output_prob);
#endif
    return 1;
}

static INT32 run_gesture_answercall(struct data_t* const output)
{
    UINT8 output_prob = 0;
    gesture_get_alg_result(&output_prob, ANSWERCALL);
    output->data_exist_count = 1;
    output->data->time_stamp = gesture_adaptor.time_stamp_ns;
    output->data->sensor_type = SENSOR_TYPE_ANSWER_CALL;
    output->data->gesture_data_t.probability = output_prob;

#ifdef _GESTURE_DEBUG_
    PRINTF_D("[GES_O]%d, %d\n", SENSOR_TYPE_ANSWER_CALL, output_prob);
#endif
    return 1;
}
static void run_gesture_algorithm(uint32_t input_time_stamp_ms)
{
    gesture_adaptor.time_stamp = input_time_stamp_ms;

    if (gesture_adaptor.input_state == INPUT_STATE_INIT) {
        gesture_adaptor.prev_set_data_time = input_time_stamp_ms;
        gesture_adaptor.input_state = INPUT_STATE_PREPARE_DATA;
    }

    INT32 time_diff;
    time_diff = input_time_stamp_ms - gesture_adaptor.prev_set_data_time;

    if (((gesture_adaptor.input_state == INPUT_STATE_PREPARE_DATA) && \
            (time_diff >= MIN_SAMPLES_TIME)) ||
            ((gesture_adaptor.input_state == INPUT_STATE_UPDATE) && \
             (time_diff >= gesture_adaptor.set_data_timing))) {
#ifdef _PC_VERSION_DEBUG_
        printf("time diff = %d, time = %u, prev_time = %u, ", time_diff, input_time_stamp_ms,
               gesture_adaptor.prev_set_data_time);
#endif
        gesture_adaptor.input_state = INPUT_STATE_UPDATE;
        gesture_adaptor.prev_set_data_time = input_time_stamp_ms;
        gesutre_alg_enter_point();

        gesture_notify_result_t gesture_results;
        gesture_get_result_ptr(&gesture_results, FLIP);
        if (start_notify.enable_flip && check_notify_or_not(&gesture_results, &gesture_adaptor.flip_notifity_data_count)) {
#ifndef _PC_VERSION_
            PRINTF_D("flip notify");
#endif
            sensor_subsys_algorithm_notify(SENSOR_TYPE_FLIP);
        }
        gesture_get_result_ptr(&gesture_results, SNAPSHOT);
        if (start_notify.enable_snapshot
                && check_notify_or_not(&gesture_results, &gesture_adaptor.snapshot_notifity_data_count)) {
#ifndef _PC_VERSION_
            PRINTF_D("snapshot notify");
#endif
            sensor_subsys_algorithm_notify(SENSOR_TYPE_SNAPSHOT);
        }
        gesture_get_result_ptr(&gesture_results, PICKUP);
        if (start_notify.enable_pick_up && check_notify_or_not(&gesture_results, &gesture_adaptor.pickup_notifity_data_count)) {
#ifndef _PC_VERSION_
            PRINTF_D("pick notify");
#endif
            sensor_subsys_algorithm_notify(SENSOR_TYPE_PICK_UP);
        }
        gesture_get_result_ptr(&gesture_results, SHAKE);
        if (start_notify.enable_shake && check_notify_or_not(&gesture_results, &gesture_adaptor.shake_notifity_data_count)) {
#ifndef _PC_VERSION_
            PRINTF_D("shake notify");
#endif
            sensor_subsys_algorithm_notify(SENSOR_TYPE_SHAKE);
        }
        gesture_get_result_ptr(&gesture_results, ANSWERCALL);
        if (start_notify.enable_answercall
                && check_notify_or_not(&gesture_results, &gesture_adaptor.answercall_notifity_data_count)) {
#ifndef _PC_VERSION_
            PRINTF_D("answer call notify");
#endif
            sensor_subsys_algorithm_notify(SENSOR_TYPE_ANSWER_CALL);
        }
        gesture_get_result_ptr(&gesture_results, FREEFALL);
        if (start_notify_freefall && check_notify_or_not(&gesture_results, &gesture_adaptor.freefall_notifity_data_count)) {
#ifndef _PC_VERSION_
            PRINTF_D("freefall notify");
#endif
            sensor_subsys_algorithm_notify(SENSOR_TYPE_FREEFALL);
        }
    }
}

static INT32 set_gesture_data(const struct data_t *input_list, void *reserve)
{
    uint32_t input_time_stamp_ms = input_list->data->time_stamp / 1000000; //input time stamp (ms)
    gesture_adaptor.time_stamp_ns = input_list->data->time_stamp;  // input time stamp (ns)
    struct data_unit_t *data_start = input_list->data;
    int count = input_list->data_exist_count;

    if (input_list->data->sensor_type == SENSOR_TYPE_ACCELEROMETER) {

        int32_t same_data = check_if_same_input(data_start, gesture_resampling.last_time_stamp, count);

        if (same_data) {
            return 0;
        }
        if (!gesture_resampling.init_flag) {
            gesture_resampling.last_time_stamp = input_time_stamp_ms;
            gesture_resampling.init_flag = 1;
            return 0;
        }
        Activity_Accelerometer_Ms2_Data_t acc_input = {0};
        while (count != 0) {
            input_time_stamp_ms = data_start->time_stamp / 1000000; //input time stamp (ms)
#ifndef _PC_VERSION_
            PRINTF_D("[GES_Acc_I]%u, %d, %d, %d, %d\n", input_time_stamp_ms, count, \
                     data_start->accelerometer_t.x, data_start->accelerometer_t.y, data_start->accelerometer_t.z);
#endif
            // prepare input for floating input
            // resampling
            sensor_vec_t *data_acc_t = &data_start->accelerometer_t;

            gesture_resampling.current_time_stamp = input_time_stamp_ms;
            sensor_subsys_algorithm_resampling_type(&gesture_resampling);

            acc_input.time_stamp = gesture_resampling.last_time_stamp;
            acc_input.x = data_acc_t->x;
            acc_input.y = data_acc_t->y;
            acc_input.z = data_acc_t->z;

            while (gesture_resampling.input_count > 0) {
                acc_input.time_stamp = acc_input.time_stamp + gesture_resampling.input_sample_delay;
                get_signal_acc(acc_input); /* using address acc_buff to update acc measurements */
                gesture_set_timestamp(acc_input.time_stamp);
                run_gesture_algorithm(acc_input.time_stamp);
                gesture_resampling.input_count--;
                gesture_resampling.last_time_stamp = gesture_resampling.current_time_stamp;
#ifdef _GESTURE_DEBUG_
                PRINTF_D("[G_Alo_In]%u, %u, %d, %d, %d\n", acc_input.time_stamp, gesture_resampling.last_time_stamp, data_acc_t->x,
                         data_acc_t->y, data_acc_t->z);
#endif
            }
            data_start++;
            count--;
        }
    }

    if (input_list->data->sensor_type == SENSOR_TYPE_PROXIMITY) {
        gesture_set_prox(data_start->proximity_t.oneshot);
        return 1;
    }


    return 1;
}

static INT32 gesture_operate(Sensor_Command command, void* buffer_in, INT32 size_in, int *start_notify_t)
{
    int err = 0;
    int value = 0;
    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            //PRINTF_D("SMD ACTIVATE\n\r");
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PRINTF_D("Enable gesture sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    //disable SMD
                    *start_notify_t = 0;
                    if ((!start_notify_freefall) && (!start_notify.enable_shake) &&
                            (!start_notify.enable_flip) && (!start_notify.enable_pick_up) &&
                            (!start_notify.enable_snapshot) && (!start_notify.enable_answercall)) {
                        gesture_resampling.init_flag = 0;
                    }
                } else {
                    *start_notify_t = 1;
                }
            }
            break;
        default:
            break;

    }
    return err;
}

static INT32 gesture_freefall_operate(Sensor_Command command, void* buffer_in, INT32 size_in, \
                                      void* buffer_out, INT32 size_out)
{
    int err = 0;
    err = gesture_operate(command, buffer_in, size_in, &start_notify_freefall);
#ifndef _PC_VERSION_
    PRINTF_D("[set freefall]%d\n", start_notify_freefall);
#endif
    return err;
}

static INT32 gesture_shake_operate(Sensor_Command command, void* buffer_in, INT32 size_in, \
                                   void* buffer_out, INT32 size_out)
{
    int err = 0;
    err = gesture_operate(command, buffer_in, size_in, &start_notify.enable_shake);
#ifndef _PC_VERSION_
    PRINTF_D("[set shake]%d\n", start_notify.enable_shake);
#endif
    return err;
}

static INT32 gesture_flip_operate(Sensor_Command command, void* buffer_in, INT32 size_in, \
                                  void* buffer_out, INT32 size_out)
{
    int err = 0;
    err = gesture_operate(command, buffer_in, size_in, &start_notify.enable_flip);
#ifndef _PC_VERSION_
    PRINTF_D("[set flip]%d\n", start_notify.enable_flip);
#endif
    return err;
}

static INT32 gesture_pick_up_operate(Sensor_Command command, void* buffer_in, INT32 size_in, \
                                     void* buffer_out, INT32 size_out)
{
    int err = 0;
    err = gesture_operate(command, buffer_in, size_in, &start_notify.enable_pick_up);
#ifndef _PC_VERSION_
    PRINTF_D("[set pickup]%d\n", start_notify.enable_pick_up);
#endif
    return err;
}

static INT32 gesture_snapshot_operate(Sensor_Command command, void* buffer_in, INT32 size_in, \
                                      void* buffer_out, INT32 size_out)
{
    int err = 0;
    err = gesture_operate(command, buffer_in, size_in, &start_notify.enable_snapshot);
#ifndef _PC_VERSION_
    PRINTF_D("[set snapshot]%d\n", start_notify.enable_snapshot);
#endif
    return err;
}

static INT32 gesture_answercall_operate(Sensor_Command command, void* buffer_in, INT32 size_in, \
                                        void* buffer_out, INT32 size_out)
{
    int err = 0;
    err = gesture_operate(command, buffer_in, size_in, &start_notify.enable_answercall);
#ifndef _PC_VERSION_
    PRINTF_D("[set answer call]%d\n", start_notify.enable_answercall);
#endif
    return err;

}

// freefall part
void set_common_gesture_input_comp()
{
    input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
    input_comp_acc.sampling_delay = GESTURE_INPUT_SAMPLE_DELAY * GESTURE_ACC_FIFO_NUM;
    input_comp_acc.next_input = NULL;
}

// this descriptor will share to freefall_adaptor
struct SensorDescriptor_t gesture_freefall_desp = {
    SENSOR_TYPE_FREEFALL, 1, one_shot, {20, 0},
    &input_comp_acc, gesture_freefall_operate, run_gesture_freefall, set_gesture_data,
    GESTURE_INPUT_ACCUMULATE
};

struct SensorDescriptor_t* get_freefall_ptr()
{
    return &gesture_freefall_desp;
}

static int gesture_register()
{
    int ret = 0;//return: fail=-1, pass>=0, which means the count of current register algorithm

    if (enable_gesture_alg.enable_shake) {
        set_common_gesture_input_comp();
        struct SensorDescriptor_t gesture_shake_desp = {
            SENSOR_TYPE_SHAKE, 1, one_shot, {20, 0},
            &input_comp_acc, gesture_shake_operate, run_gesture_shake, set_gesture_data,
            GESTURE_INPUT_ACCUMULATE
        };

        ret = sensor_subsys_algorithm_register_type(&gesture_shake_desp);
        if (ret < 0)
            LOGE("fail to register Gesture shake \r\n");
        ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_SHAKE, 1);
        if (ret < 0)
            LOGE("fail to register buffer for shake \r\n");
    }
    if (enable_gesture_alg.enable_flip) {
        set_common_gesture_input_comp();
        struct SensorDescriptor_t gesture_flip_desp = {
            SENSOR_TYPE_FLIP, 1, one_shot, {20, 0},
            &input_comp_acc, gesture_flip_operate, run_gesture_flip, set_gesture_data,
            GESTURE_INPUT_ACCUMULATE
        };
        ret = sensor_subsys_algorithm_register_type(&gesture_flip_desp);
        if (ret < 0)
            LOGE("fail to register Gesture flip \r\n");
        ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_FLIP, 1);
        if (ret < 0)
            LOGE("fail to register buffer for flip \r\n");
    }
    if (enable_gesture_alg.enable_snapshot) {
        set_common_gesture_input_comp();
        struct SensorDescriptor_t gesture_snapshot_desp = {
            SENSOR_TYPE_SNAPSHOT, 1, one_shot, {20, 0},
            &input_comp_acc, gesture_snapshot_operate, run_gesture_snapshot, set_gesture_data,
            GESTURE_INPUT_ACCUMULATE
        };
        ret = sensor_subsys_algorithm_register_type(&gesture_snapshot_desp);
        if (ret < 0)
            LOGE("fail to register Gesture snapshot \r\n");
        ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_SNAPSHOT, 1);
        if (ret < 0)
            LOGE("fail to register buffer for snapshot \r\n");
    }
    if (enable_gesture_alg.enable_pick_up) {
        set_common_gesture_input_comp();
        struct SensorDescriptor_t gesture_pick_up_desp = {
            SENSOR_TYPE_PICK_UP, 1, one_shot, {20, 0},
            &input_comp_acc, gesture_pick_up_operate, run_gesture_pick_up, set_gesture_data,
            GESTURE_INPUT_ACCUMULATE
        };
        ret = sensor_subsys_algorithm_register_type(&gesture_pick_up_desp);
        if (ret < 0)
            LOGE("fail to register Gesture pick_up \r\n");
        ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_PICK_UP, 1);
        if (ret < 0)
            LOGE("fail to register buffer for pick_up \r\n");
    }

    if (enable_gesture_alg.enable_answercall && enable_gesture_alg.enable_pick_up) {
        input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
        input_comp_acc.sampling_delay = GESTURE_INPUT_SAMPLE_DELAY * GESTURE_ACC_FIFO_NUM;
        input_comp_acc.next_input = &input_comp_pro;
        input_comp_pro.input_type = SENSOR_TYPE_PROXIMITY;
        //input_comp_pro.sampling_delay = GESTURE_INPUT_SAMPLE_DELAY;
        input_comp_pro.sampling_delay = GESTURE_INPUT_SAMPLE_DELAY * GESTURE_ACC_FIFO_NUM;
        input_comp_pro.next_input = NULL;
        struct SensorDescriptor_t gesture_answercall_desp = {
            SENSOR_TYPE_ANSWER_CALL, 1, one_shot, {20, 0},
            &input_comp_acc, gesture_answercall_operate, run_gesture_answercall, set_gesture_data,
            GESTURE_INPUT_ACCUMULATE
        };

        ret = sensor_subsys_algorithm_register_type(&gesture_answercall_desp);
        if (ret < 0)
            LOGE("fail to register Gesture answercall \r\n");
        ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_ANSWER_CALL, 1);
        if (ret < 0)
            LOGE("fail to register buffer for answercall \r\n");
    }

    return ret;
}

int gesture_init(void)
{
    uint32_t init_time = (uint32_t)(read_xgpt_stamp_ns() / 1000000);

    gesture_algorithm_init(&enable_gesture_alg, init_time);

    gesture_adaptor.set_data_timing = GESTURE_FEATURE_UPDATE_TIME;
    gesture_adaptor.prev_set_data_time = 0;
    gesture_adaptor.time_stamp = 0;
    gesture_adaptor.input_state = INPUT_STATE_INIT;
    gesture_adaptor.flip_notifity_data_count       = GESTURE_NOTIFY_DATA_NUM + 1;
    gesture_adaptor.snapshot_notifity_data_count   = GESTURE_NOTIFY_DATA_NUM + 1;
    gesture_adaptor.pickup_notifity_data_count     = GESTURE_NOTIFY_DATA_NUM + 1;
    gesture_adaptor.shake_notifity_data_count      = GESTURE_NOTIFY_DATA_NUM + 1;
    gesture_adaptor.answercall_notifity_data_count = GESTURE_NOTIFY_DATA_NUM + 1;

    gesture_resampling.init_flag = 0;
    gesture_resampling.input_sample_delay = GESTURE_INPUT_SAMPLE_DELAY;

#ifdef _PC_VERSION_DEBUG_
    printf("feature update timing = %d\n", gesture_adaptor.set_data_timing);
#endif

    gesture_register();
    return 1;
}

#ifdef _EVEREST_MODULE_DECLARE_
MODULE_DECLARE(virt_gesture_init, MOD_VIRT_SENSOR, gesture_init);
#endif


