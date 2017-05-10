#include "sensors.h"
#include "stdio.h"
#include "pedometer.h"
#include "algo_adaptor.h"
#include "sensor_manager.h"
#include "smd.h"

#define LOGE(fmt, args...)    printf("[smd] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    printf("[smd] DBG: "fmt, ##args)

struct smd_adaptor_t {
    uint32_t time_stamp;
    uint64_t time_stamp_ns;
};

static struct smd_adaptor_t smd_adaptor = {0, 0};
static struct input_list_t input_comp;
static int significant_motion = 0;
static int last_significant_motion = 0;

static INT32 run_smd(struct data_t * const output)
{
    //calculate output;
    output->data_exist_count = 1;
    output->data->sensor_type = SENSOR_TYPE_SIGNIFICANT_MOTION;
    output->data->time_stamp = smd_adaptor.time_stamp_ns;
    output->data->smd_t.state = significant_motion;
    return 1;
}

static INT32 set_smd_data(const struct data_t *input_list, void *reserve)
{
    smd_adaptor.time_stamp_ns = input_list->data->time_stamp;
    significant_motion = significant_motion_detection();
    PRINTF_D("smd:%d, %d\r\n", significant_motion, last_significant_motion);
    if ((!last_significant_motion) && (significant_motion)) {
        last_significant_motion = significant_motion;
        PRINTF_D("smd trigger\r\n");
        sensor_subsys_algorithm_notify(SENSOR_TYPE_SIGNIFICANT_MOTION);
    } else if (get_smd_still_count()) {
        last_significant_motion = significant_motion;
    }
    return 1;
}

static INT32 smd_operate(Sensor_Command command, void* buffer_in, INT32 size_in, \
                         void* buffer_out, INT32 size_out)
{
    int err = 0;
    int value = 0;
    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            PRINTF_D("SMD ACTIVATE: %d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PRINTF_D("Enable sensor parameter error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    //disable SMD
                } else {
                    PRINTF_D("SMD run re_init\n");
                    significant_init();
                }
            }
            break;
        default:
            break;
    }
    return err;
}



int smd_register(void)
{
    int ret;//return: fail=-1, pass>=0, which means the count of current register algorithm

    input_comp.input_type = SENSOR_TYPE_PEDOMETER;
    input_comp.sampling_delay = PEDOMETER_INPUT_SAMPLE_INTERVAL * PEDOMETER_ACC_FIFO_NUM;
    input_comp.next_input = NULL;

    struct SensorDescriptor_t smd_desp = {
        SENSOR_TYPE_SIGNIFICANT_MOTION, 1, one_shot, {20, 0},
        &input_comp, smd_operate, run_smd, set_smd_data, 250
    };

    ret = sensor_subsys_algorithm_register_type(&smd_desp);
    if (ret < 0) {
        LOGE("fail to register SMD \r\n");
    }

    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_SIGNIFICANT_MOTION, 1);
    if (ret < 0) {
        LOGE("fail to register buffer \r\n");
    }
    return ret;
}

int smd_init()
{
    significant_init();
    smd_register();
    significant_motion = 0;
    PRINTF_D("SMD run significant_init\n");
    return 1;
}
#ifdef _EVEREST_MODULE_DECLARE_
MODULE_DECLARE(virt_smd_init, MOD_VIRT_SENSOR, smd_init);
#endif