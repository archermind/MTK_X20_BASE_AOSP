#ifndef __ALGO_ADAPTOR_H__
#define __ALGO_ADAPTOR_H__

#include "activity_algorithm.h"
#include "init_learning.h"
#include "sensors.h"
#include "FreeRTOS.h"
#include "sensor_manager.h"

#define _EVEREST_MODULE_DECLARE_
#define INPUT_STATE_INIT 0
#define INPUT_STATE_PREPARE_DATA 1
#define INPUT_STATE_UPDATE 2
#define INPUT_SAMPLE_DELAY_MARGIN 2

#define ACTIVITY_ACC_FIFO_NUM ACC_EVENT_COUNT_PER_FIFO_LOOP
#define TILT_ACC_FIFO_NUM ACC_EVENT_COUNT_PER_FIFO_LOOP
#define GESTURE_ACC_FIFO_NUM ACC_EVENT_COUNT_PER_FIFO_LOOP
#define PEDOMETER_ACC_FIFO_NUM ACC_EVENT_COUNT_PER_FIFO_LOOP

// #define ACTIVITY_NOTIFY_DATA_NUM 15
#define GESTURE_NOTIFY_DATA_NUM  25  // 2000/80 (2s)

#define TILT_HOLD_TIME 1000
#define UNKNOWN_HOLD_TIME 2000

#define GESTURE_ON 1
#define GESTURE_OFF 0

#define RADIUS_TO_DEGREE 180/3.14159265358979323846

#ifdef _PC_VERSION_
uint64_t timestamp_get_ns();
#endif

int activity_init(void);
int gesture_init(void);
int tilt_init();
int in_pocket_init();
int freefall_init();
int answer_call_init();

int pedometer_init();
int pedometer_register();
int smd_init();
int smd_register();
int step_detector_init();
int step_detector_register();
int step_counter_init();
int step_counter_register();

//mpe virtual sensor
int Orientation_register();
int Gravity_register() ;
int Rotation_vec_register();
int Geomag_vec_register();
int Game_rot_register();
int Linear_acc_register();
int PDR_register();
int PDR_init();

struct SensorDescriptor_t* get_freefall_ptr();
int get_start_notify_freefall();
void set_freefall_input_comp();

/** @brief The required imte information for fusion algorihm, used in fusion algorithm resampling. */
typedef struct resampling {
    uint32_t current_time_stamp; /* time stamp of the current sample*/
    uint32_t last_time_stamp; /* time stamp of the last sample*/
    uint32_t input_sample_delay; /* time interval between adjacent samples*/
    uint32_t init_flag; // 0: not init, 1: init
    uint32_t input_count;
    int32_t acc_x;
    int32_t acc_y;
    int32_t acc_z;
    uint32_t baro;
    uint32_t proximity;
} resampling_t;

void sensor_subsys_algorithm_resampling_type(resampling_t *resample);
int32_t check_if_same_input(const struct data_unit_t *data_start, uint32_t last_time_stamp, int32_t data_count);

#endif
