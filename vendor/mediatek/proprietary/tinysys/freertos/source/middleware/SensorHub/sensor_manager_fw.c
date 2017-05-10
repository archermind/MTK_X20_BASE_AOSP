/* MediaTek Inc. (C) 2015. All rights reserved.
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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include "sensor_manager_fw.h"
#include "sensor_manager.h"
#define SM_TAG                  "[SensorFramework]"
//#define SF_DEBUG
#ifdef SF_DEBUG
#define SM_ERR(fmt, args...)    PRINTF_D(SM_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define SM_LOG(fmt, args...)    PRINTF_D(SM_TAG fmt, ##args)
#else
#define SM_ERR(fmt, args...)    PRINTF_D(SM_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define SM_LOG(fmt, args...)
#endif


//AP, PDCA, LPF and 2 reserved
struct TaskInformationStruct gTaskInformation[SENSOR_TASK_MAX_COUNT];
INT16 gDelay[SENSOR_TYPE_MAX_COUNT] = {0};  //ms
INT16 gTimeout[SENSOR_TYPE_MAX_COUNT] = {0};  //ms
INT32 genablecount[SENSOR_TYPE_MAX_COUNT] = {0};
INT32 gapcm4count[SENSOR_TYPE_MAX_COUNT] = {0};

static UINT8 xSensorTypeMappingToSCP(UINT8 sensortype)
{
    sensortype = sensortype + 1;

    if (sensortype == SENSOR_TYPE_GLANCE_GESTURE)
        sensortype = SENSOR_TYPE_SNAPSHOT;

    if (sensortype == SENSOR_TYPE_PICK_UP_GESTURE)
        sensortype = SENSOR_TYPE_PICK_UP;

    if (sensortype == SENSOR_TYPE_ANSWER_CALL_GESTURE)
        sensortype = SENSOR_TYPE_ANSWER_CALL;


    return (sensortype);
}
UINT8 xSensorTypeMappingToAP(UINT8 sensortype)
{
    if (sensortype == SENSOR_TYPE_SNAPSHOT)
        sensortype = SENSOR_TYPE_GLANCE_GESTURE;
    if (sensortype == SENSOR_TYPE_PICK_UP)
        sensortype = SENSOR_TYPE_PICK_UP_GESTURE;
    if (sensortype == SENSOR_TYPE_ANSWER_CALL)
        sensortype = SENSOR_TYPE_ANSWER_CALL_GESTURE;
    return (sensortype - 1);
}

static void xAPonSensorChanged(int sensor, struct data_unit_t values)
{
    return;
}
static void xAPonAccuracyChanged(int sensor, int accuracy)
{
    return;
}

static int xFrameworkIpiNotifyAP(UINT8 sensortype, UINT8 action, UINT8 event, INT8 err)
{
    UINT8 ret = 0, ap_sensor_id = 0;
    SCP_SENSOR_HUB_DATA ipiHandlerRsp;
    ipi_status ipi_ret;
    struct data_unit_t *data_gyro;

    ap_sensor_id = xSensorTypeMappingToAP(sensortype);

    switch (action) {
        case SENSOR_HUB_ACTIVATE:
        case SENSOR_HUB_SET_DELAY:
        case SENSOR_HUB_BATCH:
        case SENSOR_HUB_BATCH_TIMEOUT:
        case SENSOR_HUB_SET_CONFIG:
        case SENSOR_HUB_SET_TIMESTAMP:
        case SENSOR_HUB_SET_CUST:
            ipiHandlerRsp.rsp.sensorType = ap_sensor_id;
            ipiHandlerRsp.rsp.action     = action;
            ipiHandlerRsp.rsp.errCode = 0;
            ret = 0;
            break;
        case SENSOR_HUB_GET_DATA:
            ipiHandlerRsp.get_data_rsp.sensorType = ap_sensor_id;
            ipiHandlerRsp.get_data_rsp.action = action;
            ipiHandlerRsp.rsp.errCode = 0;
            //TODO:need mutex to protect
            //the max size of data_unit_t for AP to used is 36bytes, so we do memcpy directly
            SM_LOG("INFO: SF scp_ipi_send SENSOR_HUB_GET_DATA(size:(%d))!!\n\r", sizeof(struct data_unit_t));
            xSemaphoreTake(xSMSemaphore[sensortype], portMAX_DELAY);
            //TODO: need to check memcpy size of this API
            if (sensortype == SENSOR_TYPE_GYROSCOPE) {
                memcpy(ipiHandlerRsp.get_data_rsp.data.int8_Data, gAlgorithm[sensortype].newest->data, 44);
                data_gyro = (struct data_unit_t *)ipiHandlerRsp.get_data_rsp.data.int8_Data;
                data_gyro->gyroscope_t.x = data_gyro->gyroscope_t.x * GYROSCOPE_INCREASE_NUM_AP / GYROSCOPE_INCREASE_NUM_SCP;
                data_gyro->gyroscope_t.y = data_gyro->gyroscope_t.y * GYROSCOPE_INCREASE_NUM_AP / GYROSCOPE_INCREASE_NUM_SCP;
                data_gyro->gyroscope_t.z = data_gyro->gyroscope_t.z * GYROSCOPE_INCREASE_NUM_AP / GYROSCOPE_INCREASE_NUM_SCP;
                data_gyro->gyroscope_t.x_bias = data_gyro->gyroscope_t.x_bias * GYROSCOPE_INCREASE_NUM_AP / GYROSCOPE_INCREASE_NUM_SCP;
                data_gyro->gyroscope_t.y_bias = data_gyro->gyroscope_t.y_bias * GYROSCOPE_INCREASE_NUM_AP / GYROSCOPE_INCREASE_NUM_SCP;
                data_gyro->gyroscope_t.z_bias = data_gyro->gyroscope_t.z_bias * GYROSCOPE_INCREASE_NUM_AP / GYROSCOPE_INCREASE_NUM_SCP;
            } else {
                if (NULL == gAlgorithm[sensortype].newest)
                    break;
                memcpy(ipiHandlerRsp.get_data_rsp.data.int8_Data, gAlgorithm[sensortype].newest->data, 44);
            }
            xSemaphoreGive(xSMSemaphore[sensortype]);
            ret = 0;
            break;
        case SENSOR_HUB_NOTIFY:
            switch (event) {
                case BATCH_TIMEOUT_NOTIFY:
                case BATCH_WAKEUP_NOTIFY:
                    ipiHandlerRsp.notify_rsp.sensorType = ap_sensor_id;
                    ipiHandlerRsp.notify_rsp.action = SENSOR_HUB_NOTIFY;
                    ipiHandlerRsp.notify_rsp.event = SCP_BATCH_TIMEOUT;
                    break;
                case BATCH_DRAMFULL_NOTIFY:
                    ipiHandlerRsp.notify_rsp.sensorType = ap_sensor_id;
                    ipiHandlerRsp.notify_rsp.action = SENSOR_HUB_NOTIFY;
                    ipiHandlerRsp.notify_rsp.event = SCP_FIFO_FULL;
                    break;
                case DIRECT_PUSH_NOTIFY:
                    ipiHandlerRsp.notify_rsp.sensorType = ap_sensor_id;
                    ipiHandlerRsp.notify_rsp.action = SENSOR_HUB_NOTIFY;
                    ipiHandlerRsp.notify_rsp.event = SCP_DIRECT_PUSH;
                    break;
                case INTR_NOTIFY:
                    ipiHandlerRsp.notify_rsp.sensorType = ap_sensor_id;
                    ipiHandlerRsp.notify_rsp.action = SENSOR_HUB_NOTIFY;
                    ipiHandlerRsp.notify_rsp.event = SCP_NOTIFY;
                    xSemaphoreTake(xSMSemaphore[sensortype], portMAX_DELAY);
                    //TODO: need to check memcpy size of this API

                    memcpy(ipiHandlerRsp.notify_rsp.data.int8_Data, gAlgorithm[sensortype].newest->data, 44);
                    xSemaphoreGive(xSMSemaphore[sensortype]);
                    break;
            }
            SM_LOG("SENSOR_HUB_NOTIFY (TYPE:(%d))!!\n\r", sensortype);
            ret = 0;
            break;
        case SENSOR_HUB_POWER_NOTIFY:
            ipiHandlerRsp.notify_rsp.sensorType = xSensorTypeMappingToAP(sensortype);
            ipiHandlerRsp.notify_rsp.action = SENSOR_HUB_POWER_NOTIFY;
            SM_LOG("SENSOR_HUB_NOTIFY (TYPE:(%d))!!\n\r", sensortype);
            ret = 0;
            break;
        default:
            break;

    }

    //TODO: 48bytes
    ipi_ret = scp_ipi_send(IPI_SENSOR, (void *)&ipiHandlerRsp, 48, 1, IPI_SCP2AP);
    if (ipi_ret != DONE) {
        SM_ERR("Error: SF scp_ipi_send failed!!\n\r");
        return SM_ERROR;
    }
    return ret;
}

//pick the fastest delay for this sensor
static int xSensorFrameworkChooseFastestDelay(UINT8 sensor)
{
    int i, delay = SM_ERROR;

    for (i = 0; i < SENSOR_TASK_MAX_COUNT; i++) {
        if (delay == SM_ERROR)
            delay = gTaskInformation[i].delay[sensor];
        else if (delay > gTaskInformation[i].delay[sensor])
            delay = gTaskInformation[i].delay[sensor];
    }
    return delay;
}
static void clear_bit(UINT64 *bitmap, UINT8 sensortype)
{
    //taskENTER_CRITICAL();
    (*bitmap) &= ~(1ULL << sensortype);
    //taskEXIT_CRITICAL();
}
static void set_bit(UINT64 *bitmap, UINT8 sensortype)
{
    //taskENTER_CRITICAL();
    (*bitmap) |= (1ULL << sensortype);
    //taskEXIT_CRITICAL();
}
static int test_bit(UINT64 *bitmap, UINT8 sensortype)
{
    //taskENTER_CRITICAL();
    if (((*bitmap) & (1ULL << sensortype)) != 0)
        return 1;
    else
        return 0;
    //taskEXIT_CRITICAL();
}
static int xSensorSendEnableActivateQueue(UINT8 sensortype, int enable, UINT64 enable_bitmap)
{
    BaseType_t MSG_RET;
    struct SensorManagerQueueEventStruct event;

    event.action = SF_ACTIVATE;
    event.info.sensortype = sensortype;
    event.info.data[0] = enable;
    event.info.task_handler = 0;
    event.info.bit_map = enable_bitmap;

    MSG_RET = xQueueSendToFront(gSensorManagerQueuehandle, &event, 0);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: xSensorFrameworkSetActivate xQueueSend failed!!\n\r");
        return -1;
    }
    SM_LOG("xSensorFrameworkSetActivate xQueueSend SUCCESS!!\n\r");
    return SM_SUCCESS;
}
static int xSensorSendApBatchTimeoutQueue(UINT8 sensortype)
{
    BaseType_t MSG_RET;
    struct SensorManagerQueueEventStruct event;

    event.action = SM_BATCH_FLUSH;
    event.info.sensortype = sensortype;

    MSG_RET = xQueueSendToFront(gSensorManagerQueuehandle, &event, 0);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: xSensorFrameworkSetActivate xQueueSend failed!!\n\r");
        return -1;
    }
    SM_LOG("xSensorFrameworkSetActivate xQueueSend SUCCESS!!\n\r");
    return SM_SUCCESS;
}
static int xSensorSendSetDelayQueue(UINT8 sensortype, int delay, int enable, UINT64 delay_bitmap)
{
    BaseType_t MSG_RET;
    struct SensorManagerQueueEventStruct event;

    event.action = SF_SET_DELAY;
    event.info.sensortype = sensortype;
    event.info.data[0] = delay;
    event.info.data[1] = enable;
    event.info.task_handler = 0;
    event.info.bit_map = delay_bitmap;

    MSG_RET = xQueueSendToFront(gSensorManagerQueuehandle, &event, 0);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: xSensorSendSetDelayQueue xQueueSend failed!!\n\r");
        return -1;
    }
    SM_LOG("xSensorSendSetDelayQueue xQueueSend SUCCESS!!\n\r");
    return SM_SUCCESS;
}
static int xSensorSendDirectPushTimerQueue(UINT8 sensortype, int enable)
{
    BaseType_t MSG_RET;
    struct SensorManagerQueueEventStruct event;

    event.action = SF_DIRECT_PUSH;
    event.info.sensortype = sensortype;
    event.info.data[0] = enable;

    MSG_RET = xQueueSendToFront(gSensorManagerQueuehandle, &event, 0);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: xSensorSendDirectPushTimerQueue xQueueSend failed!!\n\r");
        return -1;
    }
    SM_LOG("xSensorSendDirectPushTimerQueue xQueueSend SUCCESS!!\n\r");
    return SM_SUCCESS;
}
static int xSensorSendSetBatchDelayQueue(UINT8 sensortype, int flag, int delay,
        int batch_timeout, int enable, UINT64 batch_delay_bitmap)
{
    BaseType_t MSG_RET;
    struct SensorManagerQueueEventStruct event;

    event.action = SF_BATCH;
    event.info.sensortype = sensortype;
    event.info.task_handler = 0;
    event.info.data[0] = flag;
    event.info.data[1] = delay;
    event.info.data[2] = batch_timeout;
    event.info.data[3] = enable;
    event.info.bit_map = batch_delay_bitmap;

    MSG_RET = xQueueSend(gSensorManagerQueuehandle, &event, 0);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: xSensorFrameworkSetBatch xQueueSend failed!!\n\r");
        return -1;
    }
    return SM_SUCCESS;
}
static int xSensorSendDirectPushDelayQueue(UINT8 sensortype, int flag, int delay,
        int batch_timeout, int enable, UINT64 batch_delay_bitmap)
{
    BaseType_t MSG_RET;
    struct SensorManagerQueueEventStruct event;

    event.action = SF_DIRECT_PUSH_DELAY;
    event.info.sensortype = sensortype;
    event.info.task_handler = 0;
    event.info.data[0] = flag;
    event.info.data[1] = delay;
    event.info.data[2] = batch_timeout;
    event.info.data[3] = enable;
    event.info.bit_map = batch_delay_bitmap;

    MSG_RET = xQueueSend(gSensorManagerQueuehandle, &event, 0);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: xSensorFrameworkSetBatch xQueueSend failed!!\n\r");
        return -1;
    }
    return SM_SUCCESS;
}

static void xSetEnableDisableState(int handle, UINT8 sensortype, UINT8 head_sensor,
                                   UINT64 *bit_map, UINT64 *delay_bitmap, int enable)
{
    SM_LOG("xSetEnableDisableState+++, type:%d, gsensorcount: %d, enable:%d, bitmap:0x%x\r\n",
           sensortype, genablecount[sensortype], enable, *bit_map);
    if (enable == SENSOR_ENABLE) {
        if (genablecount[sensortype] == 0) {
            set_bit(bit_map, sensortype);
        } else {

        }
        /*set_bit(delay_bitmap, sensortype);*/
        set_bit(&gAlgorithm[sensortype].enable, head_sensor);
        genablecount[sensortype]++;
    } else {
        if (genablecount[sensortype] == 1) {
            set_bit(bit_map, sensortype);
            genablecount[sensortype]--;
        } else if (genablecount[sensortype] == 0) {
            clear_bit(bit_map, sensortype);
            SM_LOG("this branch is used to avoid disable sensor before enable when bootup!!\n\r");
        } else {
            //clear_bit(bit_map, sensortype);
            genablecount[sensortype]--;
            configASSERT((genablecount[sensortype] < 0 ? 0 : 1));//< 0 stands for disable enable not match, so assert
            SM_LOG("xSensorFrameworkSetActivate 2 (handle:%d), sensor:(%d), enable(%d), genablecount(0x%x)\n\r", handle, sensortype,
                   enable, genablecount[sensortype]);
        }
        clear_bit(&gAlgorithm[sensortype].enable, head_sensor);
        set_bit(delay_bitmap, sensortype);
    }
    SM_LOG("xSetEnableDisableState---, type:%d, gsensorcount: %d, enable:%d, bitmap:0x%x\r\n",
           sensortype, genablecount[sensortype], enable, *bit_map);
}
static void xCheckInputSensorEnableState(int handle, UINT8 sensortype, UINT64 *bit_map,
        UINT64 *delay_bitmap, int enable)
{
    struct input_list_t *enable_list = NULL;
    struct input_list_t *sub_list = NULL;
    struct input_list_t *fast_list = NULL;
    struct input_list_t *low_list = NULL;

    UINT8 head_sensor = sensortype;
    UINT8 input_sensor = 0;
    UINT8 input_sub_sensor = 0;
    SM_LOG("xCheckInputSensorState+++, type:%d, gsensorcount: %d, enable:%d, bitmap:0x%x\r\n",
           head_sensor, genablecount[head_sensor], enable, *bit_map);
    if (NULL == gAlgorithm[head_sensor].algo_desp.input_list) {
        SM_LOG("xCheckInputSensorState***, type:%d, bitmap:0x%x, gsensorcount: %d, enable:%d\r\n",
               head_sensor, *bit_map, genablecount[head_sensor], enable);
        return;
    }

    for (enable_list = gAlgorithm[head_sensor].algo_desp.input_list;
            enable_list != NULL; enable_list = enable_list->next_input) {
        input_sensor = enable_list->input_type;
        xSetEnableDisableState(handle, input_sensor, head_sensor, bit_map, delay_bitmap, enable);
    }

    SM_LOG("xCheckInputSensorState!!!, type:%d, bitmap:0x%x, gsensorcount: %d, enable:%d\r\n",
           head_sensor, *bit_map, genablecount[head_sensor], enable);

    enable_list = gAlgorithm[head_sensor].algo_desp.input_list;

    for (fast_list = enable_list->next_input, low_list = enable_list; low_list != NULL;
            fast_list = fast_list->next_input, low_list = low_list->next_input) {
        input_sensor = low_list->input_type;
        if (gAlgorithm[input_sensor].algo_desp.input_list != NULL) {
            for (sub_list = gAlgorithm[input_sensor].algo_desp.input_list;
                    sub_list != NULL; sub_list = sub_list->next_input) {
                input_sub_sensor = sub_list->input_type;
                xSetEnableDisableState(handle, input_sub_sensor, input_sensor, bit_map, delay_bitmap, enable);
            }
        }
        if (fast_list == NULL && low_list != NULL)
            break;
    }
    /*do {
        input_sensor = enable_list->input_type;
        if (gAlgorithm[input_sensor].algo_desp.input_list != NULL) {

            for (sub_list = gAlgorithm[input_sensor].algo_desp.input_list->next_input;
                sub_list != NULL; sub_list = sub_list->next_input) {
                input_sub_sensor = sub_list->input_type;
                xSetEnableDisableState(handle, input_sub_sensor, bit_map, enable);
            }
        }
        if (enable_list->next_input != NULL)
            enable_list = enable_list->next_input;
        SM_LOG("xCheckInputSensorState~~~, type:%d, bitmap:%lld, gsensorcount: %d, enable:%d\r\n",
            input_sensor, *bit_map, genablecount[input_sensor], enable);
    } while (NULL != enable_list->next_input);*/
    SM_LOG("xCheckInputSensorState---, type:%d, bitmap:0x%x\r\n",
           head_sensor, *bit_map);
}
//judgement enable of all gTaskInformation[i].enable
static void xSetDelayState(int handle, UINT8 sensortype, UINT8 head_sensor,
                           UINT64 *bit_map, int delay)
{
    SM_LOG("xSetDelayState+++, type:%d, bitmap:0x%x\r\n",
           sensortype, *bit_map);
    set_bit(bit_map, sensortype);
    set_bit(&gAlgorithm[sensortype].enable, head_sensor);
    SM_LOG("xSetDelayState---, type:%d, bitmap:0x%x\r\n",
           sensortype, *bit_map);
}
static void xCheckInputSensorDelayState(int handle, UINT8 sensortype, UINT64 *bit_map, int enable)
{
    struct input_list_t *enable_list = NULL;
    struct input_list_t *sub_list = NULL;
    struct input_list_t *fast_list = NULL;
    struct input_list_t *low_list = NULL;

    UINT8 head_sensor = sensortype;
    UINT8 input_sensor = 0;
    UINT8 input_sub_sensor = 0;
    SM_LOG("xCheckInputSensorDelayState+++, type:%d, bitmap:0x%x\r\n",
           head_sensor, *bit_map);
    if (NULL == gAlgorithm[head_sensor].algo_desp.input_list) {
        SM_LOG("xCheckInputSensorDelayState***, type:%d, bitmap:0x%x\r\n",
               head_sensor, *bit_map);
        return;
    }

    for (enable_list = gAlgorithm[head_sensor].algo_desp.input_list;
            enable_list != NULL; enable_list = enable_list->next_input) {
        input_sensor = enable_list->input_type;
        xSetDelayState(handle, input_sensor, head_sensor, bit_map, enable);
    }

    SM_LOG("xCheckInputSensorDelayState!!!, type:%d, bitmap:0x%x\r\n",
           head_sensor, *bit_map);

    enable_list = gAlgorithm[head_sensor].algo_desp.input_list;

    for (fast_list = enable_list->next_input, low_list = enable_list; low_list != NULL;
            fast_list = fast_list->next_input, low_list = low_list->next_input) {
        input_sensor = low_list->input_type;
        if (gAlgorithm[input_sensor].algo_desp.input_list != NULL) {
            for (sub_list = gAlgorithm[input_sensor].algo_desp.input_list;
                    sub_list != NULL; sub_list = sub_list->next_input) {
                input_sub_sensor = sub_list->input_type;
                xSetDelayState(handle, input_sub_sensor, input_sensor, bit_map, enable);
            }
        }
        if (fast_list == NULL && low_list != NULL)
            break;
    }
    /*do {
        input_sensor = enable_list->input_type;
        if (gAlgorithm[input_sensor].algo_desp.input_list != NULL) {

            for (sub_list = gAlgorithm[input_sensor].algo_desp.input_list->next_input;
                sub_list != NULL; sub_list = sub_list->next_input) {
                input_sub_sensor = sub_list->input_type;
                xSetEnableDisableState(handle, input_sub_sensor, bit_map, enable);
            }
        }
        if (enable_list->next_input != NULL)
            enable_list = enable_list->next_input;
        SM_LOG("xCheckInputSensorState~~~, type:%d, bitmap:%lld, gsensorcount: %d, enable:%d\r\n",
            input_sensor, *bit_map, genablecount[input_sensor], enable);
    } while (NULL != enable_list->next_input);*/
    SM_LOG("xCheckInputSensorState---, type:%d, bitmap:0x%x\r\n",
           head_sensor, *bit_map);
}

static int xSensorFrameworkSetActivate(int handle, UINT8 sensortype, int enable)
{
    UINT64 enable_bit_map = 0;
    UINT64 delay_bit_map = 0;
    int ret = 0;

    /*if (SENSOR_TYPE_TILT_DETECTOR == sensortype || SENSOR_TYPE_SNAPSHOT == sensortype
        || SENSOR_TYPE_PICK_UP == sensortype || SENSOR_TYPE_ANSWER_CALL == sensortype)
        gDelay[sensortype] = 20;*/
    SM_LOG("xSensorFrameworkSetActivate++++++ (handle:%d),sensor:(%d), enable(%d), genablecount(0x%x)\n\r", handle,
           sensortype, enable, genablecount[sensortype]);
    switch (sensortype) {
        case SENSOR_TYPE_ACCELEROMETER:
        case SENSOR_TYPE_GYROSCOPE:
        case SENSOR_TYPE_PRESSURE:
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SENSOR_TYPE_LIGHT:
        case SENSOR_TYPE_PROXIMITY:
            xSetEnableDisableState(handle, sensortype, sensortype, &enable_bit_map, &delay_bit_map, enable);
            if (delay_bit_map != 0)
                xSensorSendSetDelayQueue(sensortype, gDelay[sensortype], enable, delay_bit_map);
            if (enable_bit_map != 0) {
                ret = xSensorSendEnableActivateQueue(sensortype, enable, enable_bit_map);
                if (ret < 0) {
                    SM_ERR("gsensorSendEnableActivateQueue fail!!\n\r");
                    return -1;
                }
            }
            break;
        default:
            xSetEnableDisableState(handle, sensortype, sensortype, &enable_bit_map, &delay_bit_map, enable);
            xCheckInputSensorEnableState(handle, sensortype, &enable_bit_map, &delay_bit_map, enable);
            if (delay_bit_map != 0)
                xSensorSendSetDelayQueue(sensortype, gDelay[sensortype], enable, delay_bit_map);
            if (enable_bit_map != 0) {
                ret = xSensorSendEnableActivateQueue(sensortype, enable, enable_bit_map);
                if (ret < 0) {
                    SM_ERR("gsensorSendEnableActivateQueue fail!!\n\r");
                    return -1;
                }
            }
            break;
    }
    SM_LOG("xSensorFrameworkSetActivate done: sensortype:(%d), enabledisable(%d), counter:%d, bitmap: 0x%x\n\r",
           sensortype, enable, genablecount[sensortype], enable_bit_map);
    return SM_SUCCESS;
}
static int xSelectApCm4RealEnableDisable(UINT32 task_handle, UINT8 sensortype, UINT32 enable)
{
    int ret = 0;
    if (enable == SENSOR_ENABLE) {
        if (gapcm4count[sensortype] == 0) {
            ret = xSensorFrameworkSetActivate(task_handle, sensortype, enable);
        }
        gapcm4count[sensortype]++;
    } else {
        if (gapcm4count[sensortype] == 1) {
            ret = xSensorFrameworkSetActivate(task_handle, sensortype, enable);
            gapcm4count[sensortype]--;
        } else if (gapcm4count[sensortype] == 0) {
            SM_LOG("this branch is used to avoid disable sensor before enable when bootup!!\n\r");
        } else {
            gapcm4count[sensortype]--;
            configASSERT((gapcm4count[sensortype] < 0 ? 0 : 1));//< 0 stands for disable enable not match, so assert
            SM_LOG("xSelectApCm4RealEnableDisable (handle:%d), sensor:(%d), enable(%d), gapcm4count(0x%x)\n\r", task_handle,
                   sensortype, enable, gapcm4count[sensortype]);
        }
    }
    return ret;
}
static int xSensorFrameworkDirectPush(int index, UINT8 sensortype, int flag, int delay, int timeout)
{
    //struct SensorManagerQueueEventStruct event;
    //BaseType_t MSG_RET;
    int ret = 0;
    //int delay_temp;
    UINT64 direct_push_bit_map = 0;

    gTaskInformation[index].delay[sensortype] = delay;
    gTaskInformation[index].timeout[sensortype] = timeout;


    switch (sensortype) {
        case SENSOR_TYPE_ACCELEROMETER:
        case SENSOR_TYPE_GYROSCOPE:
        case SENSOR_TYPE_PRESSURE:
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SENSOR_TYPE_LIGHT:
        case SENSOR_TYPE_PROXIMITY:
            xSetDelayState(index, sensortype, sensortype, &direct_push_bit_map, delay);
            if (direct_push_bit_map != 0) {
                ret = xSensorSendDirectPushDelayQueue(sensortype, flag, delay, timeout,
                                                      1, direct_push_bit_map);
                if (ret < 0) {
                    SM_ERR("gsensorSendEnableActivateQueue fail!!\n\r");
                    return -1;
                }
            }
            break;
        default:
            xSetDelayState(index, sensortype, sensortype, &direct_push_bit_map, delay);
            xCheckInputSensorDelayState(index, sensortype, &direct_push_bit_map, delay);
            if (direct_push_bit_map != 0) {
                ret = xSensorSendDirectPushDelayQueue(sensortype, flag, delay, timeout,
                                                      1, direct_push_bit_map);
                if (ret < 0) {
                    SM_ERR("gsensorSendEnableActivateQueue fail!!\n\r");
                    return -1;
                }
            }
            break;
    }

    return SM_SUCCESS;
}
static int xSensorFrameworkSetBatch(int index, UINT8 sensortype, int flag, int delay, int timeout)
{
    //struct SensorManagerQueueEventStruct event;
    //BaseType_t MSG_RET;
    int ret = 0;
    //int delay_temp;
    UINT64 batch_delay_bit_map = 0;

    gTaskInformation[index].delay[sensortype] = delay;
    gTaskInformation[index].timeout[sensortype] = timeout;


    switch (sensortype) {
        case SENSOR_TYPE_ACCELEROMETER:
        case SENSOR_TYPE_GYROSCOPE:
        case SENSOR_TYPE_PRESSURE:
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SENSOR_TYPE_LIGHT:
        case SENSOR_TYPE_PROXIMITY:
            xSetDelayState(index, sensortype, sensortype, &batch_delay_bit_map, delay);
            if (batch_delay_bit_map != 0) {
                ret = xSensorSendSetBatchDelayQueue(sensortype, flag, delay, timeout,
                                                    1, batch_delay_bit_map);
                if (ret < 0) {
                    SM_ERR("gsensorSendEnableActivateQueue fail!!\n\r");
                    return -1;
                }
            }
            break;
        default:
            xSetDelayState(index, sensortype, sensortype, &batch_delay_bit_map, delay);
            xCheckInputSensorDelayState(index, sensortype, &batch_delay_bit_map, delay);
            if (batch_delay_bit_map != 0) {
                ret = xSensorSendSetBatchDelayQueue(sensortype, flag, delay, timeout,
                                                    1, batch_delay_bit_map);
                if (ret < 0) {
                    SM_ERR("gsensorSendEnableActivateQueue fail!!\n\r");
                    return -1;
                }
            }
            break;
    }

    return SM_SUCCESS;
}
//judge delay of all gTaskInformation[i].delay[scp_sensor_type]
static int xSensorFrameworkSetDelay(int index, UINT8 sensortype, int delay)
{
    int ret = 0;
    int delay_temp;
    UINT64 delay_bit_map = 0;

    gTaskInformation[index].delay[sensortype] = delay;

    delay_temp = xSensorFrameworkChooseFastestDelay(sensortype);
    if (delay_temp < 0) {
        return delay_temp;
    }
    SM_LOG("INFO: sensor(%d): delay_temp value(%d)\n\r", sensortype, delay_temp);

    if (delay_temp != gDelay[sensortype]) {
        gDelay[sensortype] = delay_temp;
    } else {
        SM_LOG("INFO: sensor(%d): delay value(%d) is not the fastest(%d), no need to update\n\r", sensortype, delay,
               gDelay[sensortype]);
    }
    switch (sensortype) {
        case SENSOR_TYPE_PRESSURE:
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SENSOR_TYPE_LIGHT:
        case SENSOR_TYPE_PROXIMITY:
            xSetDelayState(index, sensortype, sensortype, &delay_bit_map, gDelay[sensortype]);
            if (delay_bit_map != 0) {
                ret = xSensorSendSetDelayQueue(sensortype, gDelay[sensortype], 1, delay_bit_map);
                if (ret < 0) {
                    SM_ERR("gsensorSendEnableActivateQueue fail!!\n\r");
                    return -1;
                }
            }
            break;
        case SENSOR_TYPE_ACCELEROMETER:
        case SENSOR_TYPE_MAGNETIC_FIELD:
        case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
        case SENSOR_TYPE_GYROSCOPE:
        case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
        case SENSOR_TYPE_LINEAR_ACCELERATION:
        case SENSOR_TYPE_GRAVITY:
        case SENSOR_TYPE_GAME_ROTATION_VECTOR:
        case SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
        case SENSOR_TYPE_ROTATION_VECTOR:
        case SENSOR_TYPE_ORIENTATION:
            if (index == SENSOR_FRAMEWORK_AP_INDEX) {
                if (gTaskInformation[index].delay[sensortype] <= 20) {
                    ret = xSensorFrameworkDirectPush(SENSOR_FRAMEWORK_AP_INDEX, sensortype,
                                                     SENSORS_BATCH_WAKE_UPON_FIFO_FULL, gDelay[sensortype], 20);
                    if (ret < 0) {
                        SM_ERR("ERR: xSensorFrameworkHandleIpi SetDelay(id:%d)(flag:%d)(delay:%d)(timeout:%d) failed!\n\r",
                               sensortype, SENSORS_BATCH_WAKE_UPON_FIFO_FULL, delay, 20);
                    }
                } else {
                    ret = xSensorFrameworkDirectPush(SENSOR_FRAMEWORK_AP_INDEX, sensortype,
                                                     SENSORS_BATCH_WAKE_UPON_FIFO_FULL, gDelay[sensortype],
                                                     gTaskInformation[index].delay[sensortype]);
                    if (ret < 0) {
                        SM_ERR("ERR: xSensorFrameworkHandleIpi SetDelay(id:%d)(flag:%d)(delay:%d)(timeout:%d) failed!\n\r",
                               sensortype, SENSORS_BATCH_WAKE_UPON_FIFO_FULL, delay,
                               gTaskInformation[index].delay[sensortype]);
                    }
                }
            } else {
                xSetDelayState(index, sensortype, sensortype, &delay_bit_map, gDelay[sensortype]);
                if (delay_bit_map != 0) {
                    ret = xSensorSendSetDelayQueue(sensortype, gDelay[sensortype], 1, delay_bit_map);
                    if (ret < 0) {
                        SM_ERR("gsensorSendEnableActivateQueue fail!!\n\r");
                        return -1;
                    }
                }
            }
            break;
        default:
            xSetDelayState(index, sensortype, sensortype, &delay_bit_map, gDelay[sensortype]);
            xCheckInputSensorDelayState(index, sensortype, &delay_bit_map, delay);
            if (delay_bit_map != 0) {
                ret = xSensorSendSetDelayQueue(sensortype, gDelay[sensortype], 1, delay_bit_map);
                if (ret < 0) {
                    SM_ERR("gsensorSendEnableActivateQueue fail!!\n\r");
                    return -1;
                }
            }
            break;
    }
    SM_ERR("xSensorFrameworkSetDelay sensor:(%d), delay(%d), gDelay(0x%x)\n\r", sensortype, delay, gDelay[sensortype]);

    return SM_SUCCESS;
}

static int xSensorFrameworkSetCust(int index, UINT8 sensortype, UINT32 *cust_data, UINT32 size)
{
    struct SensorManagerQueueEventStruct event;
    BaseType_t MSG_RET;
    int ret = 0;

    event.action = SF_SET_CUST;
    event.info.task_handler = 0;
    event.info.sensortype = sensortype;
    memcpy(event.info.data, cust_data, size);

    MSG_RET = xQueueSend(gSensorManagerQueuehandle, &event, 0);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: xSensorFrameworkSetCust xQueueSend failed!!\n\r");
        return -1;
    }

    return ret;
}

static int xSensorFrameworkFlushNewest(int index, UINT8 sensortype)
{
    struct SensorManagerQueueEventStruct event;
    BaseType_t MSG_RET;

    event.action = SF_GET_DATA;
    event.info.sensortype = sensortype;
    event.info.task_handler = 0;

    MSG_RET = xQueueSend(gSensorManagerQueuehandle, &event, 0);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: xSensorFrameworkFlushNewest xQueueSend failed!!\n\r");
        return -1;
    }
    return SM_SUCCESS;
}

static int xSensorFrameworkUpdateGesture(int index, UINT8 sensortype, int gesturetype, int update)
{
    struct SensorManagerQueueEventStruct event;
    BaseType_t MSG_RET;

    event.action = SF_UPDATE_GESTURE;
    event.info.sensortype = sensortype;
    event.info.task_handler = 0;
    event.info.data[0] = gesturetype;
    event.info.data[1] = update;//0 stands for cancel, 1 stands for update

    MSG_RET = xQueueSend(gSensorManagerQueuehandle, &event, 0);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: xSensorFrameworkUpdateGesture xQueueSend failed!!\n\r");
        return -1;
    }
    return SM_SUCCESS;

}

static int xSensorFrameworkHandleIpi(struct SensorFrameworkQueueEventStruct *event)
{
    UINT8 scp_sensor_type = 0;
    UINT32 enable;
    UINT32 delay;
    UINT32 timeout;
    UINT32 batch_falg;
    INT8 ret = 0;

    SCP_SENSOR_HUB_REQ *req;
    SCP_SENSOR_HUB_ACTIVATE_REQ *act_req;
    SCP_SENSOR_HUB_SET_DELAY_REQ *delay_req;
    SCP_SENSOR_HUB_BATCH_REQ *batch_req;
    SCP_SENSOR_HUB_GET_DATA_REQ *get_data_req;
    SCP_SENSOR_HUB_SET_CUST_REQ *set_cust_req;
    SCP_SENSOR_HUB_SET_CONFIG_REQ *set_cfg_req;

    req = &(event->request.ipi_req);
    switch (event->request.ipi_req.action) {
        case SENSOR_HUB_ACTIVATE:
            //judgement
            act_req = ((SCP_SENSOR_HUB_ACTIVATE_REQ *)req);
            scp_sensor_type =  xSensorTypeMappingToSCP(act_req->sensorType);
            enable = act_req->enable;
            switch (scp_sensor_type) {
                case SENSOR_TYPE_ACCELEROMETER:
                case SENSOR_TYPE_MAGNETIC_FIELD:
                case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
                case SENSOR_TYPE_GYROSCOPE:
                case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
                case SENSOR_TYPE_LINEAR_ACCELERATION:
                case SENSOR_TYPE_GRAVITY:
                case SENSOR_TYPE_GAME_ROTATION_VECTOR:
                case SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR:
                case SENSOR_TYPE_ROTATION_VECTOR:
                case SENSOR_TYPE_ORIENTATION:
                    if (0 == test_bit(&gBatchInfo.ap_batch_activate, scp_sensor_type)) {
                        ret = xSensorSendDirectPushTimerQueue(scp_sensor_type, enable);
                        if (ret < 0) {
                            SM_ERR("ERR: xSensorSendDirectPushTimerQueue failed!\n\r");
                        }
                    }
                    break;
                default:
                    break;
            }
            SM_ERR("INFO:xSensorFrameworkHandleIpi SENSOR_HUB_ACTIVATE(id:%d)(enable:%d)!!\n\r", scp_sensor_type, enable);
            ret = xSelectApCm4RealEnableDisable(SENSOR_FRAMEWORK_AP_INDEX, scp_sensor_type, enable);
            if (ret < 0) {
                SM_ERR("ERR: xSensorFrameworkHandleIpi Activate(id:%d)(enable:%d) failed!\n\r", scp_sensor_type, enable);
            }
            break;
        case SENSOR_HUB_SET_DELAY:
            //judgement
            delay_req = ((SCP_SENSOR_HUB_SET_DELAY_REQ *)req);
            scp_sensor_type = xSensorTypeMappingToSCP(delay_req->sensorType);
            delay = delay_req->delay;
            if (delay <= 5)
                delay = 5;
            SM_ERR("INFO:xSensorFrameworkHandleIpi SENSOR_HUB_SET_DELAY(id:%d)(delay:%d)!!\n\r", scp_sensor_type, delay);
            ret = xSensorFrameworkSetDelay(SENSOR_FRAMEWORK_AP_INDEX, scp_sensor_type, delay);
            if (ret < 0) {
                SM_ERR("ERR: xSensorFrameworkHandleIpi SetDelay(id:%d)(delay:%d) failed!\n\r", scp_sensor_type, delay);
            }
            break;
        case SENSOR_HUB_BATCH_TIMEOUT:
            SM_ERR("ap batch time out\n\r");
            scp_sensor_type = xSensorTypeMappingToSCP(req->sensorType);
            gBatchInfo.ap_batch_timeout = 1;
            ret = xSensorSendApBatchTimeoutQueue(scp_sensor_type);
            break;
        case SENSOR_HUB_GET_DATA:
            //return directly
            get_data_req = ((SCP_SENSOR_HUB_GET_DATA_REQ *)req);
            scp_sensor_type = xSensorTypeMappingToSCP(get_data_req->sensorType);

            SM_LOG("SENSOR_HUB_GET_DATA, gBatchInfo.ap_batch_activate = %d\n\r", gBatchInfo.ap_batch_activate);


            ret = xFrameworkIpiNotifyAP(scp_sensor_type, SENSOR_HUB_GET_DATA, 0, 0);
            if (ret < 0) {
                SM_ERR("ERR: xSensorFrameworkHandleIpi xFrameworkIpiNotifyAP SENSOR_HUB_GET_DATA failed!\n\r");
            }
            break;
        case SENSOR_HUB_BATCH:
            //judgement delay, req->period_ms
            //is batch action needed?
            batch_req = ((SCP_SENSOR_HUB_BATCH_REQ *)req);
            scp_sensor_type = xSensorTypeMappingToSCP(batch_req->sensorType);
            batch_falg = batch_req->flag;
            delay = batch_req->period_ms;
            if (delay <= 10)
                delay = 10;
            timeout = batch_req->timeout_ms;
            if (timeout != 0) {
                if (1 == test_bit(&gBatchInfo.scp_direct_push_activate, scp_sensor_type)) {
                    ret = xSensorSendDirectPushTimerQueue(scp_sensor_type, 0);
                    if (ret < 0) {
                        SM_ERR("ERR: xSensorSendDirectPushTimerQueue failed!\n\r");
                    }
                }
            }
            SM_ERR("INFO:xSensorFrameworkHandleIpi SENSOR_HUB_BATCH(id:%d)(flag:%d)(delay:%d)(timeout:%d)!!\n\r", scp_sensor_type,
                   batch_falg, delay, timeout);
            if (timeout != 0) {
                set_bit(&gBatchInfo.ap_batch_activate, scp_sensor_type);
            } else {
                clear_bit(&gBatchInfo.ap_batch_activate, scp_sensor_type);
                /*gBatchInfo.ap_batch_timeout = 1;
                ret = xSensorSendApBatchTimeoutQueue(scp_sensor_type);*/
            }
            gAlgorithm[scp_sensor_type].wakeup_batch_timeout = timeout;
            gAlgorithm[scp_sensor_type].wakeup_batch_counter = timeout;
            if (0 == timeout) {
                ret = SM_SUCCESS;
                break;
            }
            if (batch_falg & SENSORS_BATCH_WAKE_UPON_FIFO_FULL)
                set_bit(&gBatchInfo.wake_up_on_fifo_full, scp_sensor_type);
            else
                clear_bit(&gBatchInfo.wake_up_on_fifo_full, scp_sensor_type);


            ret = xSensorFrameworkSetBatch(SENSOR_FRAMEWORK_AP_INDEX, scp_sensor_type, batch_falg, delay, timeout);
            if (ret < 0) {
                SM_ERR("ERR: xSensorFrameworkHandleIpi SetDelay(id:%d)(flag:%d)(delay:%d)(timeout:%d) failed!\n\r", scp_sensor_type,
                       batch_falg, delay, timeout);
            }
            break;
        case SENSOR_HUB_SET_CONFIG:
            set_cfg_req = (SCP_SENSOR_HUB_SET_CONFIG_REQ *)req;
            scp_sensor_type = xSensorTypeMappingToSCP(set_cfg_req->sensorType);
            dvfs_enable_DRAM_resource(SENS_MEM_ID);
            get_emi_semaphore();
            gManagerDataInfo.bufferBase = (struct sensorFIFO *)ap_to_scp((uint32_t)(set_cfg_req->bufferBase));
            gManagerDataInfo.bufferSize = set_cfg_req->bufferSize;
            gManagerDataInfo.bufferBase->FIFOSize = set_cfg_req->bufferSize;
            /* gManagerDataInfo.bufferBase->rp = &(gManagerDataInfo.bufferBase->data[0]);
            gManagerDataInfo.bufferBase->wp = &(gManagerDataInfo.bufferBase->data[0]); */
            SM_ERR("get dram phy addr=%x,size=%d\n\r", gManagerDataInfo.bufferBase, gManagerDataInfo.bufferSize);
            SM_ERR("get dram phy &data[0]=%x \n\r", gManagerDataInfo.bufferBase->data[0]);
            SM_ERR("get dram phy rp=%x,wp=%x\n\r", gManagerDataInfo.bufferBase->rp, gManagerDataInfo.bufferBase->rp);
            release_emi_semaphore();
            dvfs_disable_DRAM_resource(SENS_MEM_ID);
            ret = 0;
            break;
        case SENSOR_HUB_SET_TIMESTAMP:
            set_cfg_req = (SCP_SENSOR_HUB_SET_CONFIG_REQ *)req;
            scp_sensor_type = xSensorTypeMappingToSCP(set_cfg_req->sensorType);
            //gManagerDataInfo.timestamp_offset_to_ap = timestamp_get_ns() * (100000 - 2328) / 100000;
            gManagerDataInfo.timestamp_offset_to_ap = read_xgpt_stamp_ns();
            SM_ERR("scp time:%lld, ap time:%lld\n\r", gManagerDataInfo.timestamp_offset_to_ap, set_cfg_req->ap_timestamp);
            gManagerDataInfo.timestamp_offset_to_ap =
                set_cfg_req->ap_timestamp - gManagerDataInfo.timestamp_offset_to_ap - 300000;

            SM_ERR("get timestamp offset to ap:%lld\n\r", gManagerDataInfo.timestamp_offset_to_ap);
            //gManagerDataInfo.timestamp_offset_to_ap = 0;
            ret = 0;
            break;
        case SENSOR_HUB_SET_CUST:
            set_cust_req = ((SCP_SENSOR_HUB_SET_CUST_REQ *)req);
            scp_sensor_type = xSensorTypeMappingToSCP(set_cust_req->sensorType);

            ret = xSensorFrameworkSetCust(SENSOR_FRAMEWORK_AP_INDEX, scp_sensor_type, set_cust_req->custData,
                                          sizeof(set_cust_req->custData));
            if (ret < 0) {
                SM_ERR("ERR: xSensorFrameworkHandleIpi SetCust(id:%d)failed!\n\r", scp_sensor_type);
            }
            break;
        default:
            ASSERT(0);
            break;
    }
    ret = xFrameworkIpiNotifyAP(scp_sensor_type, event->request.ipi_req.action, 0, ret);
    if (ret < 0) {
        SM_ERR("ERR: xSensorFrameworkHandleIpi xFrameworkIpiNotifyAP SENSOR_HUB_GET_DATA failed!\n\r");
    }
    //SM_LOG( "INFO:xSensorFrameworkHandleIpi (id:%d) action(%d)!!\n\r", scp_sensor_type, event->request.ipi_req.action);

    return SM_SUCCESS;
}
static int xSumCm4TaskUserEnableCount(UINT8  sensortype)
{
    int task_handle = 0;
    int sum = 0;
    for (task_handle = 1; task_handle < SENSOR_TASK_MAX_COUNT; task_handle++) {
        sum += gTaskInformation[task_handle].enable_count[sensortype];
    }
    return sum;
}
static int xSelectCm4RealEnableDisable(UINT32 task_handle, UINT8  sensortype, UINT32 enable)
{
    int err = 0;
    if (enable == 1) {
        set_bit(&gTaskInformation[task_handle].enable, sensortype);
        if (gTaskInformation[task_handle].enable_count[sensortype] == 0) {
            if (0 == xSumCm4TaskUserEnableCount(sensortype)) {
                err = xSelectApCm4RealEnableDisable(task_handle, sensortype, enable);
                if (err < 0) {
                    SM_ERR("ERR:xSensorFrameworkHandleCm4Task xSensorFrameworkSetActivate failed!!\n\r");
                }
            }
        }
        gTaskInformation[task_handle].enable_count[sensortype]++;

    } else if (enable == 0) {
        if (gTaskInformation[task_handle].enable_count[sensortype] == 1) {
            clear_bit(&gTaskInformation[task_handle].enable, sensortype);
            if (1 == xSumCm4TaskUserEnableCount(sensortype)) {
                err = xSelectApCm4RealEnableDisable(task_handle, sensortype, enable);
                if (err < 0) {
                    SM_ERR("ERR:xSensorFrameworkHandleCm4Task xSensorFrameworkSetActivate failed!!\n\r");
                }
            }
        }
        gTaskInformation[task_handle].enable_count[sensortype]--;
        configASSERT((gTaskInformation[task_handle].enable_count[sensortype] < 0 ? 0 :
                      1)); //< 0 stands for disable enable not match, so assert
    }
    return err;
}
static int xSensorFrameworkHandleCm4Task(struct SensorFrameworkQueueEventStruct *event)
{
    UINT32 task_handle;
    UINT8  sensortype;
    UINT32 enable;
    UINT32 delay;
    UINT32 gesture;
    INT32 err = 0;

    switch (event->request.cm4_task.command) {
        case SENSOR_ACTIVATE_CMD:
            task_handle = event->request.cm4_task.handle;
            sensortype = event->request.cm4_task.sensor_type;
            enable = event->request.cm4_task.value;
            err = xSelectCm4RealEnableDisable(task_handle, sensortype, enable);
            break;
        case SENSOR_SETDELAY_CMD:
            task_handle = event->request.cm4_task.handle;
            sensortype = event->request.cm4_task.sensor_type;
            delay = event->request.cm4_task.value;
            err = xSensorFrameworkSetDelay(task_handle, sensortype, delay);
            if (err < 0) {
                SM_ERR("ERR:xSensorFrameworkHandleCm4Task xSensorFrameworkSetDelay failed!!\n\r");
            }
            break;
        case SENSOR_POLL_CMD:
            task_handle = event->request.cm4_task.handle;
            sensortype = event->request.cm4_task.sensor_type;
            if (genablecount[sensortype] == 0) {
                SM_ERR("ERR:xSensorFrameworkHandleCm4Task this sensor is disable!!\n\r");
                return SM_ERROR;
            }
            err = xSensorFrameworkFlushNewest(task_handle, sensortype);
            if (err < 0) {
                SM_ERR("ERR:xSensorFrameworkHandleCm4Task xSensorFrameworkFlushNewest failed!!\n\r");
            }
            break;
        case SENSOR_UPDATE_GESTURE:
            task_handle = event->request.cm4_task.handle;
            sensortype = event->request.cm4_task.sensor_type;
            gesture = event->request.cm4_task.value;
            xSensorFrameworkUpdateGesture(task_handle, sensortype, gesture, 1);
        case SENSOR_CANCEL_GESTURE:
            task_handle = event->request.cm4_task.handle;
            sensortype = event->request.cm4_task.sensor_type;
            gesture = event->request.cm4_task.value;
            xSensorFrameworkUpdateGesture(task_handle, sensortype, gesture, 0);
        default:
            break;
    }
    //SM_LOG( "INFO: SensorFramework xSensorFrameworkHandleCm4Task ends...!!\n\r");
    return err;
}


static int xSensorFrameworkHandleData(struct SensorFrameworkQueueEventStruct *event)
{
    //this function should use mutex to protect data memory because it is shared memory with sensormanager
    int i, j, k;
    struct data_unit_t SensorData;
    UINT32 data_count = 0;

    //SM_LOG("INFO xSensorFrameworkHandleData0 notify_data(0x%8x),(%d)\n\r", event->request.notify_data, sizeof(event->request.notify_data));

    for (i = 1; i < SENSOR_TYPE_MAX_COUNT + 1; i++) {
        if (event->request.notify_data & (1ULL << i)) {
            //TODO:add mutex to protect
            SM_LOG("INFO xSensorFrameworkHandleData1 sensor(%d):(%x)\n\r", i, event->request.notify_data);
            data_count = gAlgorithm[i].newest->data_exist_count;

            for (j = 1; j < SENSOR_TASK_MAX_COUNT; j++) {
                if (gTaskInformation[j].enable & (1ULL << i)) {
                    for (k = 0; k < data_count; k++) {
                        xSemaphoreTake(xSMSemaphore[i], portMAX_DELAY);
                        //TODO: need to check whether memory is empty
                        memcpy(&SensorData, &(gAlgorithm[i].newest->data[k]), sizeof(struct data_unit_t));
                        //TODO:mutex end
                        xSemaphoreGive(xSMSemaphore[i]);
                        //TODO, check callback function run time
                        SM_LOG("INFO xSensorFrameworkHandleData2 task(%d) sensor(%d):(%x)\n\r", j, i, event->request.notify_data);
                        gTaskInformation[j].onSensorChanged(i, SensorData);
                    }
                }
            }
            //TODO:add mutex to protect
            xSemaphoreTake(xSMSemaphore[i], portMAX_DELAY);
            gManagerDataInfo.data_ready_bit &= ~(1ULL << i);
            xSemaphoreGive(xSMSemaphore[i]);
            //TODO:mutex end
        }
    }

    SM_LOG("INFO data_count:(%d)\n\r", data_count);

    return SM_SUCCESS;
}

static int xSensorFrameworkHandleAccuracy(struct SensorFrameworkQueueEventStruct *event)
{
    //this function should use mutex to protect data memory because it is shared memory with sensormanager
    int i, j;
    UINT32 accuracy;

    for (i = 1; i < SENSOR_TYPE_MAX_COUNT + 1; i++) {
        if (event->request.notify_data & (1ULL << i)) {
            //TODO:add mutex to protect
            xSemaphoreTake(xSMSemaphore[i], portMAX_DELAY);
            accuracy = gAlgorithm[i].newest->data->accelerometer_t.status;
            //TODO:mutex end
            xSemaphoreGive(xSMSemaphore[i]);
            for (j = 1; j < SENSOR_TASK_MAX_COUNT; j++) {
                if (gTaskInformation[j].enable & (1ULL << i)) {
                    //TODO, check callback function run time
                    gTaskInformation[j].onAccuracyChanged(i, accuracy);
                }
            }
            //TODO:add mutex to protect
            xSemaphoreTake(xSMSemaphore[i], portMAX_DELAY);
            gManagerDataInfo.accuracy_change_bit &= ~(1ULL << i);
            xSemaphoreGive(xSMSemaphore[i]);
            //TODO:mutex end
        }
    }

    return SM_SUCCESS;
}

void xSensorFrameworkEntry(void *arg)
{
    BaseType_t ret;
    struct SensorFrameworkQueueEventStruct event;
    UINT8 sensortype = 0;
    UINT8 action = 0;
    UINT8 event_type = 0;
    SM_LOG("INFO: SensorFramework start to run...!!\n\r");

    while (1) {
        ret = xQueueReceive(gSensorFrameworkQueuehandle, &event, portMAX_DELAY);
        if (ret != pdPASS) {
            SM_ERR("Error: SF task xQueueReceive failed!!\n\r");
        }

        SM_LOG("INFO: SensorFramework receive events...!!\n\r");

        switch (event.action) {
            case IPI_REQUEST:
                xSensorFrameworkHandleIpi(&event);
                break;
            case CM4_APP_REQUEST:
                xSensorFrameworkHandleCm4Task(&event);
                break;
            case DATA_CHANGED:
                xSensorFrameworkHandleData(&event);
                break;
            case ACCURACY_CHANGED:
                xSensorFrameworkHandleAccuracy(&event);
                break;
            case BATCH_DRAMFULL_NOTIFY:
            case INTR_NOTIFY:
            case BATCH_TIMEOUT_NOTIFY:
            case BATCH_WAKEUP_NOTIFY:
            case DIRECT_PUSH_NOTIFY:
                sensortype = event.request.ipi_req.sensorType;
                action = event.request.ipi_req.action;
                event_type = event.request.ipi_req.event;
                xFrameworkIpiNotifyAP(sensortype, action, event_type, 0);
                break;
            case POWER_NOTIFY:
                sensortype = event.request.ipi_req.sensorType;
                action = event.request.ipi_req.action;
                xFrameworkIpiNotifyAP(sensortype, action, 0, 0);
                break;
            case RESERVE_ACTION:
                break;
            default:
                break;
        }
        //SM_LOG( "INFO: SensorFramework loop ends...!!\n\r");
    }
}

/*
int SCP_Sensor_Manager_open(uint32_t sensor_type)
return value: cm4 handle
*/
int SCP_Sensor_Manager_open(struct CM4TaskInformationStruct *cm4)
{
    int i, j;

    for (i = 0; i < SENSOR_TASK_MAX_COUNT; i++) {
        if (gTaskInformation[i].onSensorChanged == NULL) { //not use this index yet
            gTaskInformation[i].handle = i;
            gTaskInformation[i].enable = 0LL;
            for (j = 1; j < SENSOR_TYPE_MAX_COUNT + 1; j++) {
                gTaskInformation[i].delay[j] = 0xFFFF;//MAX value unit ms
                gTaskInformation[i].timeout[j] = 0x00;//batch timeout default to 0
                gTaskInformation[i].enable_count[j] = 0x00;
            }
            gTaskInformation[i].onSensorChanged   = cm4->onSensorChanged;
            gTaskInformation[i].onAccuracyChanged = cm4->onAccuracyChanged;
            break;
        }
    }

    if (i == SENSOR_TASK_MAX_COUNT) {
        SM_ERR("Error: SF task full no more space to regist task!!\n\r");
        i = SM_ERROR; //return error
        return SM_ERROR;
    }

    SM_LOG("INFO: SCP_Sensor_Manager_open return(%d)!!\n\r", i);
    return i; //gTaskInformation array index, handle value
}

/*CM4 task send request to framework, put the event in queue*/
int SCP_Sensor_Manager_control(struct CM4TaskRequestStruct *req)
{
    BaseType_t MSG_RET;
    int err = 0;

    struct SensorFrameworkQueueEventStruct queueEvent;

    if (req->handle < 0 || req->handle > SENSOR_TASK_MAX_COUNT
            || gTaskInformation[req->handle].onSensorChanged == NULL) //not use this index yet
        return SM_ERROR;

    /*run judement with sensor framework queue and context*/
    /*run other commands in CM4 tasks context*/
    switch (req->command) {
        case SENSOR_ACTIVATE_CMD:
        case SENSOR_SETDELAY_CMD:
        case SENSOR_POLL_CMD:
        case SENSOR_UPDATE_GESTURE:
        case SENSOR_CANCEL_GESTURE:

            queueEvent.action = CM4_APP_REQUEST;
            queueEvent.size = sizeof(struct CM4TaskRequestStruct);

            memcpy(&queueEvent.request.cm4_task, (char*)req, sizeof(struct CM4TaskRequestStruct));

            MSG_RET = xQueueSend(gSensorFrameworkQueuehandle, &queueEvent, 0);
            if (MSG_RET != pdPASS) {
                err = -1;
                SM_ERR("Error:SCP_Sensor_Manager_control message cannot be send!!\n\r");
                return err;
            }
            break;
        default:
            //ASSERT(0);
            break;
    }

    return err;
}

int SCP_Sensor_Manager_close(int handle)
{
    //TODO: check cm4 and handle valid
    if (handle >= 0 && handle <= SENSOR_TASK_MAX_COUNT) {
        memset(&gTaskInformation[handle], 0, sizeof(struct TaskInformationStruct));
        return SM_SUCCESS;
    }
    return SM_ERROR;
}

static void vFrameworkIpiHandler(int id, void * data, unsigned int len)
{
    struct SensorFrameworkQueueEventStruct queueEvent;
    BaseType_t ret;
    portBASE_TYPE xDummy = pdTRUE;

    do {
        memcpy(&queueEvent.request.ipi_req, (char*)data, sizeof(SCP_SENSOR_HUB_REQ));
    } while (memcmp(data, &queueEvent.request.ipi_req, sizeof(SCP_SENSOR_HUB_REQ)));

    queueEvent.action = IPI_REQUEST;
    queueEvent.size = sizeof(SCP_SENSOR_HUB_REQ);
    ret = xQueueSendFromISR(gSensorFrameworkQueuehandle, &queueEvent, &xDummy);
    if (ret != pdPASS) {
        SM_ERR("Error: vFrameworkIPIHandler send failed!!\n\r");
    }
}

int xSensorFrameworkInit(void)
{
    BaseType_t ret;
    ipi_status ipi_ret;
    struct CM4TaskInformationStruct ap_task;
    int i, j;

    SM_LOG("INFO: xSensorFrameworkInit entry1!!\n\r");

    gSensorFrameworkQueuehandle = xQueueCreate(SENSOR_FRAMEWORK_QUEUE_LENGTH, SENSOR_FRAMEWORK_ITEM_SIZE);

    ipi_ret = scp_ipi_registration(IPI_SENSOR, vFrameworkIpiHandler, "sf_ipi_handler");
    scp_ipi_wakeup_ap_registration(IPI_SENSOR);
    if (ipi_ret != DONE) {
        SM_ERR("Error: SF scp_ipi_registration failed!!\n\r");
        return SM_ERROR;
    }

    SM_LOG("INFO: xSensorFrameworkInit entry2!!\n\r");

    //init parameters of delay, enable. etc.
    for (i = 0; i < SENSOR_TYPE_MAX_COUNT + 1; i++) {
        gDelay[i] = 0xFFFF;
    }

    for (i = 0; i < SENSOR_TASK_MAX_COUNT; i++) {
        gTaskInformation[i].enable = 0LL;
        gTaskInformation[i].onSensorChanged = NULL;
        for (j = 1; j < SENSOR_TYPE_MAX_COUNT + 1; j++) {
            gTaskInformation[i].delay[j] = 0xFFFF;//MAX value unit ms
            gTaskInformation[i].timeout[j] = 0x00;//batch timeout default to 0
            gTaskInformation[i].enable_count[j] = 0x00;
        }
    }

    ap_task.onSensorChanged = xAPonSensorChanged;
    ap_task.onAccuracyChanged = xAPonAccuracyChanged;
    SCP_Sensor_Manager_open(&ap_task); //register gTaskInformation[0] for ap

    SM_LOG("INFO: xSensorFrameworkInit entry3!!\n\r");

    ret = xTaskCreate(xSensorFrameworkEntry, "SF", 256, (void *) NULL, 3, NULL);
    if (ret != pdPASS) {
        SM_ERR("Error: SF task cannot be created!!\n\r");
        return ret;
    }

    return SM_SUCCESS;
}

