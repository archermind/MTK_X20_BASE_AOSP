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
//#include <dev/touch.h>

#include "shf_action.h"
#include "shf_condition.h"
#include "shf_data_pool.h"
#include "shf_debug.h"
#include "shf_sensor.h"

#ifdef SHF_UNIT_TEST_ENABLE
#include "shf_unit_test.h"
#endif

typedef uint8_t shf_condition_link_index_t;

typedef struct {
    shf_condition_index_t condition_index;
    shf_condition_link_index_t next;
} shf_condition_link_t;

//NOTE: each item has 2 data index at most,
//so we should have 2 * SHF_CONDITION_SIZE * SHF_CONDITION_ITEM_SIZE link.
//But, here we define the count is 255 for that:
//1. if we use 320(2 * 40 * 4), we need use 2 bytes to present the index
//2. in most cases, end user uses all 8 indices in each condition is impossible.
#define CONDITION_LINK_SIZE (255)//(SHF_CONDITION_SIZE * SHF_CONDITION_ITEM_SIZE)

#define ALLOCATE_CONDITION_SIZE (SHF_CONDITION_SIZE + 1)
#define ALLOCATE_CONDITION_LINK_SIZE (CONDITION_LINK_SIZE + 1)
#define ALLOCATE_POOL_SIZE (SHF_POOL_SIZE + 1)

//slot 0 is reserved for NULL check.
//data_pool_link-->condition_link-->condition_pool
shf_condition_t condition_pool[ALLOCATE_CONDITION_SIZE];
shf_condition_link_t condition_link[ALLOCATE_CONDITION_LINK_SIZE];
shf_condition_link_index_t data_link[ALLOCATE_POOL_SIZE];

uint8_t condition_added_count = 0;
bool_t condition_changed = FALSE;
//mutex_t mutex_pool;

void shf_condition_init()
{
    logv("c_init>>>\n");

    //link free head to reserved + 1, so reserved slots will not be used by add operation.
    //reserved slots only support updating operation
    shf_condition_index_t i;
    shf_condition_link_index_t j;
    condition_pool[0].data.next = SHF_CONDITION_RESERVED_COUNT;
    for (i = SHF_CONDITION_RESERVED_COUNT; i < ALLOCATE_CONDITION_SIZE - 1; i++) {
        condition_pool[i].data.next = i + 1;
    }
    for (j = 0; j < ALLOCATE_CONDITION_LINK_SIZE - 1; j++) {
        condition_link[j].next = j + 1;
    }

    logv("c_init<<<\n");
}

bool_t shf_condition_index_valid(shf_condition_index_t condition_index)
{
    if (!condition_index || condition_index > SHF_CONDITION_SIZE) {
        logw("c_valid_c: %d\n", condition_index);
        return FALSE;
    }
    return TRUE;
}

bool_t shf_action_index_valid(shf_action_index_t action_index)
{
    if (!action_index || action_index > SHF_CONDITION_ACTION_SIZE) {
        logw("c_valid_a: %d\n", action_index);
        return FALSE;
    }
    return TRUE;
}

static bool_t link_data_pool_one(shf_data_index_t data_index, shf_condition_index_t condition_index)
{
    logv("c_link: dindex=%d, cid=%d\n", data_index, condition_index);

    if (!data_index) {
        return FALSE;
    }
    bool_t linked = FALSE;
    shf_condition_link_t* free_head = condition_link;
    if (free_head->next) {
        shf_condition_link_index_t find_index = free_head->next;
        shf_condition_link_t* find = condition_link + find_index;

        if (find->condition_index) {
            logv("c_link: exists [%d %d %d]\n",
                 data_index, condition_index, find->condition_index);
        }

        //remove the node from free link
        free_head->next = find->next;
        //link the node to data link
        find->next = data_link[data_index];
        data_link[data_index] = find_index;
        find->condition_index = condition_index;
        linked = TRUE;
    }
    return linked;
}

static bool_t unlink_data_pool_one(shf_data_index_t data_index,
                                   shf_condition_index_t condition_index)
{
    logv("c_unlink: dindex=%d, cid=%d\n", data_index, condition_index);

    if (!data_index) {
        return FALSE;
    }
    shf_condition_link_index_t cur = data_link[data_index];
    shf_condition_link_index_t pre = 0;
    while(cur != 0 && (condition_link + cur)->condition_index != condition_index) {
        pre = cur;
        cur = (condition_link + cur)->next;
    }
    if (cur) {//find it
        if (!pre) {//fist
            data_link[data_index] = (condition_link + cur)->next;
        } else {
            (condition_link + pre)->next = (condition_link + cur)->next;
        }
        //link the node to free link
        (condition_link + cur)->next = condition_link->next;
        condition_link->next = cur;
        (condition_link + cur)->condition_index = 0;
    } else {
        logw("c_unlink: no [%d %d]\n", data_index, condition_index);
    }
    return TRUE;
}

bool_t shf_condition_unique_data_index(shf_condition_t* condition, shf_data_index_t* list,
                                       shf_data_index_t* size)
{
    shf_condition_item_index_t i;
    for (i = 0; i < SHF_CONDITION_ITEM_SIZE; i++) { //add it to cell list
        shf_condition_item_t* item = condition->data.condition.item + i;
#ifdef SHF_DEBUG_MODE
        logv("c_unique: index=%d\n", i);
        shf_debug_print_bytes(item, sizeof(shf_condition_item_t));
#endif
        if (item->op == SHF_OPERATION_MOD_OFFSET) {
            //reserved for the last item
            continue;
        }
        if (!item->dindex1) {
            break;
        }
        if (!contain(list, *size, item->dindex1)) {
            list[(*size)++] = item->dindex1;
        }
        if (item->dindex2 && !contain(list, *size, item->dindex2)) {
            list[(*size)++] = item->dindex2;
        }
    }
    logv("c_unique: count=%d\n", *size);
    return TRUE;
}

typedef bool_t (*process_data_pool_link_handler)(shf_data_index_t data_index,
        shf_condition_index_t condition_index);
static void process_data_pool_link(shf_condition_index_t condition_index,
                                   process_data_pool_link_handler handler)
{
    logv("c_p_dp: cid=%d\n", condition_index);

    shf_condition_t* condition = condition_pool + condition_index;
    //index1 and index2
    shf_data_index_t added_list[SHF_CONDITION_ITEM_SIZE * 2];
    memset(added_list, 0, sizeof(added_list));
    shf_data_index_t added_count = 0;
    shf_condition_unique_data_index(condition, added_list, &added_count);
    shf_data_index_t i;
    for (i = 0; i < added_count; i++) {
        //logv("process_data_pool_link: cid=%d is linked to dindexs[%d]=%d\n", condition_index, i, added_list[i]);
        handler(added_list[i], condition_index);
    }
}

shf_action_index_t shf_condition_action_add(shf_condition_index_t condition_index,
        shf_action_id_t action)
{
    logv("c_add_a: cid=%d, action=%d\n", condition_index, action);
    if (!shf_condition_index_valid(condition_index) || !shf_action_valid(action)) {
        return SHF_ACTION_INDEX_INVALID;
    }
    shf_action_id_t* actions = (condition_pool + condition_index)->data.condition.action;
    shf_action_index_t i;
    for (i = 0; i < SHF_CONDITION_ACTION_SIZE; i++) {
        shf_action_id_t* paction = actions + i;
        if (!(*paction)) {
            *paction = action;
            //action index base is 1, not 0.
            //so return value should plus 1
            return i + 1;
        }
    }
    logw("c_add_a: %d full\n", condition_index);
    return SHF_ACTION_INDEX_INVALID;
}

status_t shf_condition_remove(shf_condition_index_t condition_index);
status_t shf_condition_action_remove(shf_condition_index_t condition_index,
                                     shf_action_index_t action_index)
{
    logv("c_remove_a: cid=%d, aid=%d\n", condition_index, action_index);
    if (!shf_condition_index_valid(condition_index) || !shf_action_index_valid(action_index)) {
        return SHF_STATUS_ERROR;
    }
    shf_condition_t* find = condition_pool + condition_index;
    bool_t emptyAction = TRUE;
    shf_action_index_t i;
    for (i = 0; i < SHF_CONDITION_ACTION_SIZE; i++) {
        shf_action_id_t* paction = find->data.condition.action + i;
        if (i == action_index - 1) {//-1 is need
            if (*paction) {
                *paction = 0;
            } else {
                logw("c_remove_a: empty %d\n", condition_index);
                //has been removed, also return success
                return SHF_STATUS_OK;
            }
        } else if (*paction) {
            emptyAction = FALSE;
        }
    }
    if (emptyAction) {
        process_data_pool_link(condition_index, unlink_data_pool_one);
        // process_condition_gesture(condition_index, unregister_gesture);
        //if condition is reserved, do not remove it
        if (condition_index >= SHF_CONDITION_RESERVED_COUNT) {
            shf_condition_remove(condition_index);
        } else {
            shf_condition_t* remove = condition_pool + condition_index;
            memset(remove, 0, sizeof(shf_condition_t));
            condition_changed = TRUE;
        }
    }
    return SHF_STATUS_OK;
}

shf_condition_index_t shf_condition_add(const shf_condition_t* const condition)
{
    shf_condition_t* free_head = condition_pool;
    shf_condition_index_t find_index = 0;
    if (free_head->data.next) {
        find_index = free_head->data.next;
        shf_condition_t* find = free_head + find_index;
        free_head->data.next = find->data.next;
        memcpy(find, condition, sizeof(shf_condition_t));
        process_data_pool_link(find_index, link_data_pool_one);
        // process_condition_gesture(find_index, register_gesture);
        condition_changed = TRUE;
        condition_added_count++;
    } else {
        logw("c_add: no pos\n");
    }

    logv("c_add: cid=%d\n", find_index);
    return find_index;
}

status_t shf_condition_remove(shf_condition_index_t condition_index)
{
    logv("c_remove: cid=%d\n", condition_index);
    if (!shf_condition_index_valid(condition_index)) {
        logw("c_remove: %d\n", condition_index);
        return SHF_STATUS_ERROR;
    }
    //move the node to free link
    shf_condition_t* remove = condition_pool + condition_index;
    memset(remove, 0, sizeof(shf_condition_t));
    remove->data.next = condition_pool->data.next;
    condition_pool->data.next = condition_index;
    condition_changed = TRUE;
    //only reduce unreserved count
    if (condition_index >= SHF_CONDITION_RESERVED_COUNT) {
        condition_added_count--;
    }
    return SHF_STATUS_OK;
}

status_t shf_condition_update(shf_condition_index_t condition_index,
                              const shf_condition_t* const condition)
{
    logv("c_update: cid=%d\n", condition_index);
    if (!shf_condition_index_valid(condition_index) || condition_index >= SHF_CONDITION_RESERVED_COUNT) {
        return SHF_STATUS_ERROR;
    }
    shf_condition_t* find = condition_pool + condition_index;
    process_data_pool_link(condition_index, unlink_data_pool_one);
    //process_condition_gesture(condition_index, unregister_gesture);
    memset(find, 0, sizeof(shf_condition_t));
    if (condition) {
        memcpy(find, condition, sizeof(shf_condition_t));
        process_data_pool_link(condition_index, link_data_pool_one);
        //process_condition_gesture(condition_index, register_gesture);
    }
    condition_changed = TRUE;
    return SHF_STATUS_OK;
}

bool_t compare_value_uint64(shf_operation_type op, uint64_t cmp1, uint64_t cmp2, uint64_t offset)
{
    logv("c_c_v: op(%d), cmp1(%lld), cmp2(%lld), offset(%lld)\n", op, cmp1, cmp2, offset);

    switch (op) {
        case SHF_OPERATION_MORE_THAN:
            return cmp1 > cmp2;
        case SHF_OPERATION_MORE_THAN_OR_EQUAL:
            return cmp1 >= cmp2;
        case SHF_OPERATION_LESS_THAN:
            return cmp1 < cmp2;
        case SHF_OPERATION_LESS_THAN_OR_EQUAL:
            return cmp1 <= cmp2;
        case SHF_OPERATION_EQUAL:
            return cmp1 == cmp2;
        case SHF_OPERATION_NOT_EQUAL:
            return cmp1 != cmp2;
        case SHF_OPERATION_MOD:
            //return (cmp1 - offset) % cmp2 == 0;
            //only valid for time stamp
            return cmp1 - offset >= cmp2;
    }
    return FALSE;
}

bool_t compare_condition_item_uint32(shf_condition_item_t* item, shf_condition_item_t* nitem,
                                     bool_t old1, bool_t old2, bool_t cmp, bool_t dif, shf_operation_type op)
{
    uint32_t cmp1;
    uint32_t cmp2;
    if (old1) {
        cmp1 = *shf_data_pool_get_last_uint32(item->dindex1);
    } else {
        cmp1 = *shf_data_pool_get_uint32(item->dindex1);
    }
    if (!cmp && !dif) { //compare data1 op value
        cmp2 = item->value.duint32;
    } else {
        if (old2) {
            cmp2 = *shf_data_pool_get_last_uint32(item->dindex2);
        } else {
            cmp2 = *shf_data_pool_get_uint32(item->dindex2);
        }
        if (dif) { //compare data1 op data2 + value
            cmp2 += item->value.duint32;
        }
    }

    uint32_t offset = 0;
    if (op == SHF_OPERATION_MOD && nitem != NULL) {
        offset = nitem->value.duint32;
    }
    return compare_value_uint64(op, cmp1, cmp2, offset);
}

bool_t compare_condition_item_uint64(shf_condition_item_t* item, shf_condition_item_t* nitem,
                                     bool_t old1, bool_t old2, bool_t cmp, bool_t dif, shf_operation_type op)
{
    uint64_t cmp1;
    uint64_t cmp2;
    if (old1) {
        cmp1 = *shf_data_pool_get_last_uint64(item->dindex1);
    } else {
        cmp1 = *shf_data_pool_get_uint64(item->dindex1);
    }
    if (!cmp && !dif) { //compare data1 op value
        cmp2 = timestamp_get(&item->value.dtime);
    } else {
        if (old2) {
            cmp2 = *shf_data_pool_get_last_uint64(item->dindex2);
        } else {
            cmp2 = *shf_data_pool_get_uint64(item->dindex2);
        }
        if (dif) { //compare data1 op data2 + value
            cmp2 += timestamp_get(&item->value.dtime);
        }
    }

    uint64_t offset = 0;
    if (op == SHF_OPERATION_MOD && nitem != NULL) {
        offset = timestamp_get(&nitem->value.dtime);
    }

    bool_t pass = compare_value_uint64(op, cmp1, cmp2, offset);
    if (pass && op == SHF_OPERATION_MOD && nitem != NULL) {
        //nitem->value.dtime = cmp1;
        timestamp_set(&nitem->value.dtime, cmp1);
    }
    return pass;
}

static bool_t compare_condition_item(shf_condition_item_t* item, shf_condition_item_t* nitem)
{
    if (item->op == SHF_OPERATION_ANY) {
        return TRUE;
    }
    bool_t old1 = item->op & SHF_OPERATION_MASK_OLD1;
    bool_t old2 = item->op & SHF_OPERATION_MASK_OLD2;
    bool_t cmp = item->op & SHF_OPERATION_MASK_CMP;
    bool_t dif = item->op & SHF_OPERATION_MASK_DIF;
    shf_operation_type op = item->op & SHF_OPERATION_MASK_DATA;
    shf_data_type_t type = shf_data_pool_get_type(item->dindex1);

    logv("c_c_item: index1(%d, %d), index2(%d, %d), cmp(%d), dif(%d), op(%d), type(%d)\n",
         item->dindex1, old1, item->dindex2, old2, cmp, dif, op, type);

    switch (type) {
        case SHF_DATA_TYPE_UINT32:
            return compare_condition_item_uint32(item, nitem, old1, old2, cmp, dif, op);
        case SHF_DATA_TYPE_UINT64:
            return compare_condition_item_uint64(item, nitem, old1, old2, cmp, dif, op);
        default:
            return FALSE;
    }
}

typedef void (*condition_traverser)(shf_condition_t* condition, shf_condition_index_t condition_index);
void shf_condition_traverse(shf_data_index_t data_index, condition_traverser traverser)
{
    shf_condition_link_index_t link_index = data_link[data_index];
    shf_condition_link_t* link_node = condition_link + link_index;
    while (link_index && link_node->condition_index) {
        traverser((shf_condition_t*)(condition_pool + link_node->condition_index), link_node->condition_index);
        link_index = link_node->next;
        link_node = condition_link + link_index;
    }
}
/**
 * values[0] = 1
 * values[1] = 1
 * values[2] = 2
 * ....
 * position = 2, valid value is values[1], not values[2]
 */
const uint8_t TYPE_UNKNOWN = 0;
const uint8_t TYPE_COMBINE = 1;
const uint8_t TYPE_LEFT = 2;
const uint8_t TYPE_PASS = 3;
#define TYPE_STACK_SIZE (10)
uint8_t stack_types[TYPE_STACK_SIZE];
uint8_t stack_values[TYPE_STACK_SIZE];
uint8_t stack_position = 10;

static uint8_t pop()
{
    stack_position--;
    return stack_values[stack_position];
}

static uint8_t pop_type(uint8_t* value)
{
    if (stack_position == 0) {
        return TYPE_UNKNOWN;
    }
    stack_position--;
    *value = stack_values[stack_position];
    return stack_types[stack_position];
}

static uint8_t last()
{
    if (!stack_position) {
        return 0;
    }
    return stack_values[stack_position - 1];
}

static void push(uint8_t type, uint8_t value)
{
    stack_values[stack_position] = value;
    stack_types[stack_position] = type;
    stack_position++;
}

static bool_t op(bool_t last, shf_combine_t combine, bool_t current)
{
    switch(combine) {
        case SHF_COMBINE_AND:
            return last && current;
        case SHF_COMBINE_OR:
            return last || current;
        default:
            return last;
    }
}

static bool_t exist_left()
{
    uint8_t i;
    for(i = 0; i < stack_position; i++) {
        if (stack_types[i] == TYPE_LEFT) {
            return TRUE;
        }
    }
    return FALSE;
}
/**
 * supported cases(you can replace any "&&" by "||"):
 * a && b
 * a && b && c
 * a && b && c && d
 * (a && b) && c && d
 * (a && b && c && d)
 * a && (b && c) && d
 * a && (b && c && d)
 * a && b && (c && d)
 * (a && b) && (c && d)
 */
bool_t shf_condition_check(shf_condition_t* condition, shf_condition_index_t condition_index)
{
    bool_t pass = FALSE;
    bool_t lastpass = FALSE;
    shf_combine_t combine = SHF_COMBINE_INVALID;
    stack_position = 0;
    shf_combine_t mock = 0;
    shf_condition_item_index_t i;
    for (i = 0; i < SHF_CONDITION_ITEM_SIZE; i++) {
        shf_condition_item_t* item = condition->data.condition.item + i;
        if (item->op == SHF_OPERATION_MOD_OFFSET) {
            //reserved for the last item
            continue;
        }
        if (!item->op) { //last one, end loop
            break;
        }
        if (!mock) {//mock a right bracket to easy logic
            mock = item->combine;
        }
        bool_t left = mock & SHF_COMBINE_MASK_BRACKET_LEFT;
        bool_t right = mock & SHF_COMBINE_MASK_BRACKET_RIGHT;
        combine = mock & SHF_COMBINE_MASK_DATA;
        shf_condition_item_t* next_item = ((i == SHF_CONDITION_ITEM_SIZE + 1) ? NULL : item + 1);//TODO: size -1
        pass = compare_condition_item(item, next_item);
        logv("c_check: i[%d]=%d,m=%x\n", i, pass, mock);
        if (right) {//")&&",  ")&&("
            if (!i) {//must be non-first item
                logw("c_check:@1 %d\n", combine);
                return FALSE;
            }
            lastpass = pop();
            pop();//last left
            //Forward computing
            //Just need check 1 for none nested bracket and has computed all combine without left bracket
            shf_combine_t lastcombine = SHF_COMBINE_INVALID;
            if (pop_type(&lastcombine) == TYPE_COMBINE) {
                lastpass = op(pop(), lastcombine, lastpass);
            }
            if (lastpass && combine == SHF_COMBINE_OR) {//true || next
                //for none nested bracket, so it should be the final result
                return TRUE;
            }
            if (left) {//")&&("
                push(TYPE_PASS, lastpass);
                push(TYPE_COMBINE, combine);
                push(TYPE_LEFT, left);
                push(TYPE_PASS, pass);
            } else if (combine) {//")&&"
                push(TYPE_PASS, op(lastpass, combine, pass));
            } else {//error
                logw("c_check:@2 %d\n", combine);
            }
        } else if (left) {//"&&(", "("
            if(!combine) {//"(", fist one
                push(TYPE_LEFT, left);
                push(TYPE_PASS, pass);
            } else {//"&&("
                if (last() && combine == SHF_COMBINE_OR) {//true || next
                    //for none nested bracket, so it should be the final result
                    return TRUE;
                }
                push(TYPE_COMBINE, combine);
                push(TYPE_LEFT, left);
                push(TYPE_PASS, pass);
            }
        } else if (combine) {//&&, first combine is mask, so do not check it
            if (!i) {//do not check the first combine
                logw("c_check:@3 %d\n", combine);
                continue;
                //return FALSE;
            }
            if (last() && combine == SHF_COMBINE_OR) {//true || next
                //search next ")"
                shf_condition_item_index_t j;
                for (j = i + 1; j < SHF_CONDITION_ITEM_SIZE; j++) {
                    shf_condition_item_t* itemj = condition->data.condition.item + j;
                    right = itemj->combine & SHF_COMBINE_MASK_BRACKET_RIGHT;
                    if (right) {
                        i = j - 1;//move to current, skip item before j, remove right
                        mock = itemj->combine;// & ~SHF_COMBINE_MASK_BRACKET_RIGHT;
                        break;
                    }
                }
                if (mock) {//find ")" mock check it
                    continue;
                } else {//for none nested bracket, so it should be the final result
                    return TRUE;
                }
            }
            //lastpass = pop();
            push(TYPE_PASS, op(pop(), combine, pass));
        } else {//first one
            push(TYPE_PASS, pass);
        }
        mock = 0;
    }
    if (stack_position > 0) {
        pass = pop();
        if (exist_left()) {
            pop();//pop left
            if (pop_type(&combine) == TYPE_COMBINE) {
                //lastpass = pop();
                pass = op(pop(), combine, pass);
            }
        }
    }

    logv("c_check: cid=%d,ret=%d\n", condition_index, pass);
    return pass;
}

#ifdef SHF_UNIT_TEST_ENABLE
void unit_shf_action_run(shf_action_id_t action, shf_action_data_t* data);
#endif

void run_action(shf_condition_t* condition, shf_condition_index_t condition_index, bool_t check_new)
{
    shf_action_data_t data = { condition_index, condition->data.condition.action, condition };
    shf_action_index_t action_index;
    for (action_index = 0; action_index < SHF_CONDITION_ACTION_SIZE;
            action_index++) {
        shf_action_id_t action = condition->data.condition.action[action_index];
        if (action) {
            if (check_new && !(action & SHF_ACTION_MASK_NEW)) {
                continue;
            }
            if (!check_new && (action & SHF_ACTION_MASK_CHECK_LAST)
                    && (condition->data.condition.item->combine & SHF_COMBINE_MASK_LAST_RESULT)) {
                //new pass has checked
                continue;
            }
#ifdef SHF_UNIT_TEST_ENABLE
            unit_shf_action_run(action, &data);
#else
            shf_action_run(action, &data);
#endif
            if (!(action & SHF_ACTION_MASK_REPEAT)) {
                //Note: action_index is real index, but shf_condition_action_remove(index) is client index
                shf_condition_action_remove(condition_index, action_index + 1);
            } else { //remove new added flag as it is not repeating
                condition->data.condition.action[action_index] &= ~SHF_ACTION_MASK_NEW;
            }
        }
    }
}

void shf_condition_check_and_run(shf_condition_t* condition, shf_condition_index_t condition_index)
{
    if (!(condition->data.condition.item->combine & SHF_COMBINE_MASK_CHECKED)) { //only check it once
        condition->data.condition.item->combine |= SHF_COMBINE_MASK_CHECKED; //mark it
        if (condition_changed && (condition->data.condition.item->combine & SHF_COMBINE_MASK_LAST_RESULT)) {
            //for new added item: old pass, new pass
            //for old item: old pass, new fail,
            run_action(condition, condition_index, TRUE);
        }
        //only trigger action when last fail, current pass
        if (shf_condition_check(condition, condition_index)) { //check pass
            logv("c_check: cid=%d pass.\n", condition_index);
            run_action(condition, condition_index, FALSE);
            condition->data.condition.item->combine |= SHF_COMBINE_MASK_LAST_RESULT;
        } else {
            logv("c_check: cid=%d fail!\n", condition_index);
            condition->data.condition.item->combine &= ~SHF_COMBINE_MASK_LAST_RESULT; //change flag to fail
        }
    }
}

extern uint8_t get_shf_state();

void shf_condition_run()
{
    shf_condition_index_t cindex;
    shf_data_index_t dindex;
    logv("c_run>>>\n");
#ifdef SHF_DEBUG_MODE
    shf_data_pool_dump();
#endif
    if(!get_shf_state()) {
        logd("c_run:no condition!");
        return;
    }

    for (cindex = 1; cindex < ALLOCATE_CONDITION_SIZE; cindex++) {
        //for free condition, it will change nothing.
        (condition_pool + cindex)->data.condition.item->combine &= ~SHF_COMBINE_MASK_CHECKED;
        shf_action_clear_sent(condition_pool + cindex);
    }
    //check every pool data
    for (dindex = 1; dindex < ALLOCATE_POOL_SIZE; dindex++) {
        if (shf_data_pool_is_changed(dindex)) {
            logv("c_run: dindex=%d changed.\n", dindex);
            shf_condition_traverse(dindex, shf_condition_check_and_run);
        }
    }

    logv("c_run<<<\n");
}

void shf_condition_config()
{
    shf_data_index_t dindex;
    logv("c_config>>>changed=%d\n", condition_changed);

    if (!condition_changed) {
        return;
    }
    for (dindex = 1; dindex < ALLOCATE_POOL_SIZE; dindex++) {
        shf_sensor_collect_scp_state(dindex, data_link[dindex]);
    }

    condition_changed = FALSE;
    logv("c_config<<<\n");
}

unsigned char shf_get_condition_changed()
{
    return condition_changed;
}
#ifdef SHF_DEBUG_MODE
void shf_condition_dump_one(shf_condition_t* condition, shf_condition_index_t condition_index)
{
    logv("c[%d] --> %d a=", condition_index, condition->data.next);
    shf_action_index_t action_index;
    for (action_index = 0; action_index < SHF_CONDITION_ACTION_SIZE; action_index++) {
        logv("0x%x, ", condition->data.condition.action[action_index]);
    }
    logv(" counter=%d\n", (condition->data.condition.item->combine & SHF_COMBINE_MASK_CHECKED));
    shf_condition_item_index_t item_index;
    for (item_index = 0; item_index < SHF_CONDITION_ITEM_SIZE; item_index++) {
        shf_condition_item_t* item = condition->data.condition.item + item_index;
        switch(shf_data_pool_get_type(item->dindex1)) {
            case SHF_DATA_TYPE_UINT32:
                logv("item[%d]=%d, %d, %d, %d, %d\n",
                     item_index,
                     item->dindex1,
                     item->dindex2,
                     item->op,
                     item->combine,
                     item->value.duint32
                    );
                break;
            default:
                logv("item[%d]=%d, %d, %d, %d, %lld\n",
                     item_index,
                     item->dindex1,
                     item->dindex2,
                     item->op,
                     item->combine,
                     timestamp_get(&item->value.dtime)
                    );
                break;
        }
    }
}

void shf_condition_dump()
{
    logv("dump begin data_link info\n");
    shf_data_index_t i;
    for (i = 0; i < SHF_POOL_SIZE + 1; i++) {
        logv("dl[%d] --> %d\n", i, data_link[i]);
    }

    logv("dump begin condition_link info\n");
    shf_condition_link_index_t j;
    for (j = 0; j < SHF_POOL_SIZE + 1; j++) {
        logv("cl[%d]=%d --> [%d]\n", j, condition_link[j].condition_index, condition_link[j].next);
    }

    logv("dump begin condition_pool info\n");
    shf_condition_index_t k;
    for (k = 0; k < SHF_CONDITION_SIZE + 1; k++) {
        shf_condition_dump_one(condition_pool + k, k);
    }
    logv("dump begin free condition_pool info\n");
    shf_condition_index_t added_list[SHF_CONDITION_SIZE];
    memset(added_list, 0, sizeof(added_list));
    shf_condition_index_t added_count = 0;
    shf_condition_index_t index = 0;
    unique_add(added_list, SHF_CONDITION_SIZE, &added_count, index);
    while((condition_pool + index)->data.next) {
        unique_add(added_list, SHF_CONDITION_SIZE, &added_count, index);
        logv("free c[%d] --> %d\n", index, (condition_pool + index)->data.next);
        index = (condition_pool + index)->data.next;
    }
    unique_add(added_list, SHF_CONDITION_SIZE, &added_count, index);
    logv("free condition total count %d\n", added_count);
    shf_condition_index_t l;
    for (l = 0; l < SHF_CONDITION_SIZE + 1; l++) {
        if (!contain(added_list, added_count, l)) {
            shf_condition_dump_one(condition_pool + l, l);
        }
    }

    logv("dump begin traverse all data->condition info\n");
    const int size = ALLOCATE_POOL_SIZE;//shf_data_pool_get_size();
    shf_data_index_t data_index;
    for (data_index = 1; data_index < size; data_index++) {
        logv("data[%d]->begin\n", data_index);
        shf_condition_traverse(data_index, shf_condition_dump_one);
        logv("data[%d]->end\n", data_index);
    }
}
#endif

