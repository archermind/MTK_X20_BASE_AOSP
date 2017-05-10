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

#include "shf_data_pool.h"
#include "shf_debug.h"

#ifdef SHF_UNIT_TEST_ENABLE
#include "shf_unit_test.h"
#endif

#define ALLOCATE_POOL_SIZE (SHF_POOL_SIZE + 1)

//slot 0 is reserved for NULL check
uint32_t shf_pool_data_cur[ALLOCATE_POOL_SIZE];
uint32_t shf_pool_data_old[ALLOCATE_POOL_SIZE];

shf_gesture_t gesture_id;

//Here define types just to save code size.
//I expect this partition should be defined outside the data pool
//for that we should keep data pool simple and has nothing about sensor info.
//Maybe you have found that, data, condition and action can be reused easily if we remove this partition.
shf_data_type_t shf_pool_type[ALLOCATE_POOL_SIZE] = {
    SHF_DATA_TYPE_INVALID, //0

    SHF_DATA_INDEX_TYPE_ACCELEROMETER_X, //1
    SHF_DATA_INDEX_TYPE_ACCELEROMETER_Y, //2
    SHF_DATA_INDEX_TYPE_ACCELEROMETER_Z, //3
    SHF_DATA_INDEX_TYPE_ACCELEROMETER_TIME, //4
    SHF_DATA_TYPE_INVALID, //5

    SHF_DATA_INDEX_TYPE_LIGHT_VALUE, //6
    SHF_DATA_INDEX_TYPE_LIGHT_TIME, //7
    SHF_DATA_TYPE_INVALID, //8

    SHF_DATA_INDEX_TYPE_PROXIMITY_VALUE, //9
    SHF_DATA_INDEX_TYPE_PROXIMITY_TIME, //10
    SHF_DATA_TYPE_INVALID, //11

    SHF_DATA_INDEX_TYPE_CLOCK_TIME, //12
    SHF_DATA_TYPE_INVALID, //13

    SHF_DATA_INDEX_TYPE_PEDOMETER_LENGTH, //14
    SHF_DATA_INDEX_TYPE_PEDOMETER_FREQUENCY, //15
    SHF_DATA_INDEX_TYPE_PEDOMETER_COUNT, //16
    SHF_DATA_INDEX_TYPE_PEDOMETER_DISTANCE, //17
    SHF_DATA_INDEX_TYPE_PEDOMETER_TIME, //18
    SHF_DATA_TYPE_INVALID, //19

    SHF_DATA_INDEX_TYPE_ACTIVITY_VEHICLE, //20
    SHF_DATA_INDEX_TYPE_ACTIVITY_BIKE, //21
    SHF_DATA_INDEX_TYPE_ACTIVITY_FOOT, //22
    SHF_DATA_INDEX_TYPE_ACTIVITY_STILL, //23
    SHF_DATA_INDEX_TYPE_ACTIVITY_UNKNOWN, //24
    SHF_DATA_INDEX_TYPE_ACTIVITY_TILT, //25
    SHF_DATA_INDEX_TYPE_ACTIVITY_TIME, //26
    SHF_DATA_TYPE_INVALID, //27

    SHF_DATA_INDEX_TYPE_INPOCKET_VALUE, //28
    SHF_DATA_INDEX_TYPE_INPOCKET_TIME, //29
    SHF_DATA_TYPE_INVALID, //30

    SHF_DATA_INDEX_TYPE_MPACTIVITY_ACTIVITY, //31
    SHF_DATA_INDEX_TYPE_MPACTIVITY_CONFIDENCE, //32
    SHF_DATA_INDEX_TYPE_MPACTIVITY_TIME, //33
    SHF_DATA_TYPE_INVALID, //34

    SHF_DATA_INDEX_TYPE_SIGNIFICANT_VALUE, //35
    SHF_DATA_INDEX_TYPE_SIGNIFICANT_TIME, //36
    SHF_DATA_TYPE_INVALID, //37

    SHF_DATA_INDEX_TYPE_PICKUP_VALUE, //38
    SHF_DATA_INDEX_TYPE_PICKUP_TIME, //39
    SHF_DATA_TYPE_INVALID, //40

    SHF_DATA_INDEX_TYPE_FACEDOWN_VALUE, //41
    SHF_DATA_INDEX_TYPE_FACEDOWN_TIME, //42
    SHF_DATA_TYPE_INVALID, //43

    SHF_DATA_INDEX_TYPE_SHAKE_VALUE, //44
    SHF_DATA_INDEX_TYPE_SHAKE_TIME, //45
    SHF_DATA_TYPE_INVALID, //46

    SHF_DATA_INDEX_TYPE_GESTURE_VALUE, //47
    SHF_DATA_INDEX_TYPE_GESTURE_TIME, //48
    SHF_DATA_TYPE_INVALID, //49
    SHF_DATA_INDEX_TYPE_AUDIO_VALUE, //50
    SHF_DATA_INDEX_TYPE_AUDIO_TIME, //51
    SHF_DATA_TYPE_INVALID, //52

    SHF_DATA_INDEX_TYPE_NOISE_LEVEL_VALUE, //53
    SHF_DATA_INDEX_TYPE_NOISE_LEVEL_TIME, //54
    SHF_DATA_TYPE_INVALID, //55

    SHF_DATA_INDEX_TYPE_FREE_FALL_VALUE, //56
    SHF_DATA_INDEX_TYPE_FREE_FALL_TIME, //57
    SHF_DATA_TYPE_INVALID, //58

    SHF_DATA_INDEX_TYPE_ACTIVITY_WALKING, //59,activity
    SHF_DATA_INDEX_TYPE_ACTIVITY_RUNNING, //60
    SHF_DATA_INDEX_TYPE_ACTIVITY_CLIMBING, //61
    SHF_DATA_TYPE_INVALID, //62
    SHF_DATA_TYPE_INVALID, //63

    SHF_DATA_INDEX_TYPE_TAP_VALUE, //64
    SHF_DATA_INDEX_TYPE_TAP_TIME, //65
    SHF_DATA_TYPE_INVALID, //66

    SHF_DATA_INDEX_TYPE_TWIST_VALUE, //67
    SHF_DATA_INDEX_TYPE_TWIST_TIME, //68
    SHF_DATA_TYPE_INVALID, //69

    SHF_DATA_INDEX_TYPE_SNAPSHOT_VALUE, //70
    SHF_DATA_INDEX_TYPE_SNAPSHOT_TIME, //71
    SHF_DATA_TYPE_INVALID, //72

    SHF_DATA_INDEX_TYPE_PDR_X, //73
    SHF_DATA_INDEX_TYPE_PDR_Y, //74
    SHF_DATA_INDEX_TYPE_PDR_Z, //75
    SHF_DATA_INDEX_TYPE_PDR_TIME, //76
    SHF_DATA_TYPE_INVALID, //77
};
bool_t shf_pool_changed[ALLOCATE_POOL_SIZE];

uint32_t* shf_data_pool_get_uint32(shf_data_index_t index)
{
    return (shf_pool_data_cur + index);
}

uint64_t* shf_data_pool_get_uint64(shf_data_index_t index)
{
    return (uint64_t*) (shf_pool_data_cur + index);
}

uint32_t* shf_data_pool_get_last_uint32(shf_data_index_t index)
{
    return (shf_pool_data_old + index);
}

uint64_t* shf_data_pool_get_last_uint64(shf_data_index_t index)
{
    return (uint64_t*) (shf_pool_data_old + index);
}

void shf_data_pool_set_uint32(shf_data_index_t index, uint32_t value)
{
    uint32_t* p = shf_pool_data_cur + index;
    if (*p != value || index == SHF_DATA_INDEX_GESTURE_VALUE) {
        *(shf_pool_data_old + index) = *p; //update last
        *p = value; //update current
        *(shf_pool_changed + index) = TRUE;
    }
}

void shf_data_pool_set_uint64(shf_data_index_t index, uint64_t value)
{
    uint64_t* p = (uint64_t*) (shf_pool_data_cur + index);
    if (*p != value) {
        *(uint64_t*) (shf_pool_data_old + index) = *p; //update last
        *p = value; //update current
        *(shf_pool_changed + index) = TRUE;
    }
}

shf_data_type_t shf_data_pool_get_type(shf_data_index_t index)
{
    return *(shf_pool_type + index);
}

bool_t shf_data_pool_is_changed(shf_data_index_t index)
{
    return *(shf_pool_changed + index);
}

static void shf_data_pool_clear_changed()
{
    const int size = ALLOCATE_POOL_SIZE;//shf_data_pool_get_size();
    shf_data_index_t i;
    for (i = 0; i < size; i++) {
        *(shf_pool_changed + i) = FALSE;
    }
}

void shf_data_pool_run()
{
    //clear changed
    shf_data_pool_clear_changed();
}

void shf_data_pool_clear(shf_data_index_t index)
{
    switch (shf_pool_type[index]) {
        case SHF_DATA_TYPE_UINT64: {
            shf_pool_data_old[index] = 0;
            shf_pool_data_old[index + 1] = 0;
            shf_pool_data_cur[index] = 0;
            shf_pool_data_cur[index + 1] = 0;
        }
        break;
        default: {
            shf_pool_data_old[index] = 0;
            shf_pool_data_cur[index] = 0;
        }
        break;
    }
    shf_pool_changed[index] = FALSE;
}

void shf_data_pool_set_gesture(shf_gesture_t gid)
{
    gesture_id = gid;
}

shf_gesture_t shf_data_pool_get_gesture()
{
    return gesture_id;
}

#ifdef SHF_DEBUG_MODE
void shf_data_pool_dump()
{
    shf_data_index_t size = ALLOCATE_POOL_SIZE;
    shf_data_index_t i;
    for (i = 0; i < size; i++) {
        if (!shf_data_pool_is_changed(i)) {
            continue;
        }
        switch(shf_data_pool_get_type(i)) {
            case SHF_DATA_TYPE_UINT32:
                logv("dp[%d] %d, %d, %d, %d\n",
                     i,
                     *shf_data_pool_get_uint32(i),
                     *shf_data_pool_get_last_uint32(i),
                     shf_data_pool_is_changed(i),
                     shf_data_pool_get_type(i));
                break;
            case SHF_DATA_TYPE_UINT64:
                logv("dp[%d] %lld, %lld, %d, %d\n",
                     i,
                     *shf_data_pool_get_uint64(i),
                     *shf_data_pool_get_last_uint64(i),
                     shf_data_pool_is_changed(i),
                     shf_data_pool_get_type(i));
                break;
            default:
                logv("dp[%d] unknown\n", i);
                break;
        }
    }
}
#endif

/******************************************************************************
 * Unit Test Function
******************************************************************************/
#ifdef SHF_UNIT_TEST_ENABLE
void shf_data_pool_unit_test()
{
    logv("\n\n**********\nshf_data_pool_unit_test begin\n");
    uint32_t u32 = 97;
    shf_data_pool_clear(SHF_DATA_INDEX_SIGNIFICANT_VALUE);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE, u32);
    unit_assert("1", u32, *shf_data_pool_get_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE));
    unit_assert("2", 0, *shf_data_pool_get_last_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE));
    unit_assert("3", TRUE, shf_data_pool_is_changed(SHF_DATA_INDEX_SIGNIFICANT_VALUE));

    //shf_data_pool_clear(SHF_DATA_INDEX_SIGNIFICANT_VALUE);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE, u32 + 1);
    unit_assert("4", u32 + 1, *shf_data_pool_get_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE));
    unit_assert("5", u32, *shf_data_pool_get_last_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE));
    unit_assert("6", TRUE, shf_data_pool_is_changed(SHF_DATA_INDEX_SIGNIFICANT_VALUE));

    //shf_data_pool_clear(SHF_DATA_INDEX_SIGNIFICANT_VALUE);
    shf_data_pool_clear_changed();
    shf_data_pool_set_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE, u32 + 1);
    unit_assert("7", u32 + 1, *shf_data_pool_get_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE));
    unit_assert("8", u32, *shf_data_pool_get_last_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE));
    unit_assert("9", FALSE, shf_data_pool_is_changed(SHF_DATA_INDEX_SIGNIFICANT_VALUE));

    shf_data_pool_clear(SHF_DATA_INDEX_SIGNIFICANT_VALUE);
    unit_assert("10", 0, *shf_data_pool_get_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE));
    unit_assert("11", 0, *shf_data_pool_get_last_uint32(SHF_DATA_INDEX_SIGNIFICANT_VALUE));
    unit_assert("12", FALSE, shf_data_pool_is_changed(SHF_DATA_INDEX_SIGNIFICANT_VALUE));


    uint64_t u64 = 2;
    u64 = (u64<<34) + 3;
    shf_data_pool_clear(SHF_DATA_INDEX_SIGNIFICANT_TIME);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME, u64);
    unit_assert_u64("13", u64, *shf_data_pool_get_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME));
    unit_assert_u64("14", 0, *shf_data_pool_get_last_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME));
    unit_assert_u64("15", TRUE, shf_data_pool_is_changed(SHF_DATA_INDEX_SIGNIFICANT_TIME));

    //shf_data_pool_clear(SHF_DATA_INDEX_SIGNIFICANT_TIME);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME, u64 + 1);
    unit_assert_u64("16", u64 + 1, *shf_data_pool_get_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME));
    unit_assert_u64("17", u64, *shf_data_pool_get_last_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME));
    unit_assert_u64("18", TRUE, shf_data_pool_is_changed(SHF_DATA_INDEX_SIGNIFICANT_TIME));

    //shf_data_pool_clear(SHF_DATA_INDEX_SIGNIFICANT_TIME);
    shf_data_pool_clear_changed();
    shf_data_pool_set_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME, u64 + 1);
    unit_assert_u64("19", u64 + 1, *shf_data_pool_get_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME));
    unit_assert_u64("20", u64, *shf_data_pool_get_last_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME));
    unit_assert_u64("21", FALSE, shf_data_pool_is_changed(SHF_DATA_INDEX_SIGNIFICANT_TIME));

    shf_data_pool_clear(SHF_DATA_INDEX_SIGNIFICANT_TIME);
    unit_assert_u64("22", 0, *shf_data_pool_get_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME));
    unit_assert_u64("23", 0, *shf_data_pool_get_last_uint64(SHF_DATA_INDEX_SIGNIFICANT_TIME));
    unit_assert_u64("24", FALSE, shf_data_pool_is_changed(SHF_DATA_INDEX_SIGNIFICANT_TIME));
}
#endif

#ifdef SHF_INTEGRATION_TEST_ENABLE
int local_shf_data_pool_offset = 0;
void shf_data_pool_it()
{
    logv("\n\n**********\nshf_data_pool_it begin\n");
//    shf_data_pool_dump();
    int offset = local_shf_data_pool_offset;

    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_VEHICLE, 101 + offset);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_BIKE, 102 + offset);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_FOOT, 103 + offset);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_VEHICLE, 201 + offset);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_BIKE, 202 + offset);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_ACTIVITY_FOOT, 203 + offset);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_ACTIVITY_TIME, 555 + offset);

    shf_data_pool_set_uint32(SHF_DATA_INDEX_PEDOMETER_COUNT, 601 + offset);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_PEDOMETER_TIME, 666 + offset);

    shf_data_pool_set_uint32(SHF_DATA_INDEX_INPOCKET_VALUE, 61 - offset);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_INPOCKET_TIME, 777 + offset);

    shf_data_pool_set_uint32(SHF_DATA_INDEX_MPACTIVITY_ACTIVITY, 30 + offset);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_MPACTIVITY_CONFIDENCE, 40 + offset);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_PEDOMETER_LENGTH, 2000 + offset);
    shf_data_pool_set_uint32(SHF_DATA_INDEX_PEDOMETER_FREQUENCY, 3000 + offset);

    local_shf_data_pool_offset++;
//    shf_data_pool_dump();
    logv("\n\n**********\nshf_data_pool_it end\n");
}
#endif
