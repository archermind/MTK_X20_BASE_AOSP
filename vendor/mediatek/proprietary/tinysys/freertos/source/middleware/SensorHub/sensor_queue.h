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
#ifndef __SENSOR_QUEUE_H__
#define __SENSOR_QUEUE_H__

#include "sensors.h"
#include "queue.h"

#define SENSOR_TASK_MAX_COUNT 5 //AP, PDCA, LPF and 2 reserved
#define SENSOR_FRAMEWORK_QUEUE_LENGTH 20
#define SENSOR_FRAMEWORK_ITEM_SIZE    (sizeof(struct SensorFrameworkQueueEventStruct))

#define SENSOR_MANAGER_QUEUE_LENGTH 20
#define SENSOR_MANAGER_ITEM_SIZE (sizeof(struct SensorManagerQueueEventStruct))

#define SENSOR_FRAMEWORK_AP_INDEX 0

/*-----------------------------------sensor user message formate------------------------*/

typedef enum {
    SENSOR_ACTIVATE_CMD,
    SENSOR_SETDELAY_CMD,
    SENSOR_POLL_CMD,
    SENSOR_UPDATE_GESTURE,
    SENSOR_CANCEL_GESTURE,
    SENSOR_SET_CUST,
    SENSOR_RESVERED
} Cm4CommandEnum;

struct CM4TaskRequestStruct {
    INT32 handle; //AP, PDCA, LPF...etc
    UINT8 sensor_type;
    UINT8 reserve[3];
    Cm4CommandEnum command;
    INT32 value;//need check modify pointer to integer
};

struct CM4TaskInformationStruct { //for SCP task register sensor call back function
    void (*onSensorChanged)(int sensor, struct data_unit_t values);
    void (*onAccuracyChanged)(int sensor, int accuracy);
};

struct TaskInformationStruct { //for SF keep all sensor user information
    INT32 handle;   //gTaskInformation array index
    UINT64 enable; //bit wise, enable sensor id
    UINT16 enable_count[SENSOR_TYPE_MAX_COUNT];  //ms
    UINT16 delay[SENSOR_TYPE_MAX_COUNT];  //ms
    UINT16 timeout[SENSOR_TYPE_MAX_COUNT];  //ms
    void (*onSensorChanged)(int sensor, struct data_unit_t values);
    void (*onAccuracyChanged)(int sensor, int accuracy);
};

/*------------------------------sensor framework queue event formate----------------------*/
typedef enum { //deal with different queue event type from both CM4 task and sensormanager and AP, all commands are sent though queue message
    IPI_REQUEST,//AP event type
    CM4_APP_REQUEST,//SCP TASK event type
    DATA_CHANGED,//SensorManager TASK event
    ACCURACY_CHANGED,//SensorManager TASK event
    BATCH_DRAMFULL_NOTIFY,//SensorManager TASK event
    INTR_NOTIFY,//SensorManager TASK event for all wake up sensors, such as Psensor
    BATCH_TIMEOUT_NOTIFY,
    DIRECT_PUSH_NOTIFY,
    BATCH_WAKEUP_NOTIFY,
    POWER_NOTIFY,//SensorManager TASK event for lower power
    RESERVE_ACTION
} SensorFrameworkActionEnum;

struct accuracy_t {
    INT32 algo_type; //"ori" -> "rotation" --> null
    INT32 accuracy;
};

struct SensorFrameworkQueueEventStruct {
    SensorFrameworkActionEnum action;
    int size; //sizeof(*request)
    //IPI_REQ          : SCP_SENSOR_HUB_REQ,
    //CM4_APP_REQ      : CM4TaskRequestStruct
    //DATA_READY       : data_t
    //ACCURACY_CHANGE  : accuracy_t
    union { //ipi req, CM4 task request, handle dispatch data ...etc
        SCP_SENSOR_HUB_REQ   ipi_req;
        struct CM4TaskRequestStruct cm4_task;
        UINT64 notify_data;
    } request;
};

/*------------------------------sensor manager queue event formate----------------------*/
typedef enum {
    SF_ACTIVATE,
    SF_SET_DELAY,
    SF_SET_CUST,
    SF_BATCH,
    SM_TIMEOUT,
    SM_INTR,
    SF_GET_DATA,
    SF_UPDATE_GESTURE,
    SF_DIRECT_PUSH,
    SF_DIRECT_PUSH_DELAY,
    SM_BATCH_FLUSH,
    RESERVED
} SensorManagerActionEnum;

typedef struct {
    UINT32 task_handler;//Only handle Message from SensorFramework(0) and SensorManager(1)
    UINT8  sensortype;
    UINT8  reserve[3];
    UINT64 bit_map;
    /*when action is SF_BATCH,
    data[0] stands for flag,
    data[1] stands for sampling rate,
    data[2] stands for timout*/
    INT32  data[8];// notify: align to SCP_SENSOR_HUB_SET_CUST_REQ!!!!!!!!!!, when set xSensorFrameworkSetCust, we memcpy 8*4 bytes
} SensorManagerActionInfo;

struct SensorManagerQueueEventStruct {
    SensorManagerActionEnum action;
    SensorManagerActionInfo info;
};

/* ---------------------------For both physical sensor and virtual sensor ---------------------------*/


QueueHandle_t gSensorManagerQueuehandle;
QueueHandle_t gSensorFrameworkQueuehandle;

#endif

