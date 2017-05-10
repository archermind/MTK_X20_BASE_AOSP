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
#include "shf_data_pool.h"
#include "shf_debug.h"
#include "shf_process.h"
#include "shf_sensor.h"

void process_save_pedometer(struct data_unit_t values)
{
    shf_data_pool_set_uint32(SHF_DATA_INDEX_PEDOMETER_COUNT, values.pedometer_t.accumulated_step_count);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_PEDOMETER_DISTANCE, values.pedometer_t.accumulated_step_length);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_PEDOMETER_FREQUENCY, values.pedometer_t.step_frequency);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_PEDOMETER_LENGTH, values.pedometer_t.step_length);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_PEDOMETER_TIME, values.time_stamp);

    logd("p_p:out(%u %u %u %u)\n", values.pedometer_t.accumulated_step_count,
         values.pedometer_t.accumulated_step_length, values.pedometer_t.step_frequency, values.pedometer_t.step_length);
}

static void process_run_most_probable_activity()
{
    if (shf_data_pool_is_changed(SHF_DATA_INDEX_ACTIVITY_TIME)) {
        shf_data_index_t candidates[] = {
            //SHF_DATA_INDEX_ACTIVITY_VEHICLE,
            SHF_DATA_INDEX_ACTIVITY_BIKE,
            SHF_DATA_INDEX_ACTIVITY_FOOT,
            SHF_DATA_INDEX_ACTIVITY_STILL,
            SHF_DATA_INDEX_ACTIVITY_UNKNOWN,
            SHF_DATA_INDEX_ACTIVITY_TILT,
            SHF_DATA_INDEX_ACTIVITY_WALKING,
            SHF_DATA_INDEX_ACTIVITY_RUNNING,
            SHF_DATA_INDEX_ACTIVITY_CLIMBING,
        };
        shf_data_index_t max_activity = SHF_DATA_INDEX_ACTIVITY_VEHICLE;
        uint32_t max_confidence = *shf_data_pool_get_uint32(SHF_DATA_INDEX_ACTIVITY_VEHICLE);
        uint32_t temp_confidence;
        shf_data_index_t temp_activity;
        shf_data_index_t index;
        for (index = 0; index < 8; index++) {
            temp_activity = candidates[index];
            temp_confidence = *shf_data_pool_get_uint32(temp_activity);
            if (temp_confidence > max_confidence) {
                max_confidence = temp_confidence;
                max_activity = temp_activity;
            }
        }
        if (*shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_ACTIVITY) != max_activity
                || *shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_CONFIDENCE) != max_confidence) {
            shf_data_pool_set_uint32(SHF_DATA_INDEX_MPACTIVITY_ACTIVITY, max_activity);
            shf_data_pool_set_uint32(SHF_DATA_INDEX_MPACTIVITY_CONFIDENCE, max_confidence);
            shf_data_pool_set_uint64(SHF_DATA_INDEX_MPACTIVITY_TIME,
                                     *shf_data_pool_get_uint64(SHF_DATA_INDEX_ACTIVITY_TIME));
        }
    }
}

#ifndef SHF_DISABLE_ACTIVITY
//static void process_run_activity(const acc_oneTimeParam_t* accData)
void process_save_activity(struct data_unit_t values)
{
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_VEHICLE, values.activity_data_t.probability[IN_VEHICLE]);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_BIKE, values.activity_data_t.probability[ON_BICYCLE]);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_FOOT, values.activity_data_t.probability[ON_FOOT]);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_STILL, values.activity_data_t.probability[STILL]);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_UNKNOWN, values.activity_data_t.probability[UNKNOWN]);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_TILT, values.activity_data_t.probability[TILTING]);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_ACTIVITY_TIME, values.time_stamp);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_WALKING, values.activity_data_t.probability[WALKING]);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_RUNNING, values.activity_data_t.probability[RUNNING]);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_CLIMBING, values.activity_data_t.probability[CLIMBING]);
    logd("p_a:out(%d %d %d %d %d %d %d %d %d)\n", values.activity_data_t.probability[IN_VEHICLE], values.activity_data_t.probability[ON_BICYCLE],
         values.activity_data_t.probability[ON_FOOT], values.activity_data_t.probability[STILL], values.activity_data_t.probability[UNKNOWN], values.activity_data_t.probability[TILTING],
         values.activity_data_t.probability[WALKING], values.activity_data_t.probability[RUNNING], values.activity_data_t.probability[CLIMBING]);

}
#endif

void process_save_inpocket(struct data_unit_t values)
{
    int32_t old = *shf_data_pool_get_uint32(SHF_DATA_INDEX_INPOCKET_VALUE);
    if (values.value[0] != old) {
        shf_data_pool_set_uint32(SHF_DATA_INDEX_INPOCKET_VALUE, values.value[0]);
        shf_data_pool_set_uint64(SHF_DATA_INDEX_INPOCKET_TIME, values.time_stamp);
    }
    logd("p_i:out(%d)\n", values.value[0]);
}

void process_save_pickup(struct data_unit_t values)
{

    int32_t old = *shf_data_pool_get_uint32(SHF_DATA_INDEX_PICKUP_VALUE);
    if (values.value[0] != old) {
        shf_data_pool_set_uint32(SHF_DATA_INDEX_PICKUP_VALUE, values.value[0]);
        shf_data_pool_set_uint64(SHF_DATA_INDEX_PICKUP_TIME, values.time_stamp);
    }
    logd("p_pu:out(%d)\n", values.value[0]);
}

void process_save_shake(struct data_unit_t values)
{

    int32_t old = *shf_data_pool_get_uint32(SHF_DATA_INDEX_SHAKE_VALUE);
    if (values.value[0] != old) {
        shf_data_pool_set_uint32(SHF_DATA_INDEX_SHAKE_VALUE, values.value[0]);
        shf_data_pool_set_uint64(SHF_DATA_INDEX_SHAKE_TIME, values.time_stamp);
    }
    logd("p_s:out(%d)\n", values.value[0]);
}

void process_save_flip(struct data_unit_t values)
{
    int32_t old = *shf_data_pool_get_uint32(SHF_DATA_INDEX_FACEDOWN_VALUE);
    if (values.value[0] != old) {
        shf_data_pool_set_uint32(SHF_DATA_INDEX_FACEDOWN_VALUE, values.value[0]);
        shf_data_pool_set_uint64(SHF_DATA_INDEX_FACEDOWN_TIME, values.time_stamp);
    }
    logd("p_f:out(%d)\n", values.value[0]);
}

void process_save_tap(struct data_unit_t values)
{
    int32_t old = *shf_data_pool_get_uint32(SHF_DATA_INDEX_TAP_VALUE);
    if (values.value[0] != old) {
        shf_data_pool_set_uint32(SHF_DATA_INDEX_TAP_VALUE, values.value[0]);
        shf_data_pool_set_uint64(SHF_DATA_INDEX_TAP_TIME, values.time_stamp);
    }
    logd("p_tap:out(%d)\n", values.value[0]);
}

void process_save_twist(struct data_unit_t values)
{
    int32_t old = *shf_data_pool_get_uint32(SHF_DATA_INDEX_TWIST_VALUE);
    if (values.value[0] != old) {
        shf_data_pool_set_uint32(SHF_DATA_INDEX_TWIST_VALUE, values.value[0]);
        shf_data_pool_set_uint64(SHF_DATA_INDEX_TWIST_TIME, values.time_stamp);
    }
    logd("p_tw:out(%d)\n", values.value[0]);
}

void process_save_snapshot(struct data_unit_t values)
{
    int32_t old = *shf_data_pool_get_uint32(SHF_DATA_INDEX_SNAPSHOT_VALUE);
    if (values.value[0] != old) {
        shf_data_pool_set_uint32(SHF_DATA_INDEX_SNAPSHOT_VALUE, values.value[0]);
        shf_data_pool_set_uint64(SHF_DATA_INDEX_SNAPSHOT_TIME, values.time_stamp);
    }
    logd("p_sn:out(%d)\n", values.value[0]);
}
void process_save_freefall(struct data_unit_t values)
{
    int32_t old = *shf_data_pool_get_uint32(SHF_DATA_INDEX_FREE_FALL_VALUE);
    if (values.value[0] != old) {
        shf_data_pool_set_uint32(SHF_DATA_INDEX_FREE_FALL_VALUE, values.value[0]);
        shf_data_pool_set_uint64(SHF_DATA_INDEX_FREE_FALL_TIME, values.time_stamp);
    }
    logd("p_ff:out(%d)\n", values.value[0]);
}

void process_save_pdr(struct data_unit_t values)
{
    shf_data_pool_set_uint32(SHF_DATA_INDEX_PDR_X, values.pdr_event.x);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_PDR_Y, values.pdr_event.y);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_PDR_Z, values.pdr_event.z);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_PDR_TIME, values.time_stamp);
    logd("p_pdr:out(%d %d %d)\n",values.pdr_event.x,values.pdr_event.y,values.pdr_event.z);

}
unsigned char shf_get_condition_changed();
extern void shf_sensor_run_algo(int type);

//look at which sensor condition was set.
void shf_process_run(int type)
{
    shf_sensor_run_algo(type);
}

void shf_process_save(int sensor, struct data_unit_t values)
{
    logd("shf_p_s,sensor=%d\n",sensor);
    switch(sensor) {
        case SENSOR_TYPE_PEDOMETER:
            process_save_pedometer(values);
            break;
        case SENSOR_TYPE_ACTIVITY:
            process_save_activity(values);
            //MPA calculate
            process_run_most_probable_activity();
            break;
        case SENSOR_TYPE_FLIP:
            process_save_flip(values);
            break;
        case SENSOR_TYPE_INPOCKET:
            process_save_inpocket(values);
            break;
        case SENSOR_TYPE_SHAKE:
            process_save_shake(values);
            break;
        case SENSOR_TYPE_PICK_UP:
            process_save_pickup(values);
            break;
        case SENSOR_TYPE_TAP:
            process_save_tap(values);
            break;
        case SENSOR_TYPE_TWIST:
            process_save_twist(values);
            break;
        case SENSOR_TYPE_SNAPSHOT:
            process_save_snapshot(values);
            break;
        case SENSOR_TYPE_FREEFALL:
            process_save_freefall(values);
            break;
        case SENSOR_TYPE_PDR:
            process_save_pdr(values);
            break;
        default:
            break;
    }

}

/******************************************************************************
 * Unit Test Function
******************************************************************************/
#ifdef SHF_UNIT_TEST_ENABLE
void shf_process_unit_test()
{
    logv("\n\n**********\nshf_process_unit_test begin\n");
    shf_data_index_t activities[] = {
        SHF_DATA_INDEX_ACTIVITY_VEHICLE,
        SHF_DATA_INDEX_ACTIVITY_BIKE,
        SHF_DATA_INDEX_ACTIVITY_FOOT,
        SHF_DATA_INDEX_ACTIVITY_STILL,
        SHF_DATA_INDEX_ACTIVITY_UNKNOWN,
        SHF_DATA_INDEX_ACTIVITY_TILT,
        SHF_DATA_INDEX_ACTIVITY_TIME,
    };
    for (shf_data_index_t index = 0; index < 7; index++) {
        shf_data_pool_clear(activities[index]);
        shf_data_pool_set_uint32(activities[index], index);
    }
    shf_data_pool_clear(SHF_DATA_INDEX_MPACTIVITY_ACTIVITY);
    shf_data_pool_clear(SHF_DATA_INDEX_MPACTIVITY_CONFIDENCE);
    shf_data_pool_clear(SHF_DATA_INDEX_MPACTIVITY_TIME);
    process_run_most_probable_activity();
    unit_assert("process most probable activity 1", SHF_DATA_INDEX_ACTIVITY_TILT,
                *shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_ACTIVITY));
    unit_assert("process most probable confidence 1", 5,
                *shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_CONFIDENCE));
    unit_assert("process most probable time 1", 6,
                *shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_TIME));

    for (shf_data_index_t index = 0; index < 7; index++) {
        shf_data_pool_clear(activities[index]);
        shf_data_pool_set_uint32(activities[index], 7 - index);
    }
    shf_data_pool_clear(SHF_DATA_INDEX_MPACTIVITY_ACTIVITY);
    shf_data_pool_clear(SHF_DATA_INDEX_MPACTIVITY_CONFIDENCE);
    shf_data_pool_clear(SHF_DATA_INDEX_MPACTIVITY_TIME);
    process_run_most_probable_activity();
    unit_assert("process most probable activity 2", SHF_DATA_INDEX_ACTIVITY_VEHICLE,
                *shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_ACTIVITY));
    unit_assert("process most probable confidence 2", 7,
                *shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_CONFIDENCE));
    unit_assert("process most probable time 2", 1,
                *shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_TIME));

    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_UNKNOWN, 99);
    shf_data_pool_clear(SHF_DATA_INDEX_MPACTIVITY_ACTIVITY);
    shf_data_pool_clear(SHF_DATA_INDEX_MPACTIVITY_CONFIDENCE);
    shf_data_pool_clear(SHF_DATA_INDEX_MPACTIVITY_TIME);
    process_run_most_probable_activity();
    unit_assert("process most probable activity 3", SHF_DATA_INDEX_ACTIVITY_UNKNOWN,
                *shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_ACTIVITY));
    unit_assert("process most probable confidence 3", 99,
                *shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_CONFIDENCE));
    unit_assert("process most probable time 3", 1,
                *shf_data_pool_get_uint32(SHF_DATA_INDEX_MPACTIVITY_TIME));

    //should check confidence sum <= 100
}
#endif
