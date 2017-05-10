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
#include "shf_sensor.h"
#include "shf_configurator.h"
#include "shf_data_pool.h"
#include "shf_debug.h"

void to_scp_time(shf_condition_item_t* items, size_t size)
{
    size_t i;
    uint64_t ms;
    uint64_t time = timestamp_get_now();
    for (i = 0; i < size; i++) {
        if (0 == items[i].dindex1) {
            break;
        }
        if (SHF_DATA_INDEX_CLOCK_TIME == items[i].dindex1 && 0 == items[i].dindex2) {
            ms = timestamp_get(&items[i].value.dtime);
            time += ms;
            logv("f_to_scp: [%d]%lld->%lld\n", i, ms, time);
            timestamp_set(&items[i].value.dtime, time);
        }
    }
}

size_t parse_condition(uint8_t* data, size_t size, shf_condition_t* condition)
{
    logv("f_p_c>>>size=%d\n", size);

#ifdef SHF_DEBUG_MODE
    shf_debug_print_bytes(data, size);
#endif

    /**
      * data structure: item count(1 byte), items(item count * sizeof(shf_condition_item_t)),
      * action count(1 bytes), actions(count * sizeof(shf_action_id_t))
      * item structure: value(8 bytes), data index1(1 byte), data index2(1 byte), operator(1 byte), combine(1 byte)
      * Condition items are wrapped in method wrap_condition of mediatek/hardware/sensorhub/shf_hal.cpp.
      */
    size_t total_byte, action_byte;
    uint8_t item_count, action_count, i;
    int item_size = sizeof(shf_condition_item_t);
    item_count = *data++;
    if (item_count > SHF_CONDITION_ITEM_SIZE) {
        logw("f_p_c:@1 %d>%d\n", item_count, SHF_CONDITION_ITEM_SIZE);
        return 0;
    }

    logv("f_p_c: as=%d, cs=%d, a1=0x%x, a2=0x%x, a3=0x%x\n",
         sizeof(shf_action_id_t), item_size, &condition->data.condition.item[0].value,
         &condition->data.condition.item[0].dindex1, &condition->data.condition.item[1].value);
    logv("fpc:ic=%d,i_s=%d,size=%d\n",item_count,item_size,size);
    // 2 = item count(1 byte) + action count(1 byte)
    total_byte = item_count * (item_size + AP_MD32_CONDITION_ITEM_SIZE_DIFF) + 2;
    if (total_byte > size) {
        logw("f_p_c:@2 %d>%d\n", total_byte, size);
        return 0;
    }

    for (i = 0; i < item_count; i++) {
        memcpy(&condition->data.condition.item[i], data, item_size);
        data += (item_size + AP_MD32_CONDITION_ITEM_SIZE_DIFF);
    }
    to_scp_time(condition->data.condition.item, item_count);

    action_count = *data++;
    if (action_count > SHF_CONDITION_ACTION_SIZE) {
        logw("f_p_c:@3 %d>%d\n", action_count, SHF_CONDITION_ACTION_SIZE);
        return 0;
    }
    action_byte = sizeof(shf_action_id_t) * action_count;
    total_byte += action_byte;
    if (total_byte > size) {
        logw("f_p_c:@4 %d>%d\n", total_byte, size);
        return 0;
    }
    memcpy(&condition->data.condition.action, data, action_byte);

#ifdef SHF_DEBUG_MODE
    shf_condition_dump_one(condition, 100);
#endif

    logv("f_p_c<<<[%d,%d]\n", item_count, action_count);
    return total_byte;
}

void parse_gesture(uint8_t* data)
{
    uint8_t gesture_index, cgesture_index, command_id;
    command_id = *(data);
    gesture_index =*(data+2);
    cgesture_index = *(data+3);
    logd("<<<f_p_g,g=%d,cg=%d, cid=%d\n",gesture_index,cgesture_index, command_id);
    //map dindex to sensor manager type.
    set_configrable_gesture(gesture_index,cgesture_index, command_id);
}

void shf_handle_message(void* data, size_t size)
{
    logv("f_msg>>>size=%d\n", size);

#ifdef SHF_DEBUG_MODE
    shf_debug_print_bytes(data, size);
#endif

    uint8_t* buf_in = (uint8_t*) data; //change to uint8 pointer for read
    uint8_t opid = *buf_in;
    shf_condition_t condition;
    memset(&condition, 0, sizeof(shf_condition_t));
    uint8_t result = 0;
    uint8_t buf_out[SHF_IPI_PROTOCOL_BYTES * 2] = {0};
    buf_out[0] = opid;
    buf_out[1] = buf_in[1];//return it
    buf_out[2] = result;
    uint8_t buf_size = 3;
    switch (opid) {
        case SHF_AP_CONDITION_ADD: {
            //in: opId, sessionId, item array size, [item], action array size, [action]
            //out: opid, sessionId, CID
            if (parse_condition(buf_in + 2, size - 2, &condition)) {
                buf_out[2] = shf_condition_add(&condition);
            }
        }
        break;
        case SHF_AP_CONDITION_UPDATE: {
            //in: opId, sessionId, CID, item array size, [item], action array size, [action]
            //out: opId, sessionId, status_t
            shf_condition_index_t cid = *(buf_in + 2);
            if (parse_condition(buf_in + 3, size - 3, &condition)) {
                buf_out[2] = shf_condition_update(cid, &condition);
            }
        }
        break;
        case SHF_AP_CONDITION_ACTION_ADD: {
            //in: opId, sessionId, CID, action array size, [action]
            //out: opId, sessionId, AID array size, [AID]
            shf_condition_index_t cid = *(buf_in + 2);
            shf_action_index_t action_size = *(buf_in + 3);
            if (action_size + 4 == size) {//4: message type, CHIP_NO, CID, size
                buf_out[2] = action_size;
                uint8_t i;
                for (i = 0; i < action_size; i++) {
                    buf_out[i + 3] = shf_condition_action_add(cid, *((shf_action_id_t*) (buf_in + 4) + i));
                }
                buf_size = action_size + 3;
            }
        }
        break;
        case SHF_AP_CONDITION_ACTION_REMOVE: {
            //in: opId, sessionId, CID, AID array size, [AID]
            //out: opId, sessionId, status_t array size, [status_t]
            shf_condition_index_t cid = *(buf_in + 2);
            shf_action_index_t action_size = *(buf_in + 3);
            if (action_size + 4 == size) {//4: message type, CHIP_NO, CID, size
                buf_out[2] = action_size;
                shf_action_index_t i;
                for (i = 0; i < action_size; i++) {
                    buf_out[i + 3] = shf_condition_action_remove(cid, *((shf_action_id_t*) (buf_in + 4) + i));
                }
                buf_size = action_size + 3;
            }
        }
        break;
        case SHF_AP_CONFIGURE_GESTURE_ADD:
        case SHF_AP_CONFIGURE_GESTURE_CANCEL:
            //in: opId, sessionId, gesture id, configurable gesture id
            //out: opid, sessionId, status???
            parse_gesture(buf_in);
            return;
        default:
            logw("f_msg<<<%d\n", opid);
            break;
    }

    shf_communicator_send_message(SHF_DEVICE_AP, buf_out, buf_size);
}

void shf_configurator_init()
{
    logv("f_init>>>\n");
    shf_communicator_receive_message(SHF_DEVICE_AP, shf_handle_message);
    logv("f_init<<<\n");
}
