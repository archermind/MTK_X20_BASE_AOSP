#include "sensors.h"
#include "stdio.h"
#include "pedometer.h"
#include "algo_adaptor.h"
#include "sensor_manager.h"

#define LOGE(fmt, args...)    printf("[step_counter] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    printf("[step_counter] DBG: "fmt, ##args)

struct step_counter_adaptor_t {
    uint32_t time_stamp;
    uint64_t time_stamp_ns;
};

static struct step_counter_adaptor_t step_counter_adaptor = {0, 0};

static INT32 run_step_counter(struct data_t * const output)
{
    //calculate output;
    struct data_unit_t *step_counter_data = output->data;
    output->data_exist_count = 1;
    step_counter_data->sensor_type = SENSOR_TYPE_STEP_COUNTER;
    //step_counter_data->time_stamp = step_counter_adaptor.time_stamp_ns;
    step_counter_data->time_stamp = read_xgpt_stamp_ns();
    step_counter_data->step_counter_t.accumulated_step_count = get_pedometer_acc_step_count();
    return 1;
}

static INT32 set_step_counter_data(const struct data_t *input_list, void *reserve)
{
    static int last_total_step_count = 0;
    step_counter_adaptor.time_stamp_ns = input_list->data->time_stamp;
    if (last_total_step_count < get_pedometer_acc_step_count()) {
        last_total_step_count = get_pedometer_acc_step_count();
        return 1;
    } else {
        return 0;
    }
}

static INT32 step_counter_operate(Sensor_Command command, void* buffer_out, INT32 size_out, \
                                  void* buffer_in, INT32 size_in)
{
    return 0;
}

int step_counter_register(void)
{
    int ret;//return: fail=-1, pass>=0, which means the count of current register algorithm
    struct SensorDescriptor_t  step_counter_desp;
    struct input_list_t pedo_list;

    step_counter_desp.sensor_type = SENSOR_TYPE_STEP_COUNTER;
    step_counter_desp.version =  1;
    step_counter_desp.report_mode = on_change;
    step_counter_desp.hw.max_sampling_rate = 0;
    step_counter_desp.hw.support_HW_FIFO = 0;

    step_counter_desp.input_list = &pedo_list;
    pedo_list.input_type = SENSOR_TYPE_PEDOMETER;
    pedo_list.sampling_delay = PEDOMETER_INPUT_SAMPLE_INTERVAL * PEDOMETER_ACC_FIFO_NUM;
    pedo_list.next_input = NULL;

    step_counter_desp.operate = step_counter_operate;
    step_counter_desp.run_algorithm = run_step_counter;
    step_counter_desp.set_data = set_step_counter_data;

    step_counter_desp.accumulate = 250;

    ret = sensor_subsys_algorithm_register_type(&step_counter_desp);
    if (ret < 0) {
        LOGE("fail to register Pedometer \r\n");
    }
    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_STEP_COUNTER, 1);
    if (ret < 0) {
        LOGE("fail to register buffer \r\n");
    }
    return ret;
}

int step_counter_init(void)
{
    step_counter_register();
    return 1;
}
#ifdef _EVEREST_MODULE_DECLARE_
MODULE_DECLARE(virt_step_counter_init, MOD_VIRT_SENSOR, step_counter_init);
#endif