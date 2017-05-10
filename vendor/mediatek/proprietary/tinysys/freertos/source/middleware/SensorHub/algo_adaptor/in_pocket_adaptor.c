#include "sensors.h"
#include "stdio.h"
#include "feature_struct.h"
#include "algo_adaptor.h"
#include "sensor_manager.h"

#define LOGE(fmt, args...)    printf("[In Pocket] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    printf("[In Pocket] DBG: "fmt, ##args)

#define IN_POCKET_INPUT_SAMPLE_DELAY 20 // unit: ms
#define IN_POCKET_INPUT_ACCUMULATE  160 // unit: ms
struct input_list_t input_comp_prx;
static int start_notify_in_pocket = 0;

struct in_pocket_adaptor_t {
    uint32_t time_stamp;
    uint64_t time_stamp_ns;
    uint32_t curr_set_time;
    uint32_t last_set_time;
    int input_state; // init / prepare data / ready
    int inpk_state; // 1: detect inpk 0: not inpk
};

static struct in_pocket_adaptor_t in_pocket_adaptor = {
    .last_set_time = 0,
    .curr_set_time = 0,
    .input_state = INPUT_STATE_INIT,
};

static INT32 run_in_pocket(struct data_t * const output)
{
    output->data_exist_count = 1;
    output->data->time_stamp = in_pocket_adaptor.time_stamp_ns;
    output->data->sensor_type = SENSOR_TYPE_INPOCKET;
    output->data->inpocket_event.state = in_pocket_adaptor.inpk_state;
    return 1;
}

static INT32 set_in_pocket_data(const struct data_t *input_list, void *reserve)
{
    static int temp_inpk_state = 0;
    uint32_t input_time_stamp = input_list->data->time_stamp / 1000000;
    in_pocket_adaptor.time_stamp_ns = input_list->data->time_stamp;
    struct data_unit_t *data_start = input_list->data;

    if (input_list->data->sensor_type == SENSOR_TYPE_PROXIMITY) {
        in_pocket_adaptor.inpk_state = ((int)data_start->proximity_t.oneshot == 0) ? 1 : 0;
        if (start_notify_in_pocket && in_pocket_adaptor.inpk_state > 0 && temp_inpk_state == 0) {
            sensor_subsys_algorithm_notify(SENSOR_TYPE_PROXIMITY);
        }
        temp_inpk_state = in_pocket_adaptor.inpk_state;
    }

    if (in_pocket_adaptor.input_state == INPUT_STATE_INIT) {
        in_pocket_adaptor.last_set_time = input_time_stamp;
        in_pocket_adaptor.input_state = INPUT_STATE_PREPARE_DATA;
    }

    INT32 time_diff;
    time_diff = input_time_stamp - in_pocket_adaptor.last_set_time;

    if (((in_pocket_adaptor.input_state == INPUT_STATE_PREPARE_DATA) && (time_diff >= MIN_SAMPLES_TIME)) ||
            ((in_pocket_adaptor.input_state == INPUT_STATE_UPDATE) && (time_diff >= IN_POCKET_FEATURE_UPDATE_TIME))) {
        in_pocket_adaptor.input_state = INPUT_STATE_UPDATE;
        in_pocket_adaptor.last_set_time = input_time_stamp;
    }

    return 1;
}

static INT32 in_pocket_operate(Sensor_Command command, void* buffer_in, INT32 size_in, \
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
                value = *(int *)buffer_in;
                if (0 == value) {
                    //disable SMD
                    start_notify_in_pocket = 0;
                } else {
                    start_notify_in_pocket = 1;
                }
            }
            break;
        default:
            break;
    }
    return err;
}

static int in_pocket_register()
{
    int ret = 0;
    input_comp_prx.input_type = SENSOR_TYPE_PROXIMITY;
    input_comp_prx.sampling_delay = IN_POCKET_INPUT_SAMPLE_DELAY * ACC_EVENT_COUNT_PER_FIFO_LOOP;
    input_comp_prx.next_input = NULL;

    struct SensorDescriptor_t in_pocket_desp = {
        SENSOR_TYPE_INPOCKET, 1, one_shot, {20, 0},
        &input_comp_prx, in_pocket_operate, run_in_pocket,
        set_in_pocket_data, IN_POCKET_INPUT_ACCUMULATE
    };

    ret = sensor_subsys_algorithm_register_type(&in_pocket_desp);
    if (ret < 0)
        LOGE("fail to register in pocket. \r\n");

    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_INPOCKET, 1);
    if (ret < 0)
        LOGE("fail to register buffer for in pocket \r\n");
    return ret;
}

int in_pocket_init()
{
    in_pocket_register();
    return 1;
}
#ifdef _EVEREST_MODULE_DECLARE_
MODULE_DECLARE(virt_pkt_init, MOD_VIRT_SENSOR, in_pocket_init);
#endif


