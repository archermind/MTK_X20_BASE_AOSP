/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include "shf_communicator.h"
#include "shf_configurator.h"
#include "shf_data_pool.h"
#include "shf_debug.h"
#include "shf_scheduler.h"
#include "shf_sensor.h"

#include "shf_process.h"
#include <sensor_manager_fw.h>
#include <sensor_queue.h>

#define ALLOCATE_POOL_SIZE (SHF_POOL_SIZE + 1)

uint8_t sensor_type_map[ALLOCATE_POOL_SIZE] = {
    0, //0

    0, // 1(acc)
    0, // 2
    0, // 3
    0, // 4
    0, //5

    0, //6(als)
    0, //7
    0, //8

    0, //9(ps)
    0, //10
    0, //11

    1, //12(clock)
    1, //13

    2, //14(pedometer)
    2, //15
    2, //16
    2, //17
    2, //18
    0, //19

    3, //20(activity)
    3, //21
    3, //22
    3, //23
    3, //24
    3, //25
    3, //26
    0, //27

    4, //28(inpocket)
    4, //29
    0, //30

    3, //31(mpa)
    3, //32
    3, //33
    0, //34

    5, //35(smd)
    5, //36
    0, //37

    6, //38(pickup)
    6, //39
    0, //40

    7, //41(facedown)
    7, //42
    0, //43

    8, //44(shake)
    8, //45
    0, //46

    0, //47(gesture)
    0, //48
    0, //49

    0, //50(audio_ptr)
    0, //51
    0, //52

    0, //53(noise_level)
    0, //54
    0, //55

    12, //56(free_fall)
    12, //57
    0, //58

    3, //59,walking
    3, //60,running
    3, //61,climbing
    0, //62,reserved for activity
    0, //63,reserved for activity

    13, //64,tap
    13, //65
    0, //66

    14, //67,twist
    14, //68
    0, //69

    15, //70,snapshot
    15, //71
    0, //72

    16, //73,pdr
    16, //74
    16, //75
    16, //76
    0, //77
};

#define SENSOR_TYPE_CLOCK (0xFF)
uint8_t sensor_type_array[] = {
    0,
    SENSOR_TYPE_CLOCK, // 1
    SENSOR_TYPE_PEDOMETER, // 2
    SENSOR_TYPE_ACTIVITY, // 3
    SENSOR_TYPE_INPOCKET, // 4
    SENSOR_TYPE_SIGNIFICANT_MOTION, // 5
    SENSOR_TYPE_PICK_UP, // 6
    SENSOR_TYPE_FLIP, // 7
    SENSOR_TYPE_SHAKE, // 8
    SENSOR_TYPE_STEP_DETECTOR, //9
    SENSOR_TYPE_STEP_COUNTER,  //10
    0,  //11,instead of noiselevel
    SENSOR_TYPE_FREEFALL,  //12
    SENSOR_TYPE_TAP, //13
    SENSOR_TYPE_TWIST, //14
    SENSOR_TYPE_SNAPSHOT, //15
    SENSOR_TYPE_PDR //16
};
#define SENSOR_TYPE_COUNT (sizeof(sensor_type_array)/sizeof(sensor_type_array[0]))
#define SENSOR_TYPE_BASE                (1)

#define STATE_BIT_DATA_SCP              (0)
#define STATE_BIT_DATA_AP               (1)
#define STATE_BIT_DATA_SCP_PREVIOUS     (2)
#define STATE_BIT_CHANGE_SCP            (4)
#define STATE_BIT_CHANGE_AP             (5)

#define STATE_MASK_DATA_SCP             (1<<STATE_BIT_DATA_SCP)
#define STATE_MASK_DATA_AP              (1<<STATE_BIT_DATA_AP)
#define STATE_MASK_DATA_SCP_PREVIOUS    (1<<STATE_BIT_DATA_SCP_PREVIOUS)
#define STATE_MASK_DATA                 (STATE_MASK_DATA_SCP|STATE_MASK_DATA_AP)
#define STATE_MASK_CHANGE_SCP           (1<<STATE_BIT_CHANGE_SCP)
#define STATE_MASK_CHANGE_AP            (1<<STATE_BIT_CHANGE_AP)
#define STATE_MASK_CHANGE               (STATE_MASK_CHANGE_SCP|STATE_MASK_CHANGE_AP)

bool_t sensor_state_changed = FALSE;

uint8_t sensor_state[SENSOR_TYPE_COUNT];
int handle;

void shf_sensor_collect_scp_state(shf_data_index_t data_index, uint8_t enable_disable)
{
    uint8_t old_state;
    uint8_t changed;
    uint8_t sensor_index = sensor_type_map[data_index];
    if (0 == sensor_index) {
        return;
    }
    old_state = ((sensor_state[sensor_index]&STATE_MASK_DATA_SCP)>>STATE_BIT_DATA_SCP);
    changed = ((sensor_state[sensor_index]&STATE_MASK_CHANGE_SCP)>>STATE_BIT_CHANGE_SCP);
    logv("s_scp: dindex=%d, sensor=%d, state1=0x%x, old=%d, change=%d, enable=%d\n",
         data_index, sensor_type_array[sensor_index], sensor_state[sensor_index], old_state, changed, enable_disable);
    enable_disable = (enable_disable > 0);
    if (enable_disable != old_state) {
        if (changed) {
            sensor_state[sensor_index] |= (enable_disable<<STATE_BIT_DATA_SCP);
        } else {
            sensor_state[sensor_index] &= ~STATE_MASK_DATA_SCP;
            sensor_state[sensor_index] |= ((enable_disable<<STATE_BIT_DATA_SCP)|(1<<STATE_BIT_CHANGE_SCP));
        }
        sensor_state_changed = TRUE;
    } else if (enable_disable) { // for the special case: orig is 1, inputs are 1... 0
        sensor_state[sensor_index] |= (1<<STATE_BIT_CHANGE_SCP);
    }
    logv("s_scp: sensor=%d, state2=0x%x\n", sensor_type_array[sensor_index], sensor_state[sensor_index]);
}

void shf_sensor_set_delay(int sensor_type)
{
    int delay =0;
    struct CM4TaskRequestStruct delay_cm4= {0};
    delay_cm4.handle = handle;
    delay_cm4.sensor_type = sensor_type;
    delay_cm4.command = SENSOR_SETDELAY_CMD;
    switch(sensor_type) {
        case SENSOR_TYPE_PEDOMETER:
        case SENSOR_TYPE_FLIP:
        case SENSOR_TYPE_INPOCKET:
        case SENSOR_TYPE_SHAKE:
        case SENSOR_TYPE_PICK_UP:
        case SENSOR_TYPE_TAP:
        case SENSOR_TYPE_TWIST:
        case SENSOR_TYPE_SNAPSHOT:
        case SENSOR_TYPE_FREEFALL:
            delay = 20;
            break;
        case SENSOR_TYPE_PDR:
            delay = 200;
            break;
        case SENSOR_TYPE_ACTIVITY:
            delay = 160;
            break;
        default:
            break;
    }
    delay_cm4.value = delay;
    SCP_Sensor_Manager_control(&delay_cm4);
}

void shf_sensor_run_algo(int type)
{

    struct CM4TaskRequestStruct poll_cm4= {0};
    poll_cm4.handle = handle;
    poll_cm4.sensor_type = type;
    poll_cm4.command = SENSOR_POLL_CMD;
    poll_cm4.value = 0;
    SCP_Sensor_Manager_control(&poll_cm4);

}

void shf_sensor_config()
{
    uint8_t all_deactivate = 1;
    uint8_t sensor_index = 0;
    uint8_t scp_previous;
    int32_t enable_disable = 1;

    logv("s_cf: changed=%d\n", sensor_state_changed);

    if (!sensor_state_changed) {
        return;
    }

    for (sensor_index = SENSOR_TYPE_BASE; sensor_index < SENSOR_TYPE_COUNT; sensor_index++) {
        logv("s_cf: sensor[%d]=%d, state=%x\n",
             sensor_index, sensor_type_array[sensor_index], sensor_state[sensor_index]);
        if (sensor_state[sensor_index]&STATE_MASK_CHANGE) {
            if (all_deactivate && 0 != (sensor_state[sensor_index]&STATE_MASK_DATA)) {
                all_deactivate = 0;
            }
            if (sensor_state[sensor_index]&STATE_MASK_CHANGE_SCP) {
                enable_disable = ((sensor_state[sensor_index]&STATE_MASK_DATA_SCP)>>STATE_BIT_DATA_SCP);
                scp_previous = ((sensor_state[sensor_index]&STATE_MASK_DATA_SCP_PREVIOUS)>>STATE_BIT_DATA_SCP_PREVIOUS);
                if (enable_disable != scp_previous) {
                    //open/close sensor
                    struct CM4TaskRequestStruct request_cm4= {0};
                    request_cm4.handle = handle;
                    request_cm4.sensor_type = sensor_type_array[sensor_index];
                    request_cm4.command = SENSOR_ACTIVATE_CMD;
                    request_cm4.value = enable_disable;
                    SCP_Sensor_Manager_control(&request_cm4);
                    shf_sensor_set_delay(sensor_type_array[sensor_index]);

                    sensor_state[sensor_index] &= ~STATE_MASK_DATA_SCP_PREVIOUS;
                    sensor_state[sensor_index] |= (enable_disable<<STATE_BIT_DATA_SCP_PREVIOUS);
                    logd("s_cf: %d %x\n", sensor_type_array[sensor_index], sensor_state[sensor_index]);
                }
                logd("s_cf:e_d=%d,type=%d\n",enable_disable,sensor_type_array[sensor_index]);
                //if enable & pdca flow set,if this shf_sensor_config run,must pdca flow set
                if(enable_disable) {
                    shf_process_run(sensor_type_array[sensor_index]);
                }

            }
            enable_disable = ((sensor_state[sensor_index]&STATE_MASK_DATA) != 0);

            sensor_state[sensor_index] &= (~STATE_MASK_CHANGE);//clear change bits
        }
    }
    sensor_state_changed = FALSE;
    shf_scheduler_set_state(all_deactivate!=1);
    logv("s_cf<<<\n");
}

void set_configrable_gesture(int g_index, int cgesture, int command)
{
    int sensor_gesture = sensor_type_map[g_index];
    logd("s_c_g:g=%d,cg=%d,cid=%d\n",sensor_gesture,cgesture,command);
    struct CM4TaskRequestStruct s_gesture= {0};
    switch (command) {
        case SHF_AP_CONFIGURE_GESTURE_ADD:
            s_gesture.handle = handle;
            s_gesture.sensor_type = sensor_gesture;
            s_gesture.command = SENSOR_UPDATE_GESTURE;
            s_gesture.value = cgesture;
            break;
        case SHF_AP_CONFIGURE_GESTURE_CANCEL:
            s_gesture.handle = handle;
            s_gesture.sensor_type = sensor_gesture;
            s_gesture.command = SENSOR_CANCEL_GESTURE;
            s_gesture.value = cgesture;
            break;
    }
    SCP_Sensor_Manager_control(&s_gesture);
}

extern uint8_t get_shf_state();
void onSensorChanged_shf(int sensor, struct data_unit_t values)
{
    logv("on_%d_c=%d\n",sensor);
    shf_scheduler_notify(sensor, values);
}


void onAccuracyChanged_shf(int sensor, int accuracy)
{
}

void shf_sensor_init()
{
    //SCP_Sensor_Manager_control(NULL, SENSOR_REGISTER_ACTIVATE_CALLBACK_CMD, shf_sensor_ap_state_callback);
    struct CM4TaskInformationStruct open_cm4= {0};
    open_cm4.onAccuracyChanged = onAccuracyChanged_shf;
    open_cm4.onSensorChanged = onSensorChanged_shf;
    handle = SCP_Sensor_Manager_open(&open_cm4);
}

/******************************************************************************
 * Unit Test Function
******************************************************************************/
#ifdef SHF_UNIT_TEST_ENABLE
void shf_sensor_unit_test()
{
}
#endif
