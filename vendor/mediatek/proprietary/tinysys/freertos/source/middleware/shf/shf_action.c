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

#include <string.h>
#include "shf_action.h"
#include "shf_communicator.h"
#include "shf_condition.h"
#include "shf_configurator.h"
#include "shf_data_pool.h"
#include "shf_debug.h"

#ifdef SHF_UNIT_TEST_ENABLE
#include "shf_unit_test.h"
#endif

uint8_t action_buffer[
    SHF_CONDITION_ITEM_SIZE * 2//every condition item may have 2 index
    * (sizeof(shf_data_value_t))//max index value size
    + TRIGGER_SLOT_RESERVED//reserve some bytes for
];
uint8_t action_buffer_length;

static uint8_t wrap_data(uint8_t* buf_out, shf_data_index_t index)
{
    buf_out[0] = index;
    buf_out[1] = shf_data_pool_get_type(index);
    uint8_t len = 2;
    switch (buf_out[1]) {
        case SHF_DATA_TYPE_UINT32:
            logv("a_wrap_d: d[%d %d %d]\n", index,
                 *shf_data_pool_get_last_uint32(index), *shf_data_pool_get_uint32(index));
            save_uint32_2_uint8(*shf_data_pool_get_uint32(index), buf_out + 2);
            save_uint32_2_uint8(*shf_data_pool_get_last_uint32(index), buf_out + 6);
            len += 8;
            break;
        case SHF_DATA_TYPE_UINT64:
            logv("a_wrap_d: d2[%d %lld %lld]\n", index,
                 *shf_data_pool_get_last_uint64(index), *shf_data_pool_get_uint64(index));
            save_uint64_2_uint8(*shf_data_pool_get_uint64(index), buf_out + 2);
            save_uint64_2_uint8(*shf_data_pool_get_last_uint64(index), buf_out + 10);
            len += 16;
            break;
        default:
            logw("a_wrap_d: %d", buf_out[1]);
            break;
    }
    return len;
}

static void wrap_condition(shf_action_data_t* data)
{
    action_buffer[TRIGGER_SLOT_MSGID] = SHF_AP_TRIGGER;
    action_buffer[TRIGGER_SLOT_CHIP_NO] = 0;
    action_buffer[TRIGGER_SLOT_CID] = data->cid;
    action_buffer[TRIGGER_SLOT_AID1] = data->aid[0] & ~SHF_ACTION_MASK_NEW & ~SHF_ACTION_MASK_SENT;
    action_buffer[TRIGGER_SLOT_AID2] = data->aid[1] & ~SHF_ACTION_MASK_NEW & ~SHF_ACTION_MASK_SENT;
    action_buffer[TRIGGER_SLOT_AID3] = data->aid[2] & ~SHF_ACTION_MASK_NEW & ~SHF_ACTION_MASK_SENT;
    action_buffer[TRIGGER_SLOT_AID4] = data->aid[3] & ~SHF_ACTION_MASK_NEW & ~SHF_ACTION_MASK_SENT;
    action_buffer[TRIGGER_SLOT_RESULT] = SHF_STATUS_OK;
    action_buffer_length = TRIGGER_SLOT_RESERVED;
    //index1 and index2
    shf_data_index_t added_list[SHF_CONDITION_ITEM_SIZE * 2];
    memset(added_list, 0, sizeof(shf_data_index_t) * SHF_CONDITION_ITEM_SIZE * 2);
    shf_data_index_t added_count = 0;
    shf_condition_unique_data_index(data->condition, added_list, &added_count);
    action_buffer[TRIGGER_SLOT_SIZE] = added_count;
    shf_data_index_t i;
    for (i = 0; i < added_count; i++) {
        action_buffer_length += wrap_data(action_buffer + action_buffer_length,
                                          added_list[i]);
    }

    logv("a_wrap_c: count=%d,len=%d,cid(%d),aid(%d,%d,%d,%d)\n",
         added_count, action_buffer_length, action_buffer[TRIGGER_SLOT_CID], action_buffer[TRIGGER_SLOT_AID1],
         action_buffer[TRIGGER_SLOT_AID2], action_buffer[TRIGGER_SLOT_AID3], action_buffer[TRIGGER_SLOT_AID4]);
}

static status_t action_ap_wakeup(shf_action_data_t* data)
{
    logv("a_ap_wakeup: cid(%d) aid(%d,%d,%d,%d)\n",
         data->cid, data->aid[0], data->aid[1], data->aid[2], data->aid[3]);


    size_t i;
    for (i = 0; i < SHF_CONDITION_ACTION_SIZE; i++) {
        if((data->aid[i] & SHF_ACTION_MASK_DATA) == SHF_ACTION_ID_AP_WAKEUP
                && (data->aid[i] & SHF_ACTION_MASK_SENT)) {
            return SHF_STATUS_OK;
        }
    }
    wrap_condition(data);   //send action run data, use IPI to send trigger data
    status_t status = shf_communicator_send_message(SHF_DEVICE_AP, action_buffer,
                      action_buffer_length);
    if (SHF_STATUS_OK == status) {
        size_t i;
        for (i = 0; i < SHF_CONDITION_ACTION_SIZE; i++) {
            if((data->aid[i] & SHF_ACTION_MASK_DATA) == SHF_ACTION_ID_AP_WAKEUP) {
                data->aid[i] |= SHF_ACTION_MASK_SENT;
            }
        }
    }
    return status;
}

void shf_action_clear_sent(shf_condition_t* condition)
{
    size_t i;
    for (i = 0; i < SHF_CONDITION_ACTION_SIZE; i++) {
        if((condition->data.condition.action[i] & SHF_ACTION_MASK_DATA) == SHF_ACTION_ID_AP_WAKEUP) {
            condition->data.condition.action[i] &= ~SHF_ACTION_MASK_SENT;
        }
    }
}

action_handler action_handlers[SHF_ACTION_SIZE] = {
    NULL,
    action_ap_wakeup,
    // action_touch_active,
    // action_touch_deactive,
};
shf_action_id_t action_size = 5;

//should be called after initialization, so protection is not need here.
shf_action_id_t shf_action_add(action_handler handler)
{
    if (action_size > SHF_ACTION_SIZE) {
        logw("a_add: %d\n", action_size);
        return 0;
    }
    action_handlers[action_size] = handler;
    action_size++;
    logv("a_add: succeed. slot=%d\n", action_size - 1);
    return action_size - 1;
}

bool_t shf_action_valid(shf_action_id_t action)
{
    //allow unknown, mean remove
    if ((action & SHF_ACTION_MASK_DATA)>= action_size) {
        logw("a_check: %d\n", action);
        return FALSE;
    }
    return TRUE;
}

void shf_action_run(shf_action_id_t action, shf_action_data_t* data)
{
#ifdef SHF_DEBUG_MODE
    if (data) {
        logv("a_run: a=%d,cid(%d),aid(%d,%d,%d,%d)\n", action, data->cid,
             data->aid[0], data->aid[1], data->aid[2], data->aid[3]);
    } else {
        logv("a_run: a=%d no data!\n", action);
    }
#endif
    shf_action_id_t real = action & SHF_ACTION_MASK_DATA;
    logv("a_run: a=%d, aid=%d\n", action, real);
    if (!real || !shf_action_valid(action)) {
        logw("a_run: %d\n", action);
        return;
    }

    action_handlers[real](data);
}

/******************************************************************************
 * Unit Test Function
******************************************************************************/
#ifdef SHF_UNIT_TEST_ENABLE
uint8_t action_count = 11;
status_t unit_action_handler(shf_action_data_t* data)
{
    action_count++;
    return SHF_STATUS_OK;
}

void shf_action_unit_test()
{
    logv("\n\n**********\nshf_action_unit_test begin\n");
    uint8_t base_size = action_size;
    shf_action_id_t add_action = shf_action_add(unit_action_handler);
    unit_assert("new size", base_size + 1, action_size);

    uint8_t base_count = action_count;
    shf_action_run(add_action, NULL);//valid action id
    unit_assert("run new action", base_count + 1, action_count);

    shf_action_run(0xFF, NULL);//invalid action id
    unit_assert("run invalid action 0xFF", base_count + 1, action_count);

    shf_action_id_t invalid_action = action_size;
    unit_assert("run invalid action max size", FALSE, shf_action_valid(invalid_action));
    unit_assert("check action valid", TRUE, shf_action_valid(add_action));


    shf_condition_t condition;
    memset(&condition, 0, sizeof(shf_condition_t));

    uint64_t interval = 0xffffffffff;
    shf_condition_item_t* item = condition.item;
    item->dindex1 = SHF_DATA_INDEX_CLOCK_TIME;
    item->dindex2 = SHF_DATA_INDEX_ACTIVITY_TIME;
    item->op = SHF_OPERATION_MORE_THAN;
    timestamp_set(&item->value.dtime, 0);

    item = item + 1;
    item->combine = SHF_COMBINE_AND;
    item->dindex1 = SHF_DATA_INDEX_CLOCK_TIME;
    item->op = SHF_OPERATION_LESS_THAN;
    timestamp_set(&item->value.dtime, interval);

    item = item + 1;
    item->combine = SHF_COMBINE_AND;
    item->dindex1 = SHF_DATA_INDEX_ACTIVITY_BIKE;
    item->op = SHF_OPERATION_MORE_THAN;
    item->value.duint32 = 60;

    //don't need check current state
    condition.action[0] = SHF_ACTION_ID_AP_WAKEUP
                          | SHF_ACTION_MASK_REPEAT
                          | SHF_ACTION_MASK_CHECK_LAST;
    condition.action[1] = SHF_ACTION_ID_CONSYS_WAKEUP;

    shf_action_data_t action_data;
    memset(&action_data, 0, sizeof(shf_action_data_t));
    action_data.cid = 11;
    action_data.aid = condition.action;
    action_data.condition = &condition;

    uint64_t u64 = 0xffffffffff;
    shf_data_pool_clear(SHF_DATA_INDEX_CLOCK_TIME);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_CLOCK_TIME, u64);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_CLOCK_TIME, u64 + 1);
    shf_data_pool_clear(SHF_DATA_INDEX_ACTIVITY_TIME);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_ACTIVITY_TIME, u64 + 2);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_ACTIVITY_TIME, u64 + 3);
    uint32_t u32 = 96;
    shf_data_pool_clear(SHF_DATA_INDEX_ACTIVITY_BIKE);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_ACTIVITY_BIKE, u32);
    shf_data_pool_set_uint64(SHF_DATA_INDEX_ACTIVITY_BIKE, u32 + 1);

    shf_action_run(SHF_ACTION_ID_AP_WAKEUP, &action_data);
    unit_assert("send ap flag", TRUE, ((condition.action[0] & SHF_ACTION_MASK_SENT) > 0));
    unit_assert("send consys flag", FALSE, ((condition.action[1] & SHF_ACTION_MASK_SENT) > 0));

    //header 9
    //clock size: 1 + 1 + 8 + 8
    //activity time: 1 + 1 + 8 + 8
    //activity bike: 1 + 1 + 4 + 4
    unit_assert("slot msgid", SHF_AP_TRIGGER, action_buffer[TRIGGER_SLOT_MSGID]);
    unit_assert("slot cid", action_data.cid, action_buffer[TRIGGER_SLOT_CID]);
    unit_assert("slot aid 1", (action_data.aid[0] & ~SHF_ACTION_MASK_NEW & ~SHF_ACTION_MASK_SENT),
                action_buffer[TRIGGER_SLOT_AID1]);
    unit_assert("slot aid 2", (action_data.aid[1] & ~SHF_ACTION_MASK_NEW & ~SHF_ACTION_MASK_SENT),
                action_buffer[TRIGGER_SLOT_AID2]);
    unit_assert("slot aid 3", (action_data.aid[2] & ~SHF_ACTION_MASK_NEW & ~SHF_ACTION_MASK_SENT),
                action_buffer[TRIGGER_SLOT_AID3]);
    unit_assert("slot aid 4", (action_data.aid[3] & ~SHF_ACTION_MASK_NEW & ~SHF_ACTION_MASK_SENT),
                action_buffer[TRIGGER_SLOT_AID4]);
    //55 = 9 + 18 + 18 + 10
    unit_assert("slot size", 3, action_buffer[TRIGGER_SLOT_SIZE]);
    uint8_t* base = action_buffer + TRIGGER_SLOT_RESERVED;
    unit_assert("clock index", SHF_DATA_INDEX_CLOCK_TIME, *(base + 0));
    unit_assert("clock type", SHF_DATA_TYPE_UINT64, *(base + 1));
    unit_assert_u64("clock new value", u64 + 1, *((uint64_t*)(base + 2)));
    unit_assert_u64("clock old value", u64 + 0, *((uint64_t*)(base + 10)));

    base = base + 18;
    unit_assert("activity time index", SHF_DATA_INDEX_ACTIVITY_TIME, *(base + 0));
    unit_assert("activity time type", SHF_DATA_TYPE_UINT64, *(base + 1));
    unit_assert_u64("activity time new value", u64 + 3, *((uint64_t*)(base + 2)));
    unit_assert_u64("activity time old value", u64 + 2, *((uint64_t*)(base + 10)));

    base = base + 18;
    unit_assert("bike index", SHF_DATA_INDEX_ACTIVITY_BIKE, *(base + 0));
    unit_assert("bike type", SHF_DATA_TYPE_UINT32, *(base + 1));
    unit_assert("bike new value", u32 + 1, *((uint32_t*)(base + 2)));
    unit_assert("bike old value", u32 + 0, *((uint32_t*)(base + 6)));
}
#endif
