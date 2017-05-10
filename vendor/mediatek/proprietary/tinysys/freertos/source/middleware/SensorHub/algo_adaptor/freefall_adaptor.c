#include "sensors.h"
#include "stdio.h"
#include "feature_struct.h"
#include "gesture.h"
#include "algo_adaptor.h"
#include "sensor_manager.h"

#ifdef _PC_VERSION_
#define LOGE(fmt, args...)    printf("[FREEFALL] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    printf("[FREEFALL] DBG: "fmt, ##args)
#else
#define LOGE(fmt, args...)    PRINTF_D("[FREEFALL] ERR: "fmt, ##args)
#define LOGD(fmt, args...)    PRINTF_D("[FREEFALL] DBG: "fmt, ##args)
#endif

static uint8_t freefall_notifity_data_count;
static struct SensorDescriptor_t* dummy_freefall_ptr;

static int freefall_register()
{
    int ret = 0;
    ret = sensor_subsys_algorithm_register_type(dummy_freefall_ptr);
    if (ret < 0)
        LOGE("fail to register freefall. \r\n");

    ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_FREEFALL, 1);
    if (ret < 0)
        LOGE("fail to register buffer for freefall \r\n");
    return ret;
}

// Note: set_data and run_algorithm are declared in gesture_adaptor.c

int freefall_init()
{
    set_common_gesture_input_comp();
    dummy_freefall_ptr = get_freefall_ptr();
    freefall_register();
    freefall_notifity_data_count = GESTURE_NOTIFY_DATA_NUM + 1;
    return 1;
}

#ifdef _EVEREST_MODULE_DECLARE_
MODULE_DECLARE(virt_freefall_init, MOD_VIRT_SENSOR, freefall_init);
#endif