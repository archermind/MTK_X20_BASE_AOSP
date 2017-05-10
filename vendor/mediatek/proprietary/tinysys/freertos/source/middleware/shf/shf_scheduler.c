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
#include "shf_condition.h"
#include "shf_configurator.h"
#include "shf_data_pool.h"
#include "shf_debug.h"
#include "shf_process.h"
#include "shf_scheduler.h"
#include "shf_sensor.h"

#include <FreeRTOS.h>
#include <queue.h>
#include <utils.h>

#include <mt_gpt.h>

uint8_t shf_state;

QueueHandle_t xQueue;
msg_data mSensor_data = {0};

//for test, need delete
/*int8_t it_ipi_rece_data[25] = {0xAA,0xF0,0x13,0x00,0x80,0x01,0x01,0x01,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x1C,0x00,0x05,0x00,0x01,0x81,0x39,0x01,0xAA,0x0F
                               };
*/
//uint8_t it_ipi_rece_data_ges[12] = {0xAA,0xF0,0x06,0x00,0x89,0x01,0x1C,0x26,0xD2,0x00,0xAA,0x0F};

//extern void shf_protocol_receive_handler(int id, void * data, uint size);
//void onSensorChanged_shf(int sensor, struct data_unit_t values);

//e

static void shf_scheduler_run_one()
{
#ifdef SHF_DEBUG_PERFORMENCE
    uint32_t start_time = delay_get_current_tick();
#endif
    shf_communicator_run(); //process cached communicator buffer,parse condition
    shf_condition_config();//communicator may config condition, so check it
    shf_sensor_config();//setup sensor for config changed,poll data as to compare next.

#ifdef SHF_DEBUG_PERFORMENCE
    uint32_t end_time = delay_get_current_tick();
    logd("s_run: %u\n", end_time - start_time);
#endif
}

void shf_scheduler_notify(int type, struct data_unit_t data)
{
    logd("s_notify: t=%lld\n", timestamp_get_ns());
    portBASE_TYPE taskWoken = pdFALSE;
    BaseType_t MSG_RET;
    if(xQueue != 0) {
        msg_data *mData;
        mSensor_data.type = type;
        mSensor_data.data = &data;
        mData = &mSensor_data;
        //xQueueSend(xQueue, (void*) &mData, (TickType_t) 0);
        MSG_RET = xQueueSendFromISR(xQueue, (void*)&mData, &taskWoken);
        if (MSG_RET != pdPASS) {
            logd( "xQueueSendFromISR failed!!\n\r");
            return;
        }
    }
}

//void shf_notify_test()
//{
//send c,a,then notify
//logd("call shf_notifyt_test,it means receive data from android,will giev semaphore\n");
// 1. test receive IPI from android.will notify
// shf_protocol_receive_handler(17,(void*)it_ipi_rece_data,25);


// 2. test onSensorChanged from sensor manager.will notify.
// logd("call shf_notify_test,fill in test data,inpocket 28,value=1");
/* struct data_unit_t test_data = {0};
 test_data.value[0] = 1;
 onSensorChanged_shf(28,test_data);
*/

//3.test only configurable gesture
//shf_protocol_receive_handler(17,(void*)it_ipi_rece_data_ges,12);
//}

void shf_scheduler_run()
{
    //create a queue capable of containing 10 pointers to MsgData structures.
    //these should be passed by pointer as they contain a lot of data.
    xQueue = xQueueCreate(10,sizeof(msg_data *));
    msg_data *pxRxedMsg;
    while (1) {
        logd("s_run:t1=%lld\n",timestamp_get_ns());
        if(xQueue != 0) {
            //will block infinity.pxRxedMsg new points to the struct MsgData variable posted by notify.
            if(xQueueReceive(xQueue, &(pxRxedMsg), (TickType_t)portMAX_DELAY)) {
                logd("s_run:t2=%lld,type=%d\n",timestamp_get_ns(),pxRxedMsg->type);
                switch(pxRxedMsg->type) {
                    case MSG_TYPE_CONDITION:
                        shf_scheduler_run_one();
                        break;
                    case MSG_TYPE_GESTURE:
                        //parse gesture msg from android IPI
                        shf_communicator_run();
                        break;
                    case MSG_TYPE_VALID:
                        break;
                    default://onSensorChanged
                        logv("will call shf_process_save..\n");
                        shf_data_pool_run();//clear data pool flag for next steps
                        shf_process_save(pxRxedMsg->type, *(pxRxedMsg->data));
                        //map sensor type to dindex.only from onSensorChanged to run.check condition and run actions
                        shf_condition_run();
                        break;
                }
            }
        }
    }
}

void shf_scheduler_set_state(uint8_t state)
{
    logd("s_state: s=%d,t=%ld\n", state, timestamp_get_ns());
    shf_state = state;
}
uint8_t get_shf_state()
{
    return shf_state;
}

void shf_scheduler_init()
{
    shf_state = 0;
    shf_condition_init(); //link condition
    shf_sensor_init(); //sensor manager callback.
    shf_configurator_init(); //init protocol receiver
}
