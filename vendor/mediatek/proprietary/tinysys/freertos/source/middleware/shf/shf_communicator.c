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

#include "shf_communicator.h"
#include "shf_configurator.h"
#include "shf_debug.h"
#include "shf_scheduler.h"

#ifdef SHF_UNIT_TEST_ENABLE
#include "mock_device.h"
#include "shf_condition.h"
#include "shf_unit_test.h"
//for test IPI
//#include <FreeRTOS.h>
//#include <task.h>
#else
#include <scp_ipi.h>
#include <utils.h>
#endif


#define BTIF_RELAY_UART_PORT uart_port1

//#ifdef SWITCH_TOUCH_PANEL
// touch panel enable reference count
//int8_t touch_enable_count = 0;
//#endif

static uint16_t checksum_get(void* data, size_t size)
{
    uint16_t chksum = 0;
    uint8_t* p = (uint8_t*) data;
    size_t i;
    for (i = 0; i < size; i++, p++) {
        chksum += *p;
    }
//#ifdef SHF_DEBUG_MODE
//    shf_debug_print_bytes(data, size);
//#endif
//    logv("m_checksum: %d\n", chksum);

    return chksum;
}

static bool_t checksum_check(void* data, size_t size, uint16_t chksum)
{
    uint16_t newsum = checksum_get(data, size);
    logv("m_checksum: in=%d, out=%d\n", chksum, newsum);
    return chksum == newsum;
}


#define BUFFER_STATE_IDLE           (0X00)
#define BUFFER_STATE_STARTED        (0X01)
#define BUFFER_STATE_COMPLETED      (0X02)

typedef struct {
    uint8_t* buffer; //buffer address
    uint8_t offset; //current filled offset
    uint8_t claim; //claim size in data
    uint8_t size; //allocated buffer size
    uint8_t state; //buffer data filled state
} protocol_buffer_t;

#define ESC (0xAA)
#define BOM (0xF0)
#define EOM (0x0F)
#define HEADER_SIZE (0x08)

//TODO define here to reduce stack size.
//But this will add data size and assume this function is called in the same thread.
uint8_t buffer[SHF_PROTOCOL_SEND_BUFFER_BYTES];

typedef status_t (*protocol_send_handler)(void* data, size_t size);
static status_t protocol_send_message(void* data, size_t size, const size_t partition,
                                      protocol_send_handler handler)
{
#ifdef SHF_DEBUG_MODE
    logv("m_send:\n");
    shf_debug_print_bytes(data, size);
#endif
    const size_t total = size + HEADER_SIZE;

    buffer[0] = ESC;
    buffer[1] = BOM;
    buffer[2] = total - 6;
    buffer[3] = 0x00;
    memcpy(buffer + 4, data, size);
    //check Size_L + Size_H + data
    save_uint16_2_uint8(checksum_get(buffer + 2, total - 6), buffer + total - 4);
    buffer[total - 2] = ESC;
    buffer[total - 1] = EOM;
    size_t count = (total%partition == 0) ? total/partition : total/partition + 1;
    size_t index = 0;
    while (index < count) {
        size_t length = (total - partition * index) >= partition ? partition : (total % partition);
        status_t result = handler(buffer + index * partition, length);
        if (result != SHF_STATUS_OK) {
            logw("m_send: fail! %d %d\n", index, size);
            return result;
        }
        index++;
    }
    return SHF_STATUS_OK;
}

static status_t protocol_receive_message(protocol_buffer_t* buf, uint8_t* rec_buf, uint8_t rec_size)
{
    logv("m_rcv>>>size=%d, esc=0x%x, bom=0x%x, size=%d,buf_size=%d\n", rec_size, *rec_buf, *(rec_buf + 1), *(rec_buf + 2), buf->size);

    if (rec_size >= 3 && *rec_buf == ESC && *(rec_buf + 1) == BOM
            && *(rec_buf + 2) > 5 // 2 for size, 1 for msgid, 1 for chip no, 1 for less info
            && *(rec_buf + 2) <= buf->size) { //head
        if ((buf->offset != (buf->claim + 6)) && (buf->state != BUFFER_STATE_IDLE)) {
            logw("m_rcv: nc! %d %d\n", buf->offset, buf->claim); //not complete
        }

        if (buf->state == BUFFER_STATE_COMPLETED) {
            //last cached data hasn't been processed,
            //so lose current message.
            logw("m_rcv: full!");
            return SHF_STATUS_ERROR;
        }
        buf->state = BUFFER_STATE_STARTED;
        buf->claim = *(rec_buf + 2);//size
        buf->offset = rec_size;//total
        memcpy(buf->buffer, rec_buf, rec_size); //can be avoided if buf_length == buf_offset
    } else if (buf->offset && buf->offset + rec_size <= buf->claim + 6) { //middle or tail, claim + 6 = all size
        if (buf->state != BUFFER_STATE_STARTED) {
            logw("m_rcv: middle! %d %d\n", buf->state, buf->offset);
            return SHF_STATUS_ERROR;
        }
        //append received message to last buffer
        memcpy(buf->buffer + buf->offset, rec_buf, rec_size);
        buf->offset += rec_size;
    }
#ifdef SHF_DEBUG_MODE
    //logv("protocol_receive_message:\n");
    shf_debug_print_bytes(buf->buffer, buf->offset);
#endif
    if (buf->state == BUFFER_STATE_STARTED && buf->offset && (buf->offset == buf->claim + 6)) { //enough
        if (*(buf->buffer + buf->offset - 2) == ESC && *(buf->buffer + buf->offset - 1) == EOM) { //tail
            //checksum only check size + user data
            uint16_t chksum = convert_uint8_2_uint16(buf->buffer + 2 + buf->claim);
            if (checksum_check(buf->buffer + 2, buf->claim, chksum)) {
                //TODO If using buffer mode, we may be need more buffer space
                //else, directly process this buffer is another way.
                //But, this need a mutex for condition link
                logv("m_rcv: state=2\n");
                buf->state = BUFFER_STATE_COMPLETED;
            } else {
#ifdef SHF_DEBUG_MODE
                shf_debug_print_bytes(buf->buffer, buf->offset);
#endif
                logw("m_rcv: chksum=%d!\n", chksum);
                return SHF_STATUS_ERROR;
            }

        } else {
            logw("m_rcv: no tail!\n");
            return SHF_STATUS_ERROR;
        }
    }
    return SHF_STATUS_OK;
}

//IPI only transfers 48 bytes one time,
//so buffer it until all data are sent completely.
//Assume:
//1. max buffer is 48 * 2 = 96 bytes
//2. all data are sent in sequence
//3. header is at buffer address 0
uint8_t shf_in_buf[SHF_AP_BUFFER_BYTES] = {0}; //for merging in data
protocol_buffer_t shf_protocol_buffer = { shf_in_buf, 0, 0, SHF_AP_BUFFER_BYTES };
static status_t shf_protocol_send_handler(void* data, size_t size)
{
    enum ipi_status status = DONE;
    enum ipi_status pre_status = DONE;
    //for test IPI, should delete after.
    //vTaskDelay(5000/portTICK_PERIOD_MS);
    do {
        status = scp_ipi_send(IPI_SHF, data, size, 0, IPI_SCP2AP);
        if (status != pre_status || pre_status == DONE) {
            logd("m_send_h:[%lld]%d %d\n", timestamp_get_ns(), size, status);
        }
        pre_status = status;
    } while (DONE != status);

    return SHF_STATUS_OK;
}

static void process_protocol_buffer(protocol_buffer_t* buf, communicator_handler_t handler)
{
    if (buf->state == BUFFER_STATE_COMPLETED) {
        handler(buf->buffer + 4, buf->claim - 2);
        memset(buf->buffer, 'A', buf->size);
        buf->state = BUFFER_STATE_IDLE;
    }
}

communicator_handler_t shf_handler;

//static
void shf_protocol_receive_handler(int id, void * data, uint size)
{
    logv("m_rcv>>>id=%d, size=%d\n", id, size);
#ifdef SHF_DEBUG_MODE
    shf_debug_print_bytes(data, size);
#endif

    if (id == IPI_SHF) {
        uint8_t rec_msg_type = *((uint8_t*) data + 4);//*(((&shf_protocol_buffer)->buffer)+ 5);
        uint8_t ret_msg_type = MSG_TYPE_VALID;
        uint8_t status = protocol_receive_message(&shf_protocol_buffer, (uint8_t*) data, size);
        if (SHF_STATUS_OK == status) {
            switch(rec_msg_type) {
                case SHF_AP_CONDITION_ADD:
                case SHF_AP_CONDITION_UPDATE:
                case SHF_AP_CONDITION_ACTION_ADD:
                case SHF_AP_CONDITION_ACTION_REMOVE:
                    ret_msg_type = MSG_TYPE_CONDITION;
                    break;
                case SHF_AP_CONFIGURE_GESTURE_ADD:
                case SHF_AP_CONFIGURE_GESTURE_CANCEL:
                    ret_msg_type = MSG_TYPE_GESTURE;
                    break;
                default://other
                    ret_msg_type = MSG_TYPE_VALID;
                    break;
            }
        }
        struct data_unit_t c_data = {0};
        logv("m_rcv<<<result=%d,ret_msg_type=%d\n", status,ret_msg_type);
        shf_scheduler_notify(ret_msg_type, c_data);
    }
}

#ifdef SHF_UNIT_TEST_ENABLE
status_t unit_shf_protocol_send_handler(void* data, size_t size);
#endif

status_t shf_communicator_send_message(shf_device_t device, void* data, size_t size)
{
    logv("m_send: device=%d, size=%d\n", device, size);

    switch (device) {
        case SHF_DEVICE_AP:
#ifdef SHF_UNIT_TEST_ENABLE
            return protocol_send_message(data, size, SHF_IPI_PROTOCOL_BYTES, unit_shf_protocol_send_handler);
#else
            return protocol_send_message(data, size, SHF_IPI_PROTOCOL_BYTES, shf_protocol_send_handler);
#endif
        default:
            logw("m_send: %d\n", device);
            return SHF_STATUS_ERROR;
    }
}

status_t shf_communicator_receive_message(shf_device_t device, communicator_handler_t handler)
{
    logv("m_rcv: device=%d\n", device);

    switch (device) {
        case SHF_DEVICE_AP:
            shf_handler = handler;
            scp_ipi_registration(IPI_SHF, shf_protocol_receive_handler, "shf_handler");
            break;
        default:
            logw("m_rcv: %d\n", device);
            break;
    }
    return SHF_STATUS_OK;
}

status_t shf_communicator_wakeup(shf_device_t device)
{
    switch (device) {
        case SHF_DEVICE_AP:
            while (DONE != scp_ipi_send(IPI_SHF, NULL, 0, 0, IPI_SCP2AP));
            break;
        default:
            logw("m_wakeup: %d\n", device);
            return SHF_STATUS_ERROR;
    }
    return SHF_STATUS_OK;
}

void shf_communicator_run()
{
    logv("cm_run>>>s=%d\n",shf_protocol_buffer.state);

    //Here use state to avoid mutex
    //TODO
    //If state mechanism is bad for losing lot of messages, please use mutex instead.
    //But, mutex should be tested fully for that condition traverse may block configuration.
    if (BUFFER_STATE_COMPLETED == shf_protocol_buffer.state) {
        process_protocol_buffer(&shf_protocol_buffer, shf_handler);
    }
    logv("cm_run<<<\n");
}

#ifdef SHF_UNIT_TEST_ENABLE
uint8_t unit_send_cache[SHF_AP_BUFFER_BYTES];
size_t unit_send_offset;
size_t unit_send_count;
void unit_clear_cache()
{
    memset(unit_send_cache, 0, SHF_AP_BUFFER_BYTES);
    unit_send_offset = 0;
    unit_send_count = 0;
}
status_t unit_shf_protocol_send_handler(void* data, size_t size)
{
    memcpy(unit_send_cache + unit_send_offset, data, size);
    unit_send_offset += size;
    unit_send_count++;
    return SHF_STATUS_OK;
}
void unit_clear_buffer()
{
    memset(shf_in_buf, 0, SHF_AP_BUFFER_BYTES);
    memset(consys_in_buf, 0, SHF_CONSYS_BUFFER_BYTES);
    shf_protocol_buffer.claim = 0;
    shf_protocol_buffer.offset = 0;
    shf_protocol_buffer.state = BUFFER_STATE_IDLE;
    consys_protocol_buffer.claim = 0;
    consys_protocol_buffer.offset = 0;
    consys_protocol_buffer.state = BUFFER_STATE_IDLE;
}
void test_send_and_receive()
{
    uint8_t data[SHF_AP_BUFFER_BYTES];
    size_t size = SHF_AP_BUFFER_BYTES - 20;
    for (int i = 0; i < size; i++) {
        data[i] = i;
    }
    unit_clear_cache();
    protocol_send_message(data, size, SHF_IPI_PROTOCOL_BYTES, unit_shf_protocol_send_handler);
    unit_assert("send all size", size + 8, unit_send_offset);
    unit_assert("send all count", 2, unit_send_count);
    unit_assert("send size check", size + 2, convert_uint8_2_uint16(unit_send_cache + 2));
    unit_assert("send data check 1", 10, *(unit_send_cache + 4 + 10));
    unit_assert("send data check 2", 50, *(unit_send_cache + 4 + 50));

    unit_clear_buffer();
    protocol_receive_message(&shf_protocol_buffer, unit_send_cache, SHF_IPI_PROTOCOL_BYTES);
    protocol_receive_message(&shf_protocol_buffer, unit_send_cache + SHF_IPI_PROTOCOL_BYTES, size + 8 - SHF_IPI_PROTOCOL_BYTES);
    unit_assert("receive state completed", BUFFER_STATE_COMPLETED, shf_protocol_buffer.state);
    unit_assert("receive size", size + 2, shf_protocol_buffer.claim);

    size = 20;
    unit_clear_cache();
    protocol_send_message(data, size, SHF_IPI_PROTOCOL_BYTES, unit_shf_protocol_send_handler);
    unit_assert("send all size", size + 8, unit_send_offset);
    unit_assert("send all count", 1, unit_send_count);
    unit_assert("send size check", size + 2, convert_uint8_2_uint16(unit_send_cache + 2));
    unit_assert("send data check 1", 10, *(unit_send_cache + 4 + 10));
    unit_assert("send data check 2", 0, *(unit_send_cache + 4 + 50));

    unit_clear_buffer();
    protocol_receive_message(&shf_protocol_buffer, unit_send_cache, size + 8);
    unit_assert("receive state completed", BUFFER_STATE_COMPLETED, shf_protocol_buffer.state);
    unit_assert("receive size", size + 2, shf_protocol_buffer.claim);
}
uint8_t unit_ipi_data[][48] = {
//add condition
    {
        0xAA, 0xF0, 0x13, 0x00,//header 0
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0xA9, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x1F, 0x00,//header 1
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x02,//item size
        SHF_DATA_INDEX_ACTIVITY_FOOT, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        SHF_DATA_INDEX_PEDOMETER_COUNT, SHF_DATA_INDEX_PEDOMETER_DISTANCE, SHF_OPERATION_MASK_CMP | SHF_OPERATION_EQUAL, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0xFD, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x1F, 0x00,//header [wrong] 2
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0x07, 0x05, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x13, 0x00,//header 3
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x02,//item size [wrong]
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0xAA, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x13, 0x00,//header 4
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x02,//action size [wrong]
        0x01,//action
        0xAA, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x12, 0x00,//header 5
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value [wrong]
        0x01,//action size
        0x01,//action
        0xA8, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x17, 0x00,//header 6
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x05,//action size
        0x02, 0x03, 0x05, 0x04, 0x03,//action[wrong]
        0xAA, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x38, 0x00,//header 7
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x04,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value 8
        0x02,//action size
        0x02, 0x03,//action
        0xCC, 0xCD, 0xAA, 0x0F
    },
//add action
    {
        0xAA, 0xF0, 0x08, 0x00,//header 9
        SHF_AP_CONDITION_ACTION_ADD,//message type
        0xFF,
        0x06,//CID
        0x02,//size
        0x01, 0x08,//array[wrong]
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x08, 0x00,//header 10
        SHF_AP_CONDITION_ACTION_ADD,//message type
        0xFF,
        0x06,//CID
        0x02,//size
        0x04, 0x03,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 11
        SHF_AP_CONDITION_ACTION_ADD,//message type
        0xFF,
        0x06,//CID
        0x01,//size
        0x02,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
//remove action
    {
        0xAA, 0xF0, 0x08, 0x00,//header 12
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x06,//CID
        0x02,//size
        0x01, 0x03,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 13
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x06,//CID
        0x01,//size
        0x00,//array[wrong]
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 14
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x06,//CID
        0x01,//size
        0x05,//array [wrong]
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 15
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x06,//CID
        0x01,//size
        0x02,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 16
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x06,//CID
        0x01,//size
        0x02,//array [wrong, but true for non-exist]
        0xCC, 0xCD, 0xAA, 0x0F
    },
//update condition
    {
        0xAA, 0xF0, 0x14, 0x00,//header 17
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        0x01,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_VEHICLE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x14, 0x00,//header 18
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        0x01,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_VEHICLE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x14, 0x00,//header 19
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        0x01,
        0x01,//item size
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x00,//action size
        0x00,
        0xCC, 0xCD, 0xAA, 0x0F
    },
};
uint8_t unit_ipi_result[][SHF_IPI_PROTOCOL_BYTES] = {
//add condition
    {
        0xAA, 0xF0, 0x05, 0x00,//header 0
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x06,
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x05, 0x00,//header 1
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x07,
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x05, 0x00,//header [wrong] 2
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x00,
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x05, 0x00,//header [wrong] 3
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x00,
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x05, 0x00,//header [wrong] 4
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x00,
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x05, 0x00,//header [wrong] 5
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x00,
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x05, 0x00,//header [wrong] 6
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x00,
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0x00, 0x00, 0x00, 0x00,//header [partial] 7
        0x00,//message type
        0x00,
        0x00,
        0x00, 0x00, 0x00, 0x00
    },
    {
        0xAA, 0xF0, 0x05, 0x00,//header [completed] 8
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x08,
        0xCC, 0xCD, 0xAA, 0x0F
    },
//add action
    {
        0xAA, 0xF0, 0x07, 0x00,//header 9
        SHF_AP_CONDITION_ACTION_ADD,//message type
        0xFF,
        0x02,//size
        0x02, 0x00,//array[wrong]
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 10
        SHF_AP_CONDITION_ACTION_ADD,//message type
        0xFF,
        0x02,//size
        0x03, 0x04,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x06, 0x00,//header 11
        SHF_AP_CONDITION_ACTION_ADD,//message type
        0xFF,
        0x01,//size
        0x00,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
//remove action
    {
        0xAA, 0xF0, 0x07, 0x00,//header 12
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x02,//size
        SHF_STATUS_OK, SHF_STATUS_OK,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x06, 0x00,//header 13
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x01,//size
        SHF_STATUS_ERROR,//array[wrong]
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x06, 0x00,//header 14
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x01,//size
        SHF_STATUS_ERROR,//array [wrong]
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x06, 0x00,//header 15
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x01,//size
        SHF_STATUS_OK,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x06, 0x00,//header 16
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x01,//size
        SHF_STATUS_OK,//array [wrong, but true for non-exist]
        0xCC, 0xCD, 0xAA, 0x0F
    },
//update condition
    {
        0xAA, 0xF0, 0x05, 0x00,//header 17
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        SHF_STATUS_OK,
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x05, 0x00,//header 18
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        SHF_STATUS_OK,
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x05, 0x00,//header 19
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        SHF_STATUS_OK,
        0xCC, 0xCD, 0xAA, 0x0F
    },
};
uint8_t unit_ipi_state[] = {
//add condition
    BUFFER_STATE_COMPLETED, //0
    BUFFER_STATE_COMPLETED, //1
    BUFFER_STATE_STARTED,  //2
    BUFFER_STATE_COMPLETED, //3
    BUFFER_STATE_COMPLETED, //4
    BUFFER_STATE_COMPLETED, //5
    BUFFER_STATE_COMPLETED, //6
    BUFFER_STATE_STARTED, //7
    BUFFER_STATE_COMPLETED, //8
//add action
    BUFFER_STATE_COMPLETED, //9
    BUFFER_STATE_COMPLETED, //10
    BUFFER_STATE_COMPLETED, //11
//remove action
    BUFFER_STATE_COMPLETED, //12
    BUFFER_STATE_COMPLETED, //13
    BUFFER_STATE_COMPLETED, //14
    BUFFER_STATE_COMPLETED, //15
    BUFFER_STATE_COMPLETED, //16
//update condition
    BUFFER_STATE_COMPLETED, //17
    BUFFER_STATE_COMPLETED, //18
    BUFFER_STATE_COMPLETED, //19
};

uint8_t unit_consys_data[][SHF_UART_PROTOCOL_BYTES] = {
    {
        0xAA, 0xF0, 0x07, 0x00, //0
        SHF_CONSYS_REGISTER_TIMEOUT,
        0xFF,
        0x01,
        0x0A, 0x00,
        0xCC, 0xCC, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x06, 0x00, //1
        SHF_CONSYS_REGISTER_CONTEXT,
        0xFF,
        0x01,
        0x3C,
        0xCC, 0xCC, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x06, 0x00, //2
        SHF_CONSYS_GET_MOTION_STATE,
        0xFF,
        0x00, 0x00,
        0xCC, 0xCC, 0xAA, 0x0F
    },
};
uint8_t unit_consys_result[][SHF_UART_PROTOCOL_BYTES] = {
    {
        0xAA, 0xF0, 0x05, 0x00, //0
        SHF_CONSYS_REGISTER_TIMEOUT,
        0xFF,
        SHF_STATUS_OK,
        0xCC, 0xCC, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x05, 0x00, //1
        SHF_CONSYS_REGISTER_CONTEXT,
        0xFF,
        SHF_STATUS_OK,
        0xCC, 0xCC, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x0E, 0x00, //2
        SHF_CONSYS_GET_MOTION_STATE,
        0xFF,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xCC, 0xCC, 0xAA, 0x0F
    },
};
uint8_t unit_consys_state[] = {
    BUFFER_STATE_COMPLETED, //0
    BUFFER_STATE_COMPLETED, //1
    BUFFER_STATE_COMPLETED, //2
};
void test_configurator_consys()
{
    shf_communicator_init();
    shf_condition_init(); //link condition
    shf_configurator_init(); //init protocol receiver
    for (int i = 0; i < 3; i++) {
        logv("--------------------------------\n"
             "test configurator consys loop %d\n"
             "--------------------------------\n", i);

        uint8_t* data =  unit_consys_data[i];
        size_t size = *(data + 2);
        //*((uint16_t*)(data + size + 6 - 4)) = checksum_get(data + 2, size);
        save_uint16_2_uint8(checksum_get(data + 2, size), data + size + 6 - 4);

        unit_clear_buffer();
        protocol_receive_message(&consys_protocol_buffer, data, size + 6);//send data
        unit_assert("buffer state", unit_consys_state[i], consys_protocol_buffer.state);

        unit_clear_cache();
        shf_communicator_run();//run
        unit_assert("send cache size", unit_send_offset - 6, convert_uint8_2_uint16(unit_consys_result[i] + 2));

        if (i >= 2) {//
            break;
        }

        size_t claim = convert_uint8_2_uint16(unit_consys_result[i] + 2);
        //*((uint16_t*)(unit_consys_result[i] + claim + 6 - 4)) = ;
        save_uint16_2_uint8(checksum_get(unit_consys_result[i] + 2, claim), unit_consys_result[i] + claim + 6 - 4);

        for (int j = 0; j < unit_send_offset; j++) {
            if (unit_send_cache[j] != unit_consys_result[i][j]) {
                char string_cache[64];
                memset(string_cache, 0, 64);
                sprintf(string_cache, "consys item %d, offset %d not match", i, j);
                unit_assert(string_cache, unit_consys_result[i][j], unit_send_cache[j]);
            }
        }
    }

}
void test_configurator_ipi()
{
    shf_communicator_init();
    shf_condition_init(); //link condition
    shf_configurator_init(); //init protocol receiver

    unit_clear_buffer();
    size_t lastSize = 0;
    for (int i = 0; i < 20; i++) {
        logv("--------------------------------\n"
             "test configurator ipi loop %d\n"
             "--------------------------------\n", i);
        uint8_t* data =  unit_ipi_data[i];
        size_t size = *(data + 2);

        size_t tempSize = 0;
        if (lastSize + 6 > SHF_IPI_PROTOCOL_BYTES) {
            tempSize = lastSize + 6 - SHF_IPI_PROTOCOL_BYTES;
            lastSize = 0;
            uint8_t tb[SHF_IPI_PROTOCOL_BYTES * 2];
            memset(tb, 0, SHF_IPI_PROTOCOL_BYTES * 2);
            memcpy(tb, unit_ipi_data[i - 1], SHF_IPI_PROTOCOL_BYTES);
            memcpy(tb + SHF_IPI_PROTOCOL_BYTES, unit_ipi_data[i], tempSize);
            //*((uint16_t*)(data + tempSize - 4)) = checksum_get(tb + 2, SHF_IPI_PROTOCOL_BYTES + tempSize - 6);
            save_uint16_2_uint8(checksum_get(tb + 2, SHF_IPI_PROTOCOL_BYTES + tempSize - 6), data + tempSize - 4);
        } else {
            if (size + 6 > SHF_IPI_PROTOCOL_BYTES) {
                tempSize = SHF_IPI_PROTOCOL_BYTES;
                lastSize = size;
            } else {
                tempSize = size + 6;
                //*((uint16_t*)(data + tempSize - 4)) = checksum_get(data + 2, size);
                //*((uint16_t*)(data + 2 + size)) = checksum_get(data + 2, size);
                save_uint16_2_uint8(checksum_get(data + 2, size), data + 2 + size);
            }
        }

        protocol_receive_message(&shf_protocol_buffer, data, tempSize);//send data
        unit_assert("buffer state", unit_ipi_state[i], shf_protocol_buffer.state);

        if (unit_ipi_state[i] == BUFFER_STATE_COMPLETED) {
            unit_clear_cache();
        }
        shf_communicator_run();//run
        if (unit_ipi_state[i] == BUFFER_STATE_COMPLETED) {
            size_t claim = convert_uint8_2_uint16(unit_ipi_result[i] + 2);
            //*((uint16_t*)(unit_ipi_result[i] + claim + 6 - 4)) = checksum_get(unit_ipi_result[i] + 2, claim);
            save_uint16_2_uint8(checksum_get(unit_ipi_result[i] + 2, claim), unit_ipi_result[i] + claim + 6 - 4);
            unit_assert("send cache size", unit_send_offset - 6, convert_uint8_2_uint16(unit_ipi_result[i] + 2));
            for (int j = 0; j < unit_send_offset; j++) {
                if (unit_send_cache[j] != unit_ipi_result[i][j]) {
                    char string_cache[64];
                    memset(string_cache, 0, 64);
                    sprintf(string_cache, "ipi item %d, offset %d not match", i, j);
                    unit_assert(string_cache, unit_ipi_result[i][j], unit_send_cache[j]);
                }
            }
//        } else {
//            unit_assert("send cache size", unit_send_offset, SHF_IPI_PROTOCOL_BYTES);
        }
    }
}

void shf_communicator_unit_test()
{
    logv("\n\n**********\n shf_communicator_unit_test begin\n");
    test_send_and_receive();
    test_configurator_ipi();
    test_configurator_consys();
}
#endif

#ifdef SHF_INTEGRATION_TEST_ENABLE
uint8_t it_ipi_data[][48] = {
//add condition
    {
        0xAA, 0xF0, 0x13, 0x00,//header 0
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0xA9, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x1F, 0x00,//header 1
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x02,//item size
        SHF_DATA_INDEX_ACTIVITY_FOOT, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        SHF_DATA_INDEX_PEDOMETER_COUNT, SHF_DATA_INDEX_PEDOMETER_DISTANCE, SHF_OPERATION_MASK_CMP | SHF_OPERATION_EQUAL, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0xFD, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x1F, 0x00,//header [wrong] 2
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0x07, 0x05, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x13, 0x00,//header 3
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x02,//item size [wrong]
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0xAA, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x13, 0x00,//header 4
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x02,//action size [wrong]
        0x01,//action
        0xAA, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x12, 0x00,//header 5
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value [wrong]
        0x01,//action size
        0x01,//action
        0xA8, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x17, 0x00,//header 6
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x05,//action size
        0x02, 0x03, 0x05, 0x04, 0x03,//action[wrong]
        0xAA, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x38, 0x00,//header 7
        SHF_AP_CONDITION_ADD,//message type
        0xFF,
        0x04,//item size
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        SHF_DATA_INDEX_ACTIVITY_BIKE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value 8
        0x02,//action size
        0x02, 0x03,//action
        0xCC, 0xCD, 0xAA, 0x0F
    },
//add action
    {
        0xAA, 0xF0, 0x08, 0x00,//header 9
        SHF_AP_CONDITION_ACTION_ADD,//message type
        0xFF,
        0x06,//CID
        0x02,//size
        0x01, 0x08,//array[wrong]
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x08, 0x00,//header 10
        SHF_AP_CONDITION_ACTION_ADD,//message type
        0xFF,
        0x06,//CID
        0x02,//size
        0x04, 0x03,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 11
        SHF_AP_CONDITION_ACTION_ADD,//message type
        0xFF,
        0x06,//CID
        0x01,//size
        0x02,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
//remove action
    {
        0xAA, 0xF0, 0x08, 0x00,//header 12
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x06,//CID
        0x02,//size
        0x01, 0x03,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 13
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x06,//CID
        0x01,//size
        0x00,//array[wrong]
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 14
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x06,//CID
        0x01,//size
        0x05,//array [wrong]
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 15
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x06,//CID
        0x01,//size
        0x02,//array
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x07, 0x00,//header 16
        SHF_AP_CONDITION_ACTION_REMOVE,//message type
        0xFF,
        0x06,//CID
        0x01,//size
        0x02,//array [wrong, but true for non-exist]
        0xCC, 0xCD, 0xAA, 0x0F
    },
//update condition
    {
        0xAA, 0xF0, 0x14, 0x00,//header 17
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        0x01,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_VEHICLE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x14, 0x00,//header 18
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        0x01,
        0x01,//item size
        SHF_DATA_INDEX_ACTIVITY_VEHICLE, 0x00, SHF_OPERATION_ANY, 0x00,//item
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        0x01,//action
        0xCC, 0xCD, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x14, 0x00,//header 19
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        0x01,
        0x01,//item size
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x00,//action size
        0x00,
        0xCC, 0xCD, 0xAA, 0x0F
    },
//touch condition
    {
        0xAA, 0xF0, 0x14, 0x00,//header 0
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        SHF_CONDITION_RESERVED_TOUCH_PANEL_ON,
        0x01,//item size
        SHF_DATA_INDEX_INPOCKET_VALUE, 0x00, SHF_OPERATION_MORE_THAN, 0x00,//item
        0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        SHF_ACTION_ID_TOUCH_DEACTIVE | SHF_ACTION_MASK_REPEAT,//action
        0xA9, 0x02, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x14, 0x00,//header 0
        SHF_AP_CONDITION_UPDATE,//message type
        0xFF,
        SHF_CONDITION_RESERVED_TOUCH_PANEL_OFF,
        0x01,//item size
        SHF_DATA_INDEX_INPOCKET_VALUE, 0x00, SHF_OPERATION_LESS_THAN_OR_EQUAL, 0x00,//item
        0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,//value
        0x01,//action size
        SHF_ACTION_ID_TOUCH_ACTIVE | SHF_ACTION_MASK_REPEAT,//action
        0xA9, 0x02, 0xAA, 0x0F
    },
};
uint8_t it_consys_data[][SHF_UART_PROTOCOL_BYTES] = {
    {
        0xAA, 0xF0, 0x07, 0x00, //0
        SHF_CONSYS_REGISTER_TIMEOUT,
        0xFF,
        0x01,
        0x0A, 0x00,
        0xCC, 0xCC, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x06, 0x00, //1
        SHF_CONSYS_REGISTER_CONTEXT,
        0xFF,
        0x01,
        0x3C,
        0xCC, 0xCC, 0xAA, 0x0F
    },
    {
        0xAA, 0xF0, 0x06, 0x00, //2
        SHF_CONSYS_GET_MOTION_STATE,
        0xFF,
        0x00, 0x00,
        0x5F, 0x01, 0xAA, 0x0F
    },
};
//0, 3
void it_configurator_consys(int base, int max)
{
    for (int i = base; i < max; i++) {
        logv("--------------------------------\n"
             "test configurator consys loop %d\n"
             "--------------------------------\n", i);

        uint8_t* data =  it_consys_data[i];
        size_t size = *(data + 2);
        //*((uint16_t*)(data + size + 6 - 4)) = checksum_get(data + 2, size);
        save_uint16_2_uint8(checksum_get(data + 2, size), data + size + 6 - 4);
#ifdef SHF_CONSYS_TYPE_UART
        consys_protocol_receive_handler(data, size + 6);
#else
        consys_protocol_receive_handler(data, size + 6);
#endif
    }

}
//0, 20
void it_configurator_ipi(int base, int max)
{
    size_t lastSize = 0;
    for (int i = base; i < max; i++) {
        logv("--------------------------------\n"
             "test configurator ipi loop %d\n"
             "--------------------------------\n", i);
        uint8_t* data =  it_ipi_data[i];
        size_t size = *(data + 2);

        size_t tempSize = 0;
        if (lastSize + 6 > SHF_IPI_PROTOCOL_BYTES) {
            tempSize = lastSize + 6 - SHF_IPI_PROTOCOL_BYTES;
            lastSize = 0;
            uint8_t tb[SHF_IPI_PROTOCOL_BYTES * 2];
            memset(tb, 0, SHF_IPI_PROTOCOL_BYTES * 2);
            memcpy(tb, it_ipi_data[i - 1], SHF_IPI_PROTOCOL_BYTES);
            memcpy(tb + SHF_IPI_PROTOCOL_BYTES, it_ipi_data[i], tempSize);
            //*((uint16_t*)(data + tempSize - 4)) = checksum_get(tb + 2, SHF_IPI_PROTOCOL_BYTES + tempSize - 6);
            save_uint16_2_uint8(checksum_get(tb + 2, SHF_IPI_PROTOCOL_BYTES + tempSize - 6), data + tempSize - 4);
        } else {
            if (size + 6 > SHF_IPI_PROTOCOL_BYTES) {
                tempSize = SHF_IPI_PROTOCOL_BYTES;
                lastSize = size;
            } else {
                tempSize = size + 6;
                //*((uint16_t*)(data + 2 + size)) = checksum_get(data + 2, size);
                save_uint16_2_uint8(checksum_get(data + 2, size), data + 2 + size);
            }
        }
        shf_protocol_receive_handler(IPI_SHF, data, tempSize);
    }
}

size_t shf_ipi_offset = 6;
size_t it_ipi_test[] = {
    //test communicator, configurator, condition and so on
    0, 1,//add condition
    10, 11,//add action
    15, 16,//remove action
    //test active/deactive sensor@{
    17, 18,//update condition
    19, 20,//remove update
    //@}
    //test touch @{
    20, 21,//add enable touch
    21, 22,//add disable touch
    //@}
};
size_t shf_consys_offset = 0;
size_t it_consys_test[] = {
    0, 1,
    1, 2,
    2, 3,
    3, 4,
};
void shf_communicator_it()
{
//    it_configurator_ipi(0, 1);//add condition
//    it_configurator_ipi(10, 11);//add action
//    it_configurator_ipi(15, 16);//remove actino
//    it_configurator_ipi(17, 18);//update condition
//    it_configurator_ipi(19, 20);//update condition
    //    logv("********************************************************\n");
//    if (shf_ipi_offset < 10) {
//        it_configurator_ipi(it_ipi_test[shf_ipi_offset + 0], it_ipi_test[shf_ipi_offset + 1]);
//        shf_ipi_offset += 2;
//    } else {
//        shf_ipi_offset = 0;
//    }
    if (shf_consys_offset < 8) {
        it_configurator_consys(it_consys_test[shf_consys_offset + 0], it_consys_test[shf_consys_offset + 1]);
        shf_consys_offset += 2;
//    } else {
//        shf_consys_offset = 0;
    }
}
#endif
