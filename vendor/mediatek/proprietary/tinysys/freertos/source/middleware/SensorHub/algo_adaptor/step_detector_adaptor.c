#include "sensors.h"
#include "stdio.h"
#include "pedometer.h"
#include "algo_adaptor.h"
#include "sensor_manager.h"

#define LOGE(fmt, args...)    printf("[step_detector] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    printf("[step_detector] DBG: "fmt, ##args)


struct step_detector_adaptor_t {
    uint32_t time_stamp;
    uint64_t time_stamp_ns;
};

static struct step_detector_adaptor_t step_detector_adaptor = {0, 0};

static struct input_list_t input_comp;

static INT32 run_step_detector(struct data_t * const output)
{
    //calculate output;
    struct data_unit_t *step_detector_data = output->data;
    output->data_exist_count = 1;
    step_detector_data->sensor_type = SENSOR_TYPE_STEP_DETECTOR;
    //step_detector_data->time_stamp = step_detector_adaptor.time_stamp_ns;
    step_detector_data->time_stamp = read_xgpt_stamp_ns();
    step_detector_data->step_detector_t.step_detect = get_step_detector_result();
    return 1;
}

static INT32 set_step_detector_data(const struct data_t *input_list, void *reserve)
{
    step_detector_adaptor.time_stamp_ns = input_list->data->time_stamp;
    if (get_step_detector_result()) {
        sensor_subsys_algorithm_notify(SENSOR_TYPE_STEP_DETECTOR);
        return 1;
    } else {
        return 0;
    }
}

static INT32 step_detector_operate(Sensor_Command command, void* buffer_out, INT32 size_out, \
                                   void* buffer_in, INT32 size_in)
{
    return 0;
}



int step_detector_register(void)
{
    int ret;//return: fail=-1, pass>=0, which means the count of current register algorithm

    input_comp.input_type = SENSOR_TYPE_PEDOMETER;
    input_comp.sampling_delay = PEDOMETER_INPUT_SAMPLE_INTERVAL * PEDOMETER_ACC_FIFO_NUM;
    input_comp.next_input = NULL;

    struct SensorDescriptor_t step_detector_desp = {
        SENSOR_TYPE_STEP_DETECTOR, 1, one_shot, {0, 0},
        &input_comp, step_detector_operate, run_step_detector, set_step_detector_data, 250
    };

    ret = sensor_subsys_algorithm_register_type(&step_detector_desp);
    if (ret < 0) {
        LOGE("fail to register Pedometer \r\n");
    }
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_STEP_DETECTOR, 1);
    if (ret < 0) {
        LOGE("fail to register buffer \r\n");
    }
    return ret;
}

int step_detector_init(void)
{
    step_detector_register();
    return 1;
}
#ifdef _EVEREST_MODULE_DECLARE_
MODULE_DECLARE(virt_step_detector_init, MOD_VIRT_SENSOR, step_detector_init);
#endif