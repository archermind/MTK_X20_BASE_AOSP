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

#include "sensor_queue.h"
#include "sensor_manager.h"
#include "sensorhub.h"
#define SM_TAG                  "[SensorManager]"
//#define SM_DEBUG
#ifdef SM_DEBUG
#define SM_ERR(fmt, args...)    PRINTF_D(SM_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define SM_LOG(fmt, args...)    PRINTF_D(SM_TAG fmt, ##args)
#else
#define SM_ERR(fmt, args...)    PRINTF_D(SM_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define SM_LOG(fmt, args...)
#endif

static UINT32 g_normal_t1;
static UINT32 g_normal_t2;

#define SENSOR_DATA_SIZE 48
#define FIFO_FULL         1
#define FIFO_NO_FULL      0
//static int last_fifo_status = FIFO_NO_FULL;
char BatchFifo[SCP_SENSOR_BATCH_FIFO_BATCH_SIZE + sizeof(struct sensorFIFO)];
struct sensorFIFO *gBatchFifo;

struct algorithm_descriptor_t gAlgorithm[SENSOR_TYPE_MAX_COUNT + 1];
struct batch_info_descriptor_t gBatchInfo;
struct data_info_descriptor_t gManagerDataInfo;
SemaphoreHandle_t xSMSemaphore[SENSOR_TYPE_MAX_COUNT + 1];
SemaphoreHandle_t xSMINITSemaphore;
extern INT16 gDelay[SENSOR_TYPE_MAX_COUNT]; //ms
#ifdef FWQLOG
static int xHandleSensorLog(int sensortype);
static int xHandleSensorBufferLog(void);
#endif

static void vDataFormateTransfer(struct data_t *data, UINT8 *sensor_event, int *size);
static void vPushDataToFifo(UINT8 *src, int size);
static void xFlushScpFifo(void);


//static TimerHandle_t SMTimer;
//static TimerHandle_t scp_direct_push_timer;

static bool Polling_Start = false;
static bool Scp_Direct_Push_Polling_Start = false;

static bool Delay_Changed = false;
UINT32 Real_Delay[SENSOR_TYPE_MAX_COUNT + 1];
static UINT32 Polling_Delay = 200; //ms;
static UINT32 Scp_Direct_Push_Polling_Delay = 65535; //ms;
static UINT64 now_time = 0;
static UINT32 init_done = 0;
/*-------------------------------sensor manager register API---------------------------------------*/
static int xConnectOutputSensors(struct SensorDescriptor_t *desp)
{
    int ret = 0;
    UINT8 sensortype = 0;
    struct output_list_t *output_temp = NULL;
    struct output_list_t *output_sm_temp = NULL;
    struct input_list_t *input_sm_temp = NULL;

    SM_LOG("xConnectOutputSensors(%d) begin0\n\r", desp->sensor_type);

    output_temp = (struct output_list_t*)pvPortMalloc(sizeof(struct output_list_t));
    if (output_temp == NULL) {
        SM_ERR("Alloc memory for this sensor(%d) inputlist fail\n\r", desp->sensor_type);
        return SM_ERROR;
    }

    output_temp->sampling_delay =
        -1;//TODO: this value is not right due to we don't know the exect report rate of this sensor
    output_temp->output_type = desp->sensor_type;
    output_temp->next_output = NULL;

    input_sm_temp = desp->input_list;

    SM_LOG("xConnectOutputSensors(%d) begin1\n\r", desp->sensor_type);

    while (input_sm_temp != NULL) {
        sensortype = input_sm_temp->input_type;

        if (gAlgorithm[sensortype].next_output == NULL) {
            SM_LOG("xConnectOutputSensors(%d) begin2\n\r", desp->sensor_type);
            gAlgorithm[sensortype].next_output = (struct output_list_t*)pvPortMalloc(sizeof(struct output_list_t));
            if (gAlgorithm[sensortype].next_output == NULL) {
                SM_ERR("Alloc memory for this sensor(%s) output list fail\n\r", desp->sensor_type);
                return SM_ERROR;
            }
            memcpy(gAlgorithm[sensortype].next_output, output_temp , sizeof(struct output_list_t));
            gAlgorithm[sensortype].next_output->sampling_delay = input_sm_temp->sampling_delay;
            SM_LOG("xConnectOutputSensors(%d) begin3\n\r", desp->sensor_type);
        } else {
            SM_LOG("xConnectOutputSensors(%d) begin4\n\r", desp->sensor_type);
            output_sm_temp = gAlgorithm[sensortype].next_output;
            while (output_sm_temp->next_output != NULL) { //find the lastest output node which is NULL
                output_sm_temp = output_sm_temp->next_output;
            }
            SM_LOG("xConnectOutputSensors(%d) begin5\n\r", desp->sensor_type);
            output_sm_temp->next_output = (struct output_list_t*)pvPortMalloc(sizeof(struct output_list_t));
            if (output_sm_temp->next_output == NULL) {
                SM_ERR("Alloc memory for this sensor(%s) output list fail\n\r", desp->sensor_type);
                return SM_ERROR;
            }
            SM_LOG("xConnectOutputSensors(%d) begin6\n\r", desp->sensor_type);
            memcpy(output_sm_temp->next_output, output_temp, sizeof(struct output_list_t));
            output_sm_temp->next_output->sampling_delay = input_sm_temp->sampling_delay;
            SM_LOG("xConnectOutputSensors(%d) begin7\n\r", desp->sensor_type);
        }
        input_sm_temp = input_sm_temp->next_input;
    };

    vPortFree((void*)output_temp);
    SM_LOG("xConnectOutputSensors(%d) begin8\n\r", desp->sensor_type);
    return ret;
}

/*notify framework can be used for all sensors to notify sensor framework to do stuffs*/
static int xNotifyFramework(UINT8 sensortype, UINT32 action)
{
    BaseType_t MSG_RET;
    int ret = 0;
    struct SensorFrameworkQueueEventStruct event;

    SM_LOG("xNotifyFramework, action=%d\n\r", action);
    event.action = action;
    //notify to SensorFramework this event is a batch timeout event or wakeup AP event to AP side or other events
    switch (event.action) {
        case BATCH_DRAMFULL_NOTIFY:
            event.request.ipi_req.action = SENSOR_HUB_NOTIFY;
            event.request.ipi_req.sensorType = sensortype;
            event.request.ipi_req.event = BATCH_DRAMFULL_NOTIFY;
            MSG_RET = xQueueSendToFront(gSensorFrameworkQueuehandle, &event, 0);
            if (MSG_RET != pdPASS) {
                SM_ERR("Error: xQueueSend failed!!\n\r");
                return -1;
            }
            return 0;
        case DIRECT_PUSH_NOTIFY:
            event.request.ipi_req.action = SENSOR_HUB_NOTIFY;
            event.request.ipi_req.sensorType = sensortype;
            event.request.ipi_req.event = DIRECT_PUSH_NOTIFY;
            MSG_RET = xQueueSendToFront(gSensorFrameworkQueuehandle, &event, 0);
            if (MSG_RET != pdPASS) {
                SM_ERR("Error: xQueueSend failed!!\n\r");
                return -1;
            }
            return 0;
        case BATCH_WAKEUP_NOTIFY:
            event.request.ipi_req.action = SENSOR_HUB_NOTIFY;
            event.request.ipi_req.sensorType = sensortype;
            event.request.ipi_req.event = BATCH_WAKEUP_NOTIFY;
            MSG_RET = xQueueSendToFront(gSensorFrameworkQueuehandle, &event, 0);
            if (MSG_RET != pdPASS) {
                SM_ERR("Error: xQueueSend failed!!\n\r");
                return -1;
            }
            return 0;
        case INTR_NOTIFY:
            //this is a batch dramfull handle finish event
            event.request.ipi_req.action = SENSOR_HUB_NOTIFY;
            event.request.ipi_req.sensorType = sensortype;
            event.request.ipi_req.event = INTR_NOTIFY;
            MSG_RET = xQueueSendToFront(gSensorFrameworkQueuehandle, &event, 0);
            if (MSG_RET != pdPASS) {
                SM_ERR("Error: xQueueSend failed!!\n\r");
                return -1;
            }
            return 0;
        case BATCH_TIMEOUT_NOTIFY:
            //this is a batch dramfull handle finish event
            event.request.ipi_req.action = SENSOR_HUB_NOTIFY;
            event.request.ipi_req.sensorType = sensortype;
            event.request.ipi_req.event = BATCH_TIMEOUT_NOTIFY;
            MSG_RET = xQueueSendToFront(gSensorFrameworkQueuehandle, &event, 0);
            if (MSG_RET != pdPASS) {
                SM_ERR("Error: xQueueSend failed!!\n\r");
                return -1;
            }
            return 0;
        case POWER_NOTIFY:
            event.request.ipi_req.action = SENSOR_HUB_POWER_NOTIFY;
            event.request.ipi_req.sensorType = sensortype;
            MSG_RET = xQueueSendToFront(gSensorFrameworkQueuehandle, &event, 0);
            if (MSG_RET != pdPASS) {
                SM_ERR("Error: xQueueSend failed!!\n\r");
                return -1;
            }
            return 0;
        case DATA_CHANGED:
            //SM_LOG("xNotifyFramework DATA_CHANGED1, notify_data=0x%8x, bit:(0x%8x)\n\r", event.request.notify_data, gManagerDataInfo.data_ready_bit);
            event.request.notify_data = gManagerDataInfo.data_ready_bit;
            //SM_LOG("xNotifyFramework DATA_CHANGED2, notify_data=0x%8x, bit:(0x%8x)\n\r", event.request.notify_data, gManagerDataInfo.data_ready_bit);
            break;
        case ACCURACY_CHANGED:
            event.request.notify_data = gManagerDataInfo.accuracy_change_bit;
            break;
        default:
            break;
    }

    MSG_RET = xQueueSend(gSensorFrameworkQueuehandle, &event, 0);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: xQueueSend failed!!\n\r");
        return -1;
    }

    return ret;
}

int sensor_subsys_algorithm_register_type(struct SensorDescriptor_t *desp)
{
    int ret = 0;
    int sensortype = -1;
    struct data_t data = {0};
    struct input_list_t *input_driver_head = NULL;
    struct input_list_t *input_sm_head = NULL;

    sensortype = desp->sensor_type;

    //SM_LOG("Register for sensor(%d) begin1\n\r", desp->sensor_type);

    /*for HW sensor AUTO detect check*/
    if (desp->hw.max_sampling_rate > 0) {
        data.data = (struct data_unit_t*)pvPortMalloc(sizeof(struct data_unit_t));
        if (data.data == NULL) {
            SM_ERR("Alloc memory for this sensor test fail\n\r");
            return SM_ERROR;
        }
        ret = desp->run_algorithm(
                  &data);//TODO: need refactor for auto detect? not all drivers are register after init which may leed to i2c transfer fail
        if (ret < 0) {
            SM_ERR("hardware sensor i2c transfer fail autodetect fail: %d\n\r", desp->sensor_type);
            return SM_ERROR;
        }
    }

    //SM_LOG("Register for sensor(%d) begin2\n\r", desp->sensor_type);

    memcpy((&gAlgorithm[sensortype].algo_desp), desp, sizeof(struct SensorDescriptor_t));

    //SM_LOG("Register for sensor(%d) begin3\n\r", desp->sensor_type);

    /*add inputlist to gAlgorithm[], alloc new memory for gAlgorithm its own*/
    if (desp->input_list != NULL) {

        input_sm_head = (struct input_list_t*)pvPortMalloc(sizeof(struct input_list_t));
        if (input_sm_head == NULL) {
            SM_ERR("Alloc memory for this sensor(%d) input list fail\n\r", desp->sensor_type);
            return SM_ERROR;
        }

        //SM_LOG("Register for sensor(%d) begin4\n\r", desp->sensor_type);
        gAlgorithm[sensortype].algo_desp.input_list = input_sm_head;

        input_driver_head = desp->input_list;

        if (input_driver_head->next_input == NULL) {
            memcpy(input_sm_head, input_driver_head, sizeof(struct input_list_t));
        } else if (input_driver_head->next_input != NULL) {
            do {
                SM_LOG("input_sm_head sensor(%d) 1\n\r", input_sm_head->input_type);
                SM_LOG("input_driver_head sensor(%d) 1\n\r", input_driver_head->input_type);
                memcpy(input_sm_head, input_driver_head, sizeof(struct input_list_t));

                SM_LOG("input_sm_head sensor(%d) 2\n\r", input_sm_head->input_type);
                SM_LOG("input_driver_head sensor(%d) 2\n\r", input_driver_head->input_type);

                input_sm_head->next_input = NULL;

                if (input_driver_head->next_input !=
                        NULL) { //add for check NULL Pointer Issue which lead to one more inputlist with index 0
                    input_sm_head->next_input = (struct input_list_t*)pvPortMalloc(sizeof(struct input_list_t));
                    if (input_sm_head->next_input == NULL) {
                        SM_ERR("Alloc memory for this sensor(%d) input list fail\n\r", desp->sensor_type);
                        return SM_ERROR;
                    }
                }

                SM_LOG("input_sm_head sensor(%d) 3\n\r", input_sm_head->input_type);
                SM_LOG("input_driver_head sensor(%d) 3\n\r", input_driver_head->input_type);
                input_driver_head = input_driver_head->next_input;
                input_sm_head = input_sm_head->next_input;
                SM_LOG("input_sm_head sensor(%p) 4\n\r", input_sm_head);
                SM_LOG("input_driver_head sensor(%p) 4\n\r", input_driver_head);

            } while (input_driver_head != NULL);
        }

        input_sm_head = gAlgorithm[sensortype].algo_desp.input_list;
        SM_LOG("input_sm_head sensor(%p) , gAlgorithm(%p)\n\r", input_sm_head, gAlgorithm[sensortype].algo_desp.input_list);
        while (input_sm_head != NULL) {
            SM_LOG("inputlist sensor(%d)'s (%d)\n\r", sensortype, input_sm_head->input_type);
            input_sm_head = input_sm_head->next_input;
        }

        SM_LOG("Register for sensor(%d) begin5\n\r", desp->sensor_type);

        /*TODO: need to alloc data memory for every sensor?*/
        ret = xConnectOutputSensors(desp);
        if (ret < 0) {
            SM_ERR("contect sensor(%d) to output list fail\n\r", desp->sensor_type);
            return SM_ERROR;
        }
    }

    SM_LOG("Register for sensor(%d) done\n\r", desp->sensor_type);

    return ret;
}

int sensor_subsys_algorithm_register_data_buffer(UINT8 sensortype, int exist_data_count)
{
    gAlgorithm[sensortype].exist_data_count = exist_data_count;
    gAlgorithm[sensortype].newest = (struct data_t*)pvPortMalloc(sizeof(struct data_t));
    if (gAlgorithm[sensortype].newest == NULL) {
        SM_ERR("Alloc memory for this sensor(%s) databuffer1 fail\n\r", gAlgorithm[sensortype].algo_desp.sensor_type);
        return SM_ERROR;
    }
    gAlgorithm[sensortype].newest->fifo_max_size = exist_data_count * 2;
    gAlgorithm[sensortype].newest->next_data = NULL;
    gAlgorithm[sensortype].newest->data = (struct data_unit_t*)pvPortMalloc(sizeof(struct data_unit_t) * exist_data_count);
    if (gAlgorithm[sensortype].newest->data == NULL) {
        SM_ERR("Alloc memory for this sensor(%d) databuffer2 fail\n\r", gAlgorithm[sensortype].algo_desp.sensor_type);
        return SM_ERROR;
    }
    gAlgorithm[sensortype].newest->data_exist_count = 0;
    memset(gAlgorithm[sensortype].newest->data, 0,
           sizeof(struct data_unit_t) * gAlgorithm[sensortype].exist_data_count);
    return SM_SUCCESS;
}

int sensor_subsys_algorithm_notify(UINT8 sensortype)
{
    int ret = 0;
    struct SensorManagerQueueEventStruct event;
    BaseType_t MSG_RET;

    event.action = SM_INTR;
    event.info.sensortype = sensortype;
    event.info.task_handler = 0;
    if ((sensortype == SENSOR_TYPE_ACCELEROMETER) || (sensortype == SENSOR_TYPE_PROXIMITY)
            || (sensortype == SENSOR_TYPE_GYROSCOPE) || (sensortype == SENSOR_TYPE_MAGNETIC_FIELD)) {
        if ((gManagerDataInfo.intr_used_bit & (1ULL << sensortype)) == 0)
            gManagerDataInfo.intr_used_bit |= (1ULL << sensortype);
    }
    MSG_RET = xQueueSendToFrontFromISR(gSensorManagerQueuehandle, &event, NULL);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: TriggerInterruptAlgrithm xQueueSendFromISR failed!!\n\r");
        return -1;
    }
    return ret;
}

/*-------------------------------sensor manager common code---------------------------------------*/
static int xUpdateGestureMappingTable(UINT8 sensortype, UINT32 gesture, UINT32 update)
{
    struct input_list_t *temp_input = NULL;
    struct input_list_t *temp_input_dele = NULL;
    struct output_list_t *temp_output = NULL;
    struct output_list_t *temp_output_dele = NULL;

    SM_LOG("INFO: xUpdateGestureMappingTable sensor(%d), gesture(%d), is_update(%d)", sensortype, gesture, update);
    if ((sensortype != SENSOR_TYPE_WAKE_GESTURE)
            || (sensortype != SENSOR_TYPE_GLANCE_GESTURE)
            || (sensortype != SENSOR_TYPE_PICK_UP_GESTURE)) {

        SM_ERR("ERR: Mapping type(%d) is error!", sensortype);
        return SM_ERROR;
    }

    if ((gesture != SENSOR_TYPE_TAP)
            || (gesture != SENSOR_TYPE_TWIST)
            || (gesture != SENSOR_TYPE_FLIP)
            || (gesture != SENSOR_TYPE_SNAPSHOT)
            || (gesture != SENSOR_TYPE_PICK_UP)
            || (gesture != SENSOR_TYPE_SHAKE)) {

        SM_ERR("ERR: Mapping gesture(%d) is error!", gesture);
        return SM_ERROR;
    }

    if (update == 1) {
        temp_input = gAlgorithm[sensortype].algo_desp.input_list;
        while (temp_input->next_input != NULL) {
            temp_input = temp_input->next_input;
        }

        temp_input->next_input = (struct input_list_t*)pvPortMalloc(sizeof(struct input_list_t));
        if (temp_input->next_input == NULL) {
            SM_ERR("Alloc memory for this sensor(%d) input list fail\n\r", sensortype);
            return SM_ERROR;
        }
        temp_input->next_input->input_type = gesture;
        temp_input->next_input->sampling_delay = -1;
        temp_input->next_input->next_input = NULL;


        //TODO: add output sensor to this function
        temp_output = gAlgorithm[gesture].next_output;
        while (temp_output->next_output != NULL) {
            temp_output = temp_output->next_output;
        }

        temp_output->next_output = (struct output_list_t*)pvPortMalloc(sizeof(struct output_list_t));
        if (temp_output->next_output == NULL) {
            SM_ERR("Alloc memory for this sensor(%d) input list fail\n\r", sensortype);
            return SM_ERROR;
        }

        temp_output->next_output->output_type = sensortype;
        temp_output->next_output->sampling_delay = -1;
        temp_output->next_output->next_output = NULL;

    } else {
        //remove this gesture from inputlist, outputlist
        temp_input = gAlgorithm[sensortype].algo_desp.input_list;
        while (temp_input->next_input->input_type != gesture) {
            temp_input = temp_input->next_input;
        }
        temp_input_dele = temp_input->next_input->next_input;
        vPortFree((void *)temp_input->next_input);
        temp_input->next_input = temp_input_dele;
        //TODO: remove from output list
        temp_output = gAlgorithm[gesture].next_output;
        while (temp_output->next_output->output_type != sensortype) {
            temp_output = temp_output->next_output;
        }
        temp_output_dele = temp_output->next_output->next_output;
        vPortFree((void *)temp_output->next_output);
        temp_output->next_output = temp_output_dele;
    }
    return SM_SUCCESS;
}
static bool isGyroEnabled(void)
{
    if (gAlgorithm[SENSOR_TYPE_GYROSCOPE].enable != 0)
        return true;
    return false;
}
static bool isAccEnabled(void)
{
    if (gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable != 0)
        return true;
    return false;
}
int xSetAccGyroStatusWhenGyroEnable(int gyroenable)
{
    int fifo_en = 0, ret = 0;
    if (gyroenable == SENSOR_ENABLE) {
        if (isAccEnabled()) {
            if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) == 0)
                    && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                // SENSOR_TYPE_ACCELEROMETER is enabled by framework only
            } else if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) == 0)
                       && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                // SENSOR_TYPE_ACCELEROMETER is enabled by others only
                fifo_en = FIFO_DISABLE;
                ret = gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
                if (ret < 0) {
                    SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                    return ret;
                }
                Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP / ACC_EVENT_COUNT_PER_FIFO_LOOP;
                gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = false;

            } else if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)
                       && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                //SENSOR_TYPE_ACCELEROMETER is enabled by both framework and other sensors
                fifo_en = FIFO_DISABLE;
                ret = gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
                if (ret < 0) {
                    SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                    return ret;
                }
                if (gAlgorithm[SENSOR_TYPE_ACCELEROMETER].delay <= ANDROID_SENSOR_GAME_SPEED)
                    Real_Delay[SENSOR_TYPE_ACCELEROMETER] = gAlgorithm[SENSOR_TYPE_ACCELEROMETER].delay;
                else
                    Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP / ACC_EVENT_COUNT_PER_FIFO_LOOP;

                gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = false;
            }
        }
    } else {
        if (!isGyroEnabled()) {
            if (isAccEnabled()) {
                if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) == 0)
                        && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                    // SENSOR_TYPE_ACCELEROMETER is enabled by framework only
                } else if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) == 0)
                           && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                    // SENSOR_TYPE_ACCELEROMETER is enabled by others only
                    fifo_en = FIFO_ENABLE;
                    ret = gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
                    if (ret < 0) {
                        SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                        return ret;
                    }
                    Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP;
                    gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = true;
                } else if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)
                           && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                    //SENSOR_TYPE_ACCELEROMETER is enabled by both framework and other sensors
                    fifo_en = FIFO_ENABLE;
                    ret = gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
                    if (ret < 0) {
                        SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                        return ret;
                    }
                    if (gAlgorithm[SENSOR_TYPE_ACCELEROMETER].delay <= ANDROID_SENSOR_UI_SPEED)
                        Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP;
                    else
                        Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP;
                    gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = true;
                }
            }
        }
    }
    return ret;
}
int xSetAccGyroStatusWhenAccEnable(int accenable)
{
    int fifo_en = 0, ret = 0;
    if (accenable == SENSOR_ENABLE) {
        if (!isGyroEnabled()) {

            if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) == 0)
                    && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                // SENSOR_TYPE_ACCELEROMETER is enabled by framework only
                gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = false;
            } else if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) == 0)
                       && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                // SENSOR_TYPE_ACCELEROMETER is enabled by others only
                Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP;
                fifo_en = FIFO_ENABLE;
                ret = gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
                if (ret < 0) {
                    SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                    return ret;
                }
                gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = true;
            } else if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)
                       && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                //SENSOR_TYPE_ACCELEROMETER is enabled by both framework and other sensors

                if (gAlgorithm[SENSOR_TYPE_ACCELEROMETER].delay <= ANDROID_SENSOR_UI_SPEED)
                    Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP;
                else
                    Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP;
                fifo_en = FIFO_ENABLE;
                ret = gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
                if (ret < 0) {
                    SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                    return ret;
                }
                gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = true;
            }

        } else {
            Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP / ACC_EVENT_COUNT_PER_FIFO_LOOP;
            gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = false;
        }
    } else {
        if (!isGyroEnabled()) {
            if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) == 0)
                    && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                // SENSOR_TYPE_ACCELEROMETER is enabled by framework only
                fifo_en = FIFO_DISABLE;
                ret = gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
                if (ret < 0) {
                    SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                    return ret;
                }
                gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = false;
            } else if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) == 0)
                       && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                // SENSOR_TYPE_ACCELEROMETER is enabled by others only
                Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP;
                gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = true;
            } else if (((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & ~(1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)
                       && ((gAlgorithm[SENSOR_TYPE_ACCELEROMETER].enable & (1ULL << SENSOR_TYPE_ACCELEROMETER)) != 0)) {
                //SENSOR_TYPE_ACCELEROMETER is enabled by both framework and other sensors

                if (gAlgorithm[SENSOR_TYPE_ACCELEROMETER].delay <= ANDROID_SENSOR_UI_SPEED)
                    Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP;
                else
                    Real_Delay[SENSOR_TYPE_ACCELEROMETER] = ACC_DELAY_PER_FIFO_LOOP;
                gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.hw.support_HW_FIFO = true;
            }
        }
    }
    return ret;
}
int xUpdateRealDelay(UINT8 sensor_type, UINT32 *this_sensor_delay, int enable)
{
    INT32 ret = 0;
    UINT32 temp_real_delay = 0xFFFF;
    struct output_list_t *output_delay_list = NULL;
    int fifo_en = 0;

    if (gAlgorithm[sensor_type].algo_desp.run_algorithm == NULL) {
        SM_ERR("Error: SM xUpdateRealDelay sensor(%d) fail, sensor not exsit!!\n\r", sensor_type);
        return SM_ERROR;
    }

    SM_LOG("INFO: 1.sensor(%d)xUpdateRealDelay(%d)FrameworkDelay(%d):!!\n\r", sensor_type, *this_sensor_delay,
           gAlgorithm[sensor_type].delay);

    output_delay_list = gAlgorithm[sensor_type].next_output;

    while (output_delay_list != NULL) {
        //SM_ERR("INFO:looping sensor(%d), delay(%d)!!\n\r", output_delay_list->output_type, output_delay_list->sampling_delay);
        if ((*this_sensor_delay > output_delay_list->sampling_delay) && (output_delay_list->sampling_delay > 0)) {
            *this_sensor_delay = output_delay_list->sampling_delay;
        }
        output_delay_list = output_delay_list->next_output;
    }

    //SM_LOG( "INFO:(%x),(%x)!!\n\r", (gAlgorithm[sensor_type].enable & ~(1ULL<<sensor_type)), (gAlgorithm[sensor_type].enable & (1ULL<<sensor_type)));


    //SM_LOG( "INFO:this_sensor_delay(%d)!!\n\r", *this_sensor_delay);

    if (((gAlgorithm[sensor_type].enable & (1ULL << sensor_type)) == 0)
            && ((gAlgorithm[sensor_type].enable & ~(1ULL << sensor_type)) != 0)) {
        //this sensor is enabled by other sensors
        //the delay value is set to the fastest required value
        if (enable == 0) {
            gAlgorithm[sensor_type].delay = 0xFFFF;//clear oringinal frame work delay value when disable by framework
            output_delay_list = gAlgorithm[sensor_type].next_output;
            while (output_delay_list != NULL) {
                //SM_LOG( "INFO:looping sensor(%d), delay(%d)!!\n\r", output_delay_list->output_type, output_delay_list->sampling_delay);
                if ((temp_real_delay > output_delay_list->sampling_delay) && (output_delay_list->sampling_delay > 0)) {
                    temp_real_delay = output_delay_list->sampling_delay;
                }
                output_delay_list = output_delay_list->next_output;
            }

            if (*this_sensor_delay < temp_real_delay) {
                *this_sensor_delay = temp_real_delay;
                SM_LOG("INFO:update real_delay when due to this sensor is enabled and used by other as input(%d)!\n\r",
                       *this_sensor_delay);
            }
        }
        switch (sensor_type) {
            case SENSOR_TYPE_ACCELEROMETER:
                xSetAccGyroStatusWhenAccEnable(enable);
                break;
            case SENSOR_TYPE_GYROSCOPE:
                xSetAccGyroStatusWhenGyroEnable(enable);
                fifo_en = FIFO_ENABLE;
                ret = gAlgorithm[sensor_type].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
                if (ret < 0) {
                    SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                    return ret;
                }
                break;
            default:
                break;
        }
        if (gAlgorithm[sensor_type].algo_desp.operate == NULL) {
            SM_ERR("Error: xUpdateRealDelay set failed sensor(%d) operate = NULL!!\n\r", sensor_type);
            return ret;
        }

        ret = gAlgorithm[sensor_type].algo_desp.operate(SETDELAY, this_sensor_delay, sizeof(*this_sensor_delay), NULL, 0);
        if (ret < 0) {
            SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
            return ret;
        }

        SM_LOG("INFO: this sensor(%d) delay (%d)is enabled by other sensors\n\r", sensor_type, *this_sensor_delay);
    } else if (((gAlgorithm[sensor_type].enable & ~(1ULL << sensor_type)) == 0)
               && ((gAlgorithm[sensor_type].enable & (1ULL << sensor_type)) != 0)) {
        //this sensor is enabled by framework only
        switch (sensor_type) {
            case SENSOR_TYPE_ACCELEROMETER:
                *this_sensor_delay = gAlgorithm[sensor_type].delay;
                xSetAccGyroStatusWhenAccEnable(enable);
                break;
            case SENSOR_TYPE_GYROSCOPE:
            case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
                xSetAccGyroStatusWhenGyroEnable(enable);
                if (gAlgorithm[sensor_type].delay <= GYRO_DELAY_PER_FIFO_LOOP) {
                    *this_sensor_delay = GYRO_DELAY_PER_FIFO_LOOP;
                    fifo_en = FIFO_ENABLE;
                    ret = gAlgorithm[sensor_type].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
                    if (ret < 0) {
                        SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                        return ret;
                    }
                } else {
                    *this_sensor_delay = gAlgorithm[sensor_type].delay;
                }
                break;
            default:
                *this_sensor_delay = gAlgorithm[sensor_type].delay;
                break;
        }

        if (gAlgorithm[sensor_type].algo_desp.operate == NULL) {
            SM_ERR("Error: xUpdateRealDelay set failed sensor(%d) operate = NULL!!\n\r", sensor_type);
            return ret;
        }

        ret = gAlgorithm[sensor_type].algo_desp.operate(SETDELAY, this_sensor_delay, sizeof(*this_sensor_delay), NULL, 0);
        if (ret < 0) {
            SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
            return ret;
        }

        SM_LOG("INFO: this sensor(%d) delay(%d) gdelay(%d)is enabled by framework\n\r", sensor_type, *this_sensor_delay,
               gAlgorithm[sensor_type].delay);
    } else if (((gAlgorithm[sensor_type].enable & ~(1ULL << sensor_type)) != 0)
               && ((gAlgorithm[sensor_type].enable & (1ULL << sensor_type)) != 0)) {
        //this sensor is enabled by both framework and other sensors
        switch (sensor_type) {
            case SENSOR_TYPE_ACCELEROMETER:
                if (gAlgorithm[sensor_type].delay <= ANDROID_SENSOR_UI_SPEED) {
                    *this_sensor_delay = ACC_DELAY_PER_FIFO_LOOP;
                }
                xSetAccGyroStatusWhenAccEnable(enable);
                break;
            case SENSOR_TYPE_GYROSCOPE:
            case SENSOR_TYPE_GYROSCOPE_UNCALIBRATED:
                if (gAlgorithm[sensor_type].delay <= GYRO_DELAY_PER_FIFO_LOOP) {
                    if (*this_sensor_delay > GYRO_DELAY_PER_FIFO_LOOP) {
                        *this_sensor_delay = GYRO_DELAY_PER_FIFO_LOOP;
                    }
                } else {
                    if (*this_sensor_delay > gAlgorithm[sensor_type].delay) {
                        *this_sensor_delay = gAlgorithm[sensor_type].delay;
                    }
                }
                xSetAccGyroStatusWhenGyroEnable(enable);
                fifo_en = FIFO_ENABLE;
                ret = gAlgorithm[sensor_type].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
                if (ret < 0) {
                    SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                    return ret;
                }
                break;
            default:
                if (*this_sensor_delay > gAlgorithm[sensor_type].delay) {
                    *this_sensor_delay = gAlgorithm[sensor_type].delay;
                }
                break;
        }
        if (gAlgorithm[sensor_type].algo_desp.operate == NULL) {
            SM_ERR("Error: xUpdateRealDelay set failed sensor(%d) operate = NULL!!\n\r", sensor_type);
            return ret;
        }
        ret = gAlgorithm[sensor_type].algo_desp.operate(SETDELAY, this_sensor_delay, sizeof(*this_sensor_delay), NULL, 0);
        if (ret < 0) {
            SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
            return ret;
        }
        SM_LOG("INFO: this sensor(%d) delay(%d) gdelay(%d)is enabled by both framework and other sensors\n\r", sensor_type,
               *this_sensor_delay, gAlgorithm[sensor_type].delay);
    } else if (((gAlgorithm[sensor_type].enable & ~(1ULL << sensor_type)) == 0)
               && ((gAlgorithm[sensor_type].enable & (1ULL << sensor_type)) == 0)) { //this sensor is disabled finally
        SM_LOG("INFO: this sensor(%d) is disabled finally\n\r", sensor_type);

        switch (sensor_type) {
            case SENSOR_TYPE_ACCELEROMETER:
                xSetAccGyroStatusWhenAccEnable(enable);
                break;
            case SENSOR_TYPE_GYROSCOPE:
                xSetAccGyroStatusWhenGyroEnable(enable);
                break;
            default:
                break;
        }
        gAlgorithm[sensor_type].delay = 0xFFFF;
        *this_sensor_delay = 0xFFFF;
    }

    SM_LOG("INFO: 2.sensor(%d)xUpdateRealDelay(%d)FrameworkDelay(%d):!!\n\r", sensor_type, *this_sensor_delay,
           gAlgorithm[sensor_type].delay);
    return ret;
}

static bool xChoosePollingDelay(UINT32 *delay)
{
    INT32 i;
    //UINT32 temp_delay[SENSOR_TYPE_MAX_COUNT+1] = {0};
    UINT32 return_delay = 200;

    for (i = 1; i < SENSOR_TYPE_MAX_COUNT + 1; i++) {
        //SM_LOG("xChoosePollingDelay1(%d): Real_Delay(%d) gAlgorithm(%d)!\n\r", i, Real_Delay[i], gAlgorithm[i].delay);
        //temp_delay[i] = (Real_Delay[i]>gAlgorithm[i].delay)?(gAlgorithm[i].delay):(Real_Delay[i]);
        if (return_delay > Real_Delay[i]) {
            return_delay = Real_Delay[i];
        }
    }

    SM_LOG("xChoosePollingDelay2 inputdelay(%d) return_delay(%d)!\n\r", *delay, return_delay);

    if (*delay != return_delay) {
        *delay = return_delay;
        return true;
    }
    return false;
}

static bool bCheckSensorState(void)
{
    int i = 0;
    for (i = 1; i < SENSOR_TYPE_MAX_COUNT + 1; i++) {
        if ((gAlgorithm[i].enable != 0) && ((gManagerDataInfo.intr_used_bit & (1ULL << i)) == 0)) {
            SM_LOG("bCheckSensorState(%d) enabled!\n\r", i);
            return true;
        }
    }
    return false;
}

static void bReachSensorDelay(UINT8 sensortype, UINT64 now_time, UINT32 delay_time)
{
    SM_LOG("sensor(%d)(%d)now_time(%lld), gManagerDataInfo.last_report_time(%lld), delay(%d)!!\n\r", sensortype,
           now_time, gManagerDataInfo.last_report_time[sensortype], delay_time);
    if (((now_time - gManagerDataInfo.last_report_time[sensortype]) >= delay_time)
            && ((gAlgorithm[sensortype].enable & (1LL << sensortype)) != 0)) {
        SM_LOG("INFO: SM gAlgorithm[%d] sensor timeout reached so report this sensor\n\r", sensortype);
        xSemaphoreTake(xSMSemaphore[sensortype], portMAX_DELAY);
        if (gAlgorithm[sensortype].algo_desp.report_mode == continus) {
            gManagerDataInfo.data_ready_bit |= 1ULL << sensortype;
        }
        xSemaphoreGive(xSMSemaphore[sensortype]);
        gManagerDataInfo.last_report_time[sensortype] = now_time;
    }
}

static int xHandleSensorFlushNewest(UINT8 sensortype, struct data_t *data)
{
    int ret = 0, i = 0;
    struct output_list_t *sensor_list = NULL;
    int output_sensor_type = -1;
    UINT8 FifoData[512];
    int data_size = 0;


    SM_LOG("xHandleSensorFlushNewest: (%d)\n\r", sensortype);

    //add check for whether timeout has come
    if (gAlgorithm[sensortype].algo_desp.run_algorithm == NULL) {
        SM_ERR("Error: xHandleSensorFlushNewest run_algorithm failed sensor(%d) run_algorithm = NULL!!\n\r", sensortype);
        return -1;
    }

    ret = gAlgorithm[sensortype].algo_desp.run_algorithm(data);
    if (ret < 0) {
        SM_ERR("ERR: SM run_algorithm failed at type:%d\n\r", sensortype);
        return ret;
    }
    for (; i < data->data_exist_count; ++i) {
        data->data[i].time_stamp_gpt = gManagerDataInfo.timestamp_offset_to_ap;
    }
    if (gAlgorithm[sensortype].next_output != NULL) {

        sensor_list = gAlgorithm[sensortype].next_output;

        do {
            output_sensor_type = sensor_list->output_type;

            if (gAlgorithm[output_sensor_type].algo_desp.set_data == NULL) {
                SM_ERR("Error: xHandleSensorAlgrithm set_data failed sensor(%d) set_data = NULL!!\n\r", sensortype);
                return -1;
            }
            if ((gAlgorithm[sensortype].enable & (1ULL << output_sensor_type)) != 0) {
                ret = gAlgorithm[output_sensor_type].algo_desp.set_data(gAlgorithm[sensortype].newest, NULL);
                if (ret < 0) {
                    return ret;
                }
                SM_LOG("type: %d is enabled by type: %d\r\n", sensortype, output_sensor_type);
            }
            sensor_list = sensor_list->next_output;
        } while (sensor_list != NULL);

    }
    if (((gBatchInfo.ap_batch_activate & (1ULL << sensortype)) != 0) ||
            ((gBatchInfo.scp_direct_push_activate & (1ULL << sensortype)) != 0)) {
        xSemaphoreTake(xSMSemaphore[sensortype], portMAX_DELAY);
        data_size = gAlgorithm[sensortype].newest->data_exist_count;
        vDataFormateTransfer(gAlgorithm[sensortype].newest, FifoData, &data_size);
        vPushDataToFifo(FifoData, data_size);
        xSemaphoreGive(xSMSemaphore[sensortype]);
    }
    return ret;
}
static int xcalcuSensorSamplingDelay(UINT8 input_type, int output_sampling)
{
    int sampling_delay = 0;
    if (gAlgorithm[input_type].algo_desp.hw.support_HW_FIFO == true) {
        sampling_delay = output_sampling;
    } else {
        if (input_type == SENSOR_TYPE_ACCELEROMETER)
            sampling_delay = output_sampling / ACC_EVENT_COUNT_PER_FIFO_LOOP;
        else
            sampling_delay = output_sampling;
    }
    return sampling_delay;
}
static int xHandleSensorAlgrithm(UINT8 sensortype, struct data_t *data)
{
    int ret = 0, i = 0;
    struct output_list_t *sensor_list = NULL;
    UINT8 output_sensor_type = -1;
    UINT8 FifoData[512];
    int data_size = 0;
    //struct data_t dummy_data;
    //struct data_unit_t dummy_data_unit;
    memset(FifoData, 0, sizeof(FifoData));
    now_time = read_xgpt_stamp_ns() / 1000000;

    SM_LOG("xHandleSensorAlgrithm: (%d)\n\r", sensortype);

    //SM_ERR("INFO: SM gAlgorithm[%d], lasttime:%lld, nowtime: %lld, real_delay: %d\n\r",
    //sensortype, gManagerDataInfo.last_real_time[sensortype], now_time, Real_Delay[sensortype]);
    if ((now_time - gManagerDataInfo.last_real_time[sensortype]) >= (Real_Delay[sensortype] - TIME_DEVIATION)) {

        //SM_ERR("INFO: SM gAlgorithm[%d], nowtime: %lld, real_delay: %d\n\r", sensortype, now_time, Real_Delay[sensortype]);

        if (gAlgorithm[sensortype].algo_desp.run_algorithm == NULL) {
            gManagerDataInfo.last_real_time[sensortype] = now_time;
            SM_ERR("Error: xHandleSensorAlgrithm run_algorithm failed sensor(%d) run_algorithm = NULL!!\n\r", sensortype);
            return -1;
        }

        ret = gAlgorithm[sensortype].algo_desp.run_algorithm(data);
        if (ret < 0) {
            gManagerDataInfo.last_real_time[sensortype] = now_time;
            return ret;
        }
        SM_ERR("type :%d, data_exist_count : %d\n", sensortype, data->data_exist_count);
        for (i = 0; i < data->data_exist_count; ++i) {
            data->data[i].time_stamp_gpt = gManagerDataInfo.timestamp_offset_to_ap;
        }
        if (0 == data->data[0].time_stamp) {
            gManagerDataInfo.last_real_time[sensortype] = now_time;
            SM_LOG("sensor: %d old data need throw away\n", sensortype);
            return 0;
        }
        if (gAlgorithm[sensortype].next_output != NULL) {

            sensor_list = gAlgorithm[sensortype].next_output;
            do {
                output_sensor_type = sensor_list->output_type;
                if ((((now_time - gManagerDataInfo.last_real_time[sensortype])) * (++sensor_list->count))
                        >= (xcalcuSensorSamplingDelay(sensortype, sensor_list->sampling_delay) - TIME_DEVIATION)) {
                    if (gAlgorithm[output_sensor_type].algo_desp.set_data == NULL) {
                        gManagerDataInfo.last_real_time[sensortype] = now_time;
                        SM_ERR("Error: xHandleSensorAlgrithm set_data failed sensor(%d) set_data = NULL!!\n\r", sensortype);
                        return -1;
                    }
                    if ((gAlgorithm[sensortype].enable & (1ULL << output_sensor_type)) != 0) {

                        ret = gAlgorithm[output_sensor_type].algo_desp.set_data(gAlgorithm[sensortype].newest, NULL);
                        if (ret < 0) {
                            return ret;
                        }
                        SM_LOG("type: %d, count: %d\r\n", output_sensor_type, sensor_list->count);
                    }
                    sensor_list->count = 0;
                }
                sensor_list = sensor_list->next_output;
            } while (sensor_list != NULL);
        }

        if (((gBatchInfo.ap_batch_activate & (1ULL << sensortype)) != 0) ||
                ((gBatchInfo.scp_direct_push_activate & (1ULL << sensortype)) != 0)) {

            xSemaphoreTake(xSMSemaphore[sensortype], portMAX_DELAY);
            data_size = gAlgorithm[sensortype].newest->data_exist_count;
            vDataFormateTransfer(gAlgorithm[sensortype].newest, FifoData, &data_size);
            vPushDataToFifo(FifoData, data_size);
            xSemaphoreGive(xSMSemaphore[sensortype]);
        }
        gManagerDataInfo.last_real_time[sensortype] = now_time;
    }

    if (gAlgorithm[sensortype].last_accuracy != data->data->accelerometer_t.status) { //check accuracy is changed
        gManagerDataInfo.accuracy_change_bit |= 1ULL << sensortype;
        gAlgorithm[sensortype].last_accuracy = data->data->accelerometer_t.status;
    }

#ifdef FWQLOG
    //dump log
    if (SENSOR_TYPE_ACCELEROMETER == sensortype) {
        SM_LOG("fwq INFO: SM gAlgorithm[%d] (%d,%d,%d) t_scp=%lld,t_gpt=%lld \n\r", sensortype, data->data->accelerometer_t.x,
               data->data->accelerometer_t.y, data->data->accelerometer_t.z, data->data->time_stamp,
               data->data->time_stamp_gpt);
        // push raw data to tcmbuffer
        xHandleSensorLog(SENSOR_TYPE_ACCELEROMETER); //get each sensor data, depends on activate state.
        xHandleSensorBufferLog(); //check flush flag, put data into buffer, if buffer full, flush to DRAM
        SM_LOG("fwq INFO: log done\n\r");
    }
#endif

    return ret;
}

#ifdef FWQLOG

static int xHandleSensorLog(int sensortype)
{
    int ret = -1;
    struct data_unit_t *FifoData = NULL;
    int data_size = 0;

    now_time = read_xgpt_stamp_ns();
    SM_LOG("INFO: SM read_xgpt_stamp_ns:(%lld)\n\r", now_time);

    now_time = now_time / 1000000;
    SM_LOG("INFO: SM now_time:(%lld)\n\r", now_time);


    xSemaphoreTake(xSMSemaphore[sensortype], portMAX_DELAY);
    data_size = gAlgorithm[sensortype].newest->data_exist_count;
    vDataFormateTransfer(gAlgorithm[sensortype].newest, &FifoData, &data_size);
    vPushDataToFifo(FifoData, data_size);
    xSemaphoreGive(xSMSemaphore[sensortype]);
    if (FifoData != NULL)
        vPortFree((void *)FifoData);
    SM_LOG("fwq INFO: xHandleSensorLog done\n\r");
    return ret;
}

#endif

static void vPushDataToFifo(UINT8 *src, int size)
{
    /*int part1=0;
    int part2=0 ;*/

    //SM_ERR("%s : start, size = %d, rp = %d, wp = %d\n\r", __func__,  size, gBatchFifo->rp, gBatchFifo->wp);

    if (size == 0) {
        return;
    }

    memcpy((UINT8 *)gBatchFifo->data + gBatchFifo->wp, src, size);
    gBatchFifo->wp += size;

    if ((gBatchFifo->FIFOSize - gBatchFifo->wp) < SCP_SENSOR_BATCH_FIFO_THRESHOLD)  {
        xFlushScpFifo();
        //SM_ERR("overflow SCP_SENSOR_BATCH_FIFO_THRESHOLD so xFlushScpFifo done\n");
    }
    return;

    /*
        if(gBatchFifo->wp >= gBatchFifo->rp)
            part1 = ((UINT8 *)&gBatchFifo->data[0] + SCP_SENSOR_BATCH_FIFO_BATCH_SIZE)-((UINT8 *)gBatchFifo->wp);
        else
            part1 = (UINT8 *)gBatchFifo->rp - (UINT8 *)gBatchFifo->wp;
        if(part1>=size) { //NOT wrap scp fifo
            memcpy((void *)gBatchFifo->wp, src, size);
            gBatchFifo->wp = (struct data_unit_t *)((UINT8 *)gBatchFifo->wp + size);
        } else {
            part2 = size-part1;

            if (gBatchFifo->wp >=gBatchFifo->rp)
            {
                memcpy((void *)gBatchFifo->wp, src, part1);
                memcpy(gBatchFifo->data, (UINT8 *)src + part1, part2);
                 gBatchFifo->wp = (struct data_unit_t *)((UINT8 *)&gBatchFifo->data[0]+part2);
                 if (gBatchFifo->wp >=gBatchFifo->rp)
                  {
                    gBatchFifo->rp += 48*((gBatchFifo->wp -gBatchFifo->rp)/48 + 1);
                  }

            }
            else
            {

                if (((UINT8 *)&gBatchFifo->data[0] + SCP_SENSOR_BATCH_FIFO_BATCH_SIZE - gBatchFifo->wp) > size)
                  {
                    memcpy((void *)gBatchFifo->wp, src, part1);
                    memcpy(gBatchFifo->data, (UINT8 *)src + part1, part2);
                    gBatchFifo->wp = (struct data_unit_t *)((UINT8 *)&gBatchFifo->data[0]+part2);
                    gBatchFifo->rp += 48*((gBatchFifo->wp -gBatchFifo->rp)/48 + 1);
                    if (gBatchFifo->rp > ((UINT8 *)&gBatchFifo->data[0] + SCP_SENSOR_BATCH_FIFO_BATCH_SIZE))
    //                   SM_ERR("rp2  over write\r\n")
                     {
                         gBatchFifo->rp += ((UINT8 *)gBatchFifo->rp - ( (UINT8 *)&gBatchFifo->data[0] + SCP_SENSOR_BATCH_FIFO_BATCH_SIZE))  ;
                      }
                  }
                 else
                {
                  part1 =((UINT8 *)&gBatchFifo->data[0] + SCP_SENSOR_BATCH_FIFO_BATCH_SIZE - gBatchFifo->wp);
                  part2 =  size - (((UINT8 *)&gBatchFifo->data[0] + SCP_SENSOR_BATCH_FIFO_BATCH_SIZE )- (UINT8 *)gBatchFifo->wp);
                    memcpy((void *)gBatchFifo->wp, src, part1);
                    memcpy(gBatchFifo->data, (UINT8 *)src + part1, part2);
                    gBatchFifo->rp += 48*((gBatchFifo->wp -gBatchFifo->rp)/48 + 2);
                    gBatchFifo->rp  =  (gBatchFifo->rp -  + SCP_SENSOR_BATCH_FIFO_BATCH_SIZE);

                }

            }
            gBatchFifo->wp = (struct data_unit_t *)((UINT8 *)&gBatchFifo->data[0]+part2);
        }

        //gBatchFifo->wp = (struct UINT8 *)((((UINT32)gBatchFifo->wp+3)/4)*4); //4 bytes aligned

    */
}

static void vDataFormateTransfer(struct data_t *data, UINT8 *sensor_event, int *size)
{
    int event_size = 0;
    int i = 0;
    UINT8 *pDest = NULL;
    struct data_unit_t *pSrc = NULL;
    struct data_unit_t *pSensorEvent = NULL;
    event_size = data->data_exist_count;

    pDest = sensor_event;
    pSrc = data->data;
    for (i = 0; i < event_size; ++i) {
        memcpy(pDest, pSrc, SENSOR_DATA_SIZE);
        pSensorEvent = (struct data_unit_t *)pDest;
        if (pSensorEvent->sensor_type == SENSOR_TYPE_GYROSCOPE
                || pSensorEvent->sensor_type == SENSOR_TYPE_GYROSCOPE_UNCALIBRATED) {
            pSensorEvent->gyroscope_t.x = pSensorEvent->gyroscope_t.x * GYROSCOPE_INCREASE_NUM_AP / GYROSCOPE_INCREASE_NUM_SCP;
            pSensorEvent->gyroscope_t.y = pSensorEvent->gyroscope_t.y * GYROSCOPE_INCREASE_NUM_AP / GYROSCOPE_INCREASE_NUM_SCP;
            pSensorEvent->gyroscope_t.z = pSensorEvent->gyroscope_t.z * GYROSCOPE_INCREASE_NUM_AP / GYROSCOPE_INCREASE_NUM_SCP;
            pSensorEvent->gyroscope_t.x_bias = pSensorEvent->gyroscope_t.x_bias * GYROSCOPE_INCREASE_NUM_AP /
                                               GYROSCOPE_INCREASE_NUM_SCP;
            pSensorEvent->gyroscope_t.y_bias = pSensorEvent->gyroscope_t.y_bias * GYROSCOPE_INCREASE_NUM_AP /
                                               GYROSCOPE_INCREASE_NUM_SCP;
            pSensorEvent->gyroscope_t.z_bias = pSensorEvent->gyroscope_t.z_bias * GYROSCOPE_INCREASE_NUM_AP /
                                               GYROSCOPE_INCREASE_NUM_SCP;
        }
        pSensorEvent->sensor_type = xSensorTypeMappingToAP(pSensorEvent->sensor_type);
        pDest += SENSOR_DATA_SIZE;
        pSrc++;
    }
    *size = SENSOR_DATA_SIZE * event_size;

    SM_LOG("vDataFormateTransfer done sensortype:%d, data count:%d!\n\r", data->data->sensor_type, *size);
}
static int xHandleSensorData(void)
{
    int ret = -1;
    int i = 0;

    for (i = 1; i < SENSOR_TYPE_MAX_COUNT + 1; i++) {
        if ((gAlgorithm[i].enable == 0) || ((gManagerDataInfo.intr_used_bit & (1ULL << i)) != 0)) {
            continue;
        }

        if (gAlgorithm[i].newest == NULL) {
            SM_LOG("INFO: SM gAlgorithm[%d].newest == NULL\n\r", i);
            continue;
        }

        ret = xHandleSensorAlgrithm(i, gAlgorithm[i].newest);//real delay check
        if (ret < 0) {
            SM_ERR("ERR: SM xHandleSensorAlgrithm[%d] failed\n\r", i);
            continue;
        }

        bReachSensorDelay(i, now_time, gAlgorithm[i].delay);//polling delay from sensor framework


    }

    if (gManagerDataInfo.data_ready_bit != 0) {
        ret = xNotifyFramework(SENSOR_TYPE_MAX_COUNT, DATA_CHANGED);
        if (ret < 0) {
            SM_ERR("ERR: SM xNotifyFramework[datachanged] failed\n\r", i);
            return ret;
        }
    }

    if (gManagerDataInfo.accuracy_change_bit != 0) {
        ret = xNotifyFramework(SENSOR_TYPE_MAX_COUNT, ACCURACY_CHANGED);
        if (ret < 0) {
            SM_ERR("ERR: SM xNotifyFramework[datachanged] failed\n\r", i);
            return ret;
        }
    }
    /*store timestamp of this sensor to check sensor reportrate*/
    //TODO:the timing to set last report time is wrong, we need a array for every sensor to save their last report time
    //gManagerDataInfo.last_report_time = now_time; //this operation is moved to line402

    return ret;
}

#ifdef FWQLOG

static int xHandleSensorBufferLog(void)
{
    UINT32 size_left; //ues unsign int for left value, rp-wp equals abs(rp-wp), don't care (rp > wp) or not

    SM_LOG("xHandleSensorBuffer start\n\r");

    //enter_critical_section();
    if (gBatchFifo->wp >= gBatchFifo->rp)
        size_left = gBatchFifo->FIFOSize - ((UINT32)gBatchFifo->wp - (UINT32)gBatchFifo->rp);
    else
        size_left = (UINT32)gBatchFifo->rp - (UINT32)gBatchFifo->wp;
    //any sensor needs flush, we flush all sensor data
    //exit_critical_section();
    SM_LOG("xHandleSensorBuffer, size_left = %d\n\r", size_left);
    if ((size_left < SCP_SENSOR_BATCH_FIFO_THRESHOLD)
            || (gBatchInfo.ap_batch_timeout)) { //scp fifo almost full, write to DRAM
        xFlushScpFifo();
        SM_LOG("fwq scpfifo full\n\r");

        if (xQueryDramFull()) {
            //Send message to sensor framework to inform sensor framework to notify AP
            SM_LOG("fwq DRAM fifo full\n\r");
            xNotifyFramework(SENSOR_TYPE_MAX_COUNT, BATCH_DRAMFULL);
        }
    }

    SM_LOG("sensor_manager_handle_buffer end\n\r");
    return 0;
}
#endif

//return physical memory address
unsigned int xSensorManagerGetDramWPAddr(struct sensorFIFO *dram_fifo)
{
    return (unsigned int)gManagerDataInfo.bufferBase + offsetof(struct sensorFIFO, wp);
}
//return physical memory address
unsigned int xSensorManagerGetDramWP(struct sensorFIFO *dram_fifo)
{
    return (unsigned int)gManagerDataInfo.bufferBase->data + (unsigned int)dram_fifo->wp;
}
//return physical memory address
unsigned int xSensorManagerGetDramRP(struct sensorFIFO *dram_fifo)
{
    return (unsigned int)gManagerDataInfo.bufferBase->data + (unsigned int)dram_fifo->rp;
}

//return physical memory address
unsigned int xSensorManagerGetDramRPAddr(struct sensorFIFO *dram_fifo)
{
    return (unsigned int)gManagerDataInfo.bufferBase + offsetof(struct sensorFIFO, rp);
}

static int xPushDataToDram(struct data_unit_t *src, unsigned int size)
{
    struct sensorFIFO *dram_fifo;
    unsigned int size_left;
    //unsigned int size_rp2end;
    unsigned int size_wp2end;
    unsigned int size_rp_moving;
    DMA_RESULT ret;

    if (size == 0)
        return 0;
    //SM_ERR("%s : size = %d, sensor type : %d addr : %p , data: %d %d %d\n\r",
    //     __func__, size, src->sensor_type, src, src->value[0], src->value[1], src->value[2]);

    if (get_scp_semaphore(SEMAPHORE_SENSOR) < 0) {
        printf("xPushDataToDram, wait semaphore timeout\n\r");
        return 0;
    }
    dvfs_enable_DRAM_resource(SENS_MEM_ID);
    get_emi_semaphore();
    dram_fifo = (struct sensorFIFO *)gManagerDataInfo.bufferBase;
    if (dram_fifo->wp >= dram_fifo->rp)
        size_left = dram_fifo->FIFOSize - (dram_fifo->wp - dram_fifo->rp);
    else
        size_left = dram_fifo->rp - dram_fifo->wp;

    //size_rp2end = dram_fifo->FIFOSize - dram_fifo->rp;
    size_wp2end = dram_fifo->FIFOSize - dram_fifo->wp;

    /* step1 */
    if (size <= size_wp2end) {
        // SM_ERR("clj store_part1 --> size_wp2end: %d .\n", size_wp2end);
        ret = dma_transaction(xSensorManagerGetDramWP(dram_fifo), (unsigned int)src, size);
        if (ret != DMA_RESULT_DONE) {
            SM_ERR("dma_transaction failed!\n\r");
            goto err;
        }
    } else {
        //SM_ERR("clj store_part1.1 -->need to divide into tow parts : size_wp2end: %d .\n", size_wp2end);
        ret = dma_transaction(xSensorManagerGetDramWP(dram_fifo), (unsigned int)src, size_wp2end);
        if (ret != DMA_RESULT_DONE) {
            SM_ERR("dma_transaction failed!\n\r");
            goto err;
        }
        ret = dma_transaction((unsigned int)gManagerDataInfo.bufferBase->data, (unsigned int)src + size_wp2end,
                              size - size_wp2end);
        if (ret != DMA_RESULT_DONE) {
            SM_ERR("dma_transaction failed!\n\r");
            goto err;
        }
    }

    /* step2 */
    dram_fifo->wp = (dram_fifo->wp + size) % dram_fifo->FIFOSize;

    /* step3 */
    if (size > size_left) {
        size_rp_moving = ((size - size_left) / SENSOR_DATA_SIZE + 1) * SENSOR_DATA_SIZE;
        dram_fifo->rp = (dram_fifo->rp + size_rp_moving) % dram_fifo->FIFOSize;
    }
    // SM_ERR("xPushDataToDram done, dram_fifo.rp =%x wp = %x type = %d \n\r", dram_fifo->rp, dram_fifo->wp,
    //        ((struct data_unit_t *)xSensorManagerGetDramRP(dram_fifo))->sensor_type);

    ret = 1;
err:
    release_emi_semaphore();
    dvfs_disable_DRAM_resource(SENS_MEM_ID);
    if (release_scp_semaphore(SEMAPHORE_SENSOR) < 0) {
        SM_ERR("xPushDataToDram, release semaphore failed\n\r");
    }
    return ret;
}

static void xFlushScpFifo(void)
{
    xPushDataToDram(gBatchFifo->data, gBatchFifo->wp);
    gBatchFifo->wp = 0;
}

static int xQueryDramFull(void)
{
    struct sensorFIFO *dram_fifo;
    unsigned int size_left;

    if (get_scp_semaphore(SEMAPHORE_SENSOR) < 0) {
        printf("%s, wait semaphore timeout\n\r", __func__);
        return -1;
    }
    dvfs_enable_DRAM_resource(SENS_MEM_ID);
    get_emi_semaphore();

    dram_fifo = (struct sensorFIFO *)gManagerDataInfo.bufferBase;
    if (dram_fifo->wp >= dram_fifo->rp)
        size_left = dram_fifo->FIFOSize - (dram_fifo->wp - dram_fifo->rp);
    else
        size_left = dram_fifo->rp - dram_fifo->wp;

    release_emi_semaphore();
    dvfs_disable_DRAM_resource(SENS_MEM_ID);
    if (release_scp_semaphore(SEMAPHORE_SENSOR) < 0) {
        SM_ERR("%s, release semaphore failed\n\r", __func__);
        return -1;
    }

    return size_left < SENSOR_DATA_SIZE ? FIFO_FULL : FIFO_NO_FULL;
}


static int xHandleSensorBuffer(void)
{

    SM_LOG("xHandleSensorBuffer start\n\r");

    if (gBatchInfo.ap_batch_timeout || gBatchInfo.scp_direct_push_timeout
            || gBatchInfo.wakeup_batch_timeout) {
        xFlushScpFifo();
        if (gBatchInfo.ap_batch_timeout == 1) {
            gBatchInfo.ap_batch_timeout = 0;
            xNotifyFramework(SENSOR_TYPE_MAX_COUNT, BATCH_TIMEOUT_NOTIFY);
            //SM_ERR("ap batch timeout so xFlushScpFifo and xNotifyFramework\n");
        }
        if (gBatchInfo.scp_direct_push_timeout == 1) {
            gBatchInfo.scp_direct_push_timeout = 0;
            xNotifyFramework(SENSOR_TYPE_MAX_COUNT, DIRECT_PUSH_NOTIFY);
            //SM_ERR("scp direct push timeout so xFlushScpFifo and xNotifyFramework\n");
        }
        if (gBatchInfo.wakeup_batch_timeout == 1) {
            gBatchInfo.wakeup_batch_timeout = 0;
            xNotifyFramework(SENSOR_TYPE_MAX_COUNT, BATCH_WAKEUP_NOTIFY);
            //SM_ERR("wakeup timeout so xFlushScpFifo and xNotifyFramework\n");
        }
    }

    if (gBatchInfo.wake_up_on_fifo_full && xQueryDramFull() == FIFO_FULL) {
        SM_ERR("batch dram full so xNotifyFramework\n");
        xNotifyFramework(SENSOR_TYPE_MAX_COUNT, BATCH_DRAMFULL_NOTIFY);
    }

    SM_LOG("sensor_manager_handle_buffer end\n\r");
    return 0;
}
static int xEnableSensorAlgrithm(UINT8 sensor_type, int enable)
{
    int ret = 0;
    int fifo_en = 0;

    if (gAlgorithm[sensor_type].algo_desp.run_algorithm == NULL) {
        //configASSERT(0);
        SM_ERR("Error: SM enable HW sensor(%d) fail, sensor not exsit!!\n\r", sensor_type);
        return SM_ERROR;
    }

    SM_LOG("INFO: xEnableSensorAlgrithm end!!\n\r");

    if (gAlgorithm[sensor_type].algo_desp.hw.max_sampling_rate > 0) { //HW sensor
        //inform HW sensor enable or disable through set_data 2nd parameter (void *)
        if (gAlgorithm[sensor_type].algo_desp.operate == NULL) {
            configASSERT(0);
            SM_ERR("Error: SM enable HW sensor(%d) operate = NULL!!\n\r", sensor_type);
            return -1;
        }
        if ((enable == SENSOR_DISABLE) && (sensor_type == SENSOR_TYPE_ACCELEROMETER)) {
            //if (!isGyroEnabled()) {
            fifo_en = FIFO_DISABLE;
            ret = gAlgorithm[SENSOR_TYPE_ACCELEROMETER].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
            if (ret < 0) {
                SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                return ret;
            }
            //}
        }

        if ((enable == SENSOR_DISABLE) && (sensor_type == SENSOR_TYPE_GYROSCOPE)) {
            //if (!isAccEnabled()) {
            fifo_en = FIFO_DISABLE;
            ret = gAlgorithm[SENSOR_TYPE_GYROSCOPE].algo_desp.operate(ENABLEFIFO, &fifo_en, sizeof(fifo_en), NULL, 0);
            if (ret < 0) {
                SM_ERR("xUpdateRealDelay set failed for this sensor\n\r ");
                return ret;
            }
            //}
        }

        ret = gAlgorithm[sensor_type].algo_desp.operate(ACTIVATE, &enable, sizeof(enable), NULL, 0);
        if (enable == SENSOR_DISABLE) {
            switch (sensor_type) {
                case SENSOR_TYPE_PROXIMITY:
                    break;
                default:
                    gAlgorithm[sensor_type].newest->data_exist_count = 0;
                    memset(gAlgorithm[sensor_type].newest->data, 0,
                           sizeof(struct data_unit_t) * gAlgorithm[sensor_type].exist_data_count);
                    SM_ERR("flush data buffer to 0 when disable sensor: %d\n", sensor_type);
                    break;
            }
        }
        if (ret < 0) {
            SM_ERR("Error: SM enable HW sensor(%d) fail!!\n\r", sensor_type);
            return ret;
        }
    }

    SM_LOG("INFO: SM enable sensor(%d) success(enable = 0x%x)!!\n\r", sensor_type, gAlgorithm[sensor_type].enable);
    return ret;
}
static void xCheckEnableSensorBitMap(UINT8 sensortype, int enable, UINT64 bitmap)
{
    int i = 0;
    switch (sensortype) {
        case SENSOR_TYPE_ACCELEROMETER:
        case SENSOR_TYPE_GYROSCOPE:
        case SENSOR_TYPE_PRESSURE:
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SENSOR_TYPE_LIGHT:
        case SENSOR_TYPE_PROXIMITY:
            SM_LOG("HWSENSOR: xEnableSensorAlgrithm enter!! %d\n\r", sensortype);
            if (enable) {
                SM_LOG("enable sensortype :%d\r\n", sensortype);
            } else {
                SM_LOG("disable sensortype :%d\r\n", sensortype);
                gManagerDataInfo.intr_used_bit &= ~(1ULL << sensortype);
            }
            xEnableSensorAlgrithm(sensortype, enable);
            break;
        default:
            SM_LOG("VIRTUALSENSOR: xEnableSensorAlgrithm enter!!\n\r");
            for (i = SENSOR_TYPE_ACCELEROMETER; i <= SENSOR_TYPE_MAX_COUNT; ++i) {
                if ((bitmap & (1ULL << i)) != 0) {
                    if (enable) {
                        SM_LOG("enable sensortype :%d\r\n", i);
                    } else {
                        SM_LOG("disable sensortype :%d\r\n", i);
                        gManagerDataInfo.intr_used_bit &= ~(1ULL << sensortype);
                    }
                    xEnableSensorAlgrithm(i, enable);
                }
            }
            break;
    }
}
/* in practice, this is too aggressive, but guaranteed to be enough
*to flush empty the fifo. */
static int xFlushSensorDataToDramWhenDisable(UINT8 sensortype, int enable)
{
    int ret = 0;

    if (enable == SENSOR_DISABLE) {
        switch (sensortype) {
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
            case SENSOR_TYPE_PEDOMETER:
            case SENSOR_TYPE_ACTIVITY:
            case SENSOR_TYPE_PRESSURE:
                SM_ERR("disable sensor(%d), guaranteed to flush fifo\n", sensortype);
                xFlushScpFifo();
                ret = xNotifyFramework(SENSOR_TYPE_MAX_COUNT, BATCH_TIMEOUT_NOTIFY);
                break;
            default:
                break;
        }
    }
    return ret;
}
static void xCheckDelaySensorBitMap(UINT8 sensortype, int delay, int enable, UINT64 bitmap)
{
    int i = 0;
    gAlgorithm[sensortype].delay = delay;
    switch (sensortype) {
        case SENSOR_TYPE_ACCELEROMETER:
        case SENSOR_TYPE_GYROSCOPE:
        case SENSOR_TYPE_PRESSURE:
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
        case SENSOR_TYPE_LIGHT:
        case SENSOR_TYPE_PROXIMITY:
            //SM_ERR("HWSENSOR: setdelay enter %d!!\n\r",sensortype);
            if (bitmap != 0)
                xUpdateRealDelay(sensortype, &Real_Delay[sensortype], enable);
            break;
        default:
            //SM_ERR("VIRTUALSENSOR: setdelay enter!!\n\r");
            for (i = SENSOR_TYPE_ACCELEROMETER; i <= SENSOR_TYPE_MAX_COUNT; ++i) {
                if ((bitmap & (1ULL << i)) != 0) {
                    SM_LOG("setdelay sensortype :%d\r\n", i);
                    xUpdateRealDelay(i, &Real_Delay[i], enable);
                }
            }
            break;
    }
}

static void vGetDirectPushpTimerDelay(void)
{
    int i = 0;
    unsigned int Polling_Delay = 65535;
    for (i = 1; i < SENSOR_TYPE_MAX_COUNT + 1; i++) {

        if (gAlgorithm[i].scp_direct_push_timeout <= 0)
            continue;

        SM_LOG("%d,%d\n\r", Polling_Delay, gAlgorithm[i].scp_direct_push_timeout);
        if (gAlgorithm[i].scp_direct_push_timeout < Polling_Delay) {
            Polling_Delay = gAlgorithm[i].scp_direct_push_timeout;
        }
    }
    Scp_Direct_Push_Polling_Delay = Polling_Delay;
}

static void vStartDirectPushTimerPoling(UINT8 sensortype)
{
    //BaseType_t TimerRet;

    if (gBatchInfo.scp_direct_push_activate & (1ULL << sensortype)) {
        if (Scp_Direct_Push_Polling_Start == false) {
            //trigger timer function
            vGetDirectPushpTimerDelay();
            //TimerRet = xTimerStart(scp_direct_push_timer, Scp_Direct_Push_Polling_Delay);
            //if (TimerRet != pdPASS) {
            //    SM_ERR("Error: xTimerStart failed to start!!\n\r");
            //} else {
            timer_sensor2_change_period(Scp_Direct_Push_Polling_Delay);
            Scp_Direct_Push_Polling_Start = true;
            SM_ERR("polling start Scp_Direct_Push_Polling_Delay:%d\n\r", Scp_Direct_Push_Polling_Delay);
            //}
        } else {
            vGetDirectPushpTimerDelay();
            //TimerRet = xTimerStop(scp_direct_push_timer, tmrNO_DELAY);
            //if (TimerRet != pdPASS) {
            //    SM_ERR("Error: xTimerStop failed!!\n\r");
            //}
            //TimerRet = xTimerChangePeriod(scp_direct_push_timer, Scp_Direct_Push_Polling_Delay, 0);
            //if (TimerRet != pdPASS) {
            //   SM_ERR("Error: xTimerChangePeriod failed to start!!\n\r");
            //}
            timer_sensor2_change_period(Scp_Direct_Push_Polling_Delay);
            SM_ERR("polling ChangePeriod Scp_Direct_Push_Polling_Delay:%d\n\r", Scp_Direct_Push_Polling_Delay);
        }
    }
}

static void vStopDirectPushpTimerPoling(UINT8 sensortype)
{
    //BaseType_t TimerRet;

    if (Scp_Direct_Push_Polling_Start == true) {
        if (!gBatchInfo.scp_direct_push_activate) {
            //cancel timer function
            gAlgorithm[sensortype].scp_direct_push_timeout = -1;
            /*TimerRet = xTimerStop(scp_direct_push_timer, tmrNO_DELAY);//TODO: check polling start is true to do xTimerStop
            if (TimerRet != pdPASS) {
                SM_ERR("Error: xTimerStop failed!!\n\r");
            } else {*/

            timer_sensor2_stop();
            Scp_Direct_Push_Polling_Start = false;
            SM_ERR("polling stop\n\r");
            //}
        } else {
            gAlgorithm[sensortype].scp_direct_push_timeout = -1;
            vGetDirectPushpTimerDelay();
            /*TimerRet = xTimerChangePeriod(scp_direct_push_timer, Scp_Direct_Push_Polling_Delay, 0);
            if (TimerRet != pdPASS) {
                SM_ERR("Error: xTimerStop failed!!\n\r");
            }*/
            timer_sensor2_change_period(Scp_Direct_Push_Polling_Delay);
            SM_ERR("polling ChangePeriod Scp_Direct_Push_Polling_Delay:%d\n\r", Scp_Direct_Push_Polling_Delay);
        }
    }
}

void vSensorManagerEntry(void *pvParameters)
{
    int ret;
    struct SensorManagerQueueEventStruct event;
    bool activate;
    //BaseType_t TimerRet, MSG_RET;
    BaseType_t MSG_RET;
    SM_LOG("INFO: SensorManager Start to run...!!\n\r");
    while (1) {
        MSG_RET = xQueueReceive(gSensorManagerQueuehandle, &event, portMAX_DELAY);
        if (MSG_RET != pdPASS) {
            SM_ERR("Error: xTimerStop failed!!\n\r");
            continue;
        }
        SM_LOG("TIMEOUT1: %lld\n", read_xgpt_stamp_ns());
#ifdef SM_DEBUG
        SM_LOG("INFO: SensorManager receive events..(%d)!!\n\r", event.action);
#endif
        switch (event.action) {
            case SF_ACTIVATE:
                SM_ERR("INFO: SensorManager handle activate command:sensor(%d), activate(%d)!!\n\r", event.info.sensortype,
                       event.info.data[0]);
                xCheckEnableSensorBitMap(event.info.sensortype, event.info.data[0], event.info.bit_map);
                ret = xFlushSensorDataToDramWhenDisable(event.info.sensortype, event.info.data[0]);
                break;
            case SF_SET_DELAY:
                SM_ERR("INFO: SensorManager handle setdelay command:sensor(%d), delay(%d), enable(%d)!!\n\r", event.info.sensortype,
                       event.info.data[0], event.info.data[1]);
                xCheckDelaySensorBitMap(event.info.sensortype, event.info.data[0], event.info.data[1], event.info.bit_map);
                break;
            case SF_DIRECT_PUSH:
                SM_LOG("INFO: SensorManager handle directpush command:sensor(%d), enable(%d)!!\n\r", event.info.sensortype,
                       event.info.data[0]);
                if (event.info.data[0] != 0) {
                    gBatchInfo.scp_direct_push_activate |= (1ULL << event.info.sensortype);
                    vStartDirectPushTimerPoling(event.info.sensortype);
                } else {
                    gBatchInfo.scp_direct_push_activate &= ~(1ULL << event.info.sensortype);
                    vStopDirectPushpTimerPoling(event.info.sensortype);
                }
                break;
            case SF_DIRECT_PUSH_DELAY:
                SM_LOG("INFO: SensorManager handle directpushdelay command:sensor(%d), flag(%d), delay(%d), timeout:(%d)!!\n\r",
                       event.info.sensortype, event.info.data[0],
                       event.info.data[1], event.info.data[2]);
                gAlgorithm[event.info.sensortype].scp_direct_push_timeout = event.info.data[2];
                gAlgorithm[event.info.sensortype].scp_direct_push_counter = event.info.data[2];
                xCheckDelaySensorBitMap(event.info.sensortype, event.info.data[1], event.info.data[3], event.info.bit_map);
                gBatchInfo.scp_direct_push_activate |= (1ULL << event.info.sensortype);
                vStartDirectPushTimerPoling(event.info.sensortype);
                break;
            case SF_BATCH:
                if (event.info.task_handler == 0) {
                    SM_LOG("INFO: SensorManager handle batch command:sensor(%d), flag(%d), delay(%d), timeout:(%d)!!\n\r",
                           event.info.sensortype, event.info.data[0], event.info.data[1], event.info.data[2]);
                    //gAlgorithm[event.info.sensortype].delay = event.info.data[1];
                    gAlgorithm[event.info.sensortype].wakeup_batch_timeout = event.info.data[2];
                    gAlgorithm[event.info.sensortype].wakeup_batch_counter = event.info.data[2];
                    xCheckDelaySensorBitMap(event.info.sensortype, event.info.data[1], event.info.data[3], event.info.bit_map);
                } else {
                    SM_ERR("Error: SM handle sensor batch command failed task_handler err(%d)!\n\r", event.info.task_handler);
                }
                break;
            case SM_BATCH_FLUSH:
                SM_LOG("INFO: SensorManager handle batchflush command:sensor(%d)!!\n\r", event.info.sensortype);
                //if((0 != gBatchInfo.ap_batch_activate) || (0 != gBatchInfo.scp_direct_push_activate))
                xHandleSensorBuffer(); //check flush flag, put data into buffer, if buffer full, flush to DRAM
                continue;
            case SF_SET_CUST:
                if (event.info.task_handler == 0) {
                    SM_LOG("INFO: SensorManager handle SET_CUST command:sensor(%d)!!\n\r", event.info.sensortype);
                    if (gAlgorithm[event.info.sensortype].algo_desp.operate == NULL) {
                        SM_ERR("gAlgorithm[%d].algo_desp.operate == NULL", event.info.sensortype);
                        continue;
                    }
                    ret = gAlgorithm[event.info.sensortype].algo_desp.operate(SETCUST, (void *)event.info.data, sizeof(event.info.data),
                            NULL, 0);
                } else {
                    SM_ERR("Error: SM handle sensor batch command failed task_handler err(%d)!\n\r", event.info.task_handler);
                }
                continue;
            case SF_GET_DATA:
            case SM_INTR:
                //for SCP user immiditlay get data requirments
                if (event.info.task_handler == 0) {
                    SM_LOG("INFO: SensorManager handle get data command:sensor(%d), activate(%d)!!\n\r", event.info.sensortype,
                           event.info.data[0]);
                    ret = xHandleSensorFlushNewest(event.info.sensortype, gAlgorithm[event.info.sensortype].newest);
                    if (ret < 0) {
                        SM_ERR("Error: SM sensor(%d) activate(%d) failed!\n\r", event.info.sensortype, event.info.data[0]);
                    } else {
                        if ((gAlgorithm[event.info.sensortype].enable & (1ULL << event.info.sensortype)) != 0) {
                            gManagerDataInfo.data_ready_bit |= (1ULL << event.info.sensortype);
                            if (gManagerDataInfo.data_ready_bit != 0) {
                                ret = xNotifyFramework(event.info.sensortype, INTR_NOTIFY);
                                if (ret < 0) {
                                    SM_ERR("ERR: SM xNotifyFramework[datachanged] failed\n\r");
                                }
                            }
                        }
                    }
                } else {
                    SM_ERR("Error: SM handle sensor activate command failed task_handler err(%d)!\n\r", event.info.task_handler);
                }
                continue;
            case SM_TIMEOUT:
                xHandleSensorData(); //get each sensor data, depends on activate state.
                break;
            case SF_UPDATE_GESTURE:
                xUpdateGestureMappingTable(event.info.sensortype, event.info.data[0], event.info.data[1]);
                break;
            default:
                SM_ERR("Error: SM command not support:(%d)!\n\r", event.action);
                break;
        }

        //SM_LOG( "SensorManager check polling methord1!!\n\r");
        //reset timer to trigger new timer call back when thread run once
        activate = bCheckSensorState();
        Delay_Changed = xChoosePollingDelay(&Polling_Delay);
        Delay_Changed = true;
        /* if no sensor enable, we adjust lower freq */
        if (activate == false) {
            ret = xNotifyFramework(SENSOR_TYPE_MAX_COUNT, POWER_NOTIFY);
            if (ret < 0)
                SM_ERR("ERR: SM xNotifyFramework[POWER_NOTIFY] failed\n\r");
        }
        //SM_ERR( "check polling, Polling_Delay(%d), delaychanged(%d)!!\n\r", Polling_Delay, Delay_Changed);
        //SM_ERR( "check polling, activate(%d), Polling_Start(%d)!!\n\r", activate, Polling_Start);

        //Polling_Delay = 40;
        g_normal_t2 = read_xgpt_stamp_ns() / 1000000;
        if (activate && !Polling_Start) {
            //trigger timer function
            SM_LOG("SensorManager check polling methord3!!\n\r");
            timer_sensor1_change_period(Polling_Delay);
            Polling_Start = true;
        } else if (!activate && Polling_Start) {
            //cancel timer function
            SM_LOG("SensorManager check polling methord4!!\n\r");
            timer_sensor1_stop();
            Polling_Start = false;
        } else if (activate && Delay_Changed) {
            //trigger timer function
            SM_LOG("SensorManager check polling methord5!!\n\r");
            if ((g_normal_t2 - g_normal_t1) >= Polling_Delay) {
                timer_sensor1_change_period(1);
            } else {
                if (!((Polling_Delay - (g_normal_t2 - g_normal_t1)) > 2000))  {
                    timer_sensor1_change_period(Polling_Delay - (g_normal_t2 - g_normal_t1));
                } else {
                    timer_sensor1_change_period(10);
                }

            }


            Delay_Changed = false;
        } else {
            SM_LOG("SensorManager handle commands no need to trigger timer!!\n\r");
        }

        SM_LOG("TIMEOUT2: %lld\n", timestamp_get_ns());
    }
}

void tSMTimerCallBack(TimerHandle_t xTimer)
{
    struct SensorManagerQueueEventStruct timerevent;
    BaseType_t ret;
    timerevent.action = SM_TIMEOUT;
    g_normal_t1 = read_xgpt_stamp_ns() / 1000000;
    ret = xQueueSendFromISR(gSensorManagerQueuehandle, &timerevent, NULL);
    if (ret != pdPASS) {
        SM_ERR("Error: tSMTimerCallBack timerevent cannot be sent!!\n\r");
    }
}


void tDirectPushTimerCallBack(TimerHandle_t xTimer)
{
    int i = 0;
//    BaseType_t TimerRet;
    struct SensorManagerQueueEventStruct event;
    BaseType_t MSG_RET;

    SM_LOG("batch timeer out\n\r");
    gBatchInfo.scp_direct_push_timeout = 1;
    for (i = 1; i < SENSOR_TYPE_MAX_COUNT + 1; i++) {
        if ((gBatchInfo.scp_direct_push_activate & (1 << i)) != 0) {
            gAlgorithm[i].scp_direct_push_counter =
                gAlgorithm[i].scp_direct_push_counter - Scp_Direct_Push_Polling_Delay;
            if (gAlgorithm[i].scp_direct_push_counter <= 0) {
                SM_LOG("batch[%d] time out\n\r", i);
                event.action = SM_BATCH_FLUSH;
                event.info.sensortype = SENSOR_TYPE_MAX_COUNT;
                event.info.task_handler = 0;
                MSG_RET = xQueueSendToFrontFromISR(gSensorManagerQueuehandle, &event, NULL);
                if (MSG_RET != pdPASS) {
                    SM_ERR("Error: xQueueSendToFrontFromISR failed!!\n\r");
                }
                gAlgorithm[i].scp_direct_push_counter = gAlgorithm[i].scp_direct_push_timeout;
            }
        }

    }

    SM_LOG("reset timer to %d \n\r", Scp_Direct_Push_Polling_Delay);
    /*TimerRet = xTimerChangePeriod(scp_direct_push_timer, Scp_Direct_Push_Polling_Delay / portTICK_PERIOD_MS, 0);
    if (TimerRet != pdPASS) {
        SM_ERR("Error: batch TimerStart failed to start!!\n\r");
    }*/
    timer_sensor2_change_period(Scp_Direct_Push_Polling_Delay);
}
void vSensorDriverInitTaskEntry(void *pvParameters)
{
    while (1) {
        if (init_done == 0) {
            SM_LOG("INFO: vSensorDriverInitTaskEntry entry!!\n\r");
            module_init(MOD_PHY_SENSOR);
            module_init(MOD_VIRT_SENSOR);
            SM_LOG("INFO: vSensorDriverInitTaskEntry entry2!!\n\r");
            init_done = 1;
            xSemaphoreGive(xSMINITSemaphore);
        }
        vTaskSuspend(NULL);
    }
}

int xSensorManagerInit(void)
{
    int i;
    BaseType_t MSG_RET;
    int ret = 0;

    SM_LOG("INFO: xSensorManagerInit entry1!!\n\r");

    gBatchFifo = (struct sensorFIFO *)
                 &BatchFifo;//(struct sensorFIFO *)malloc(SCP_SENSOR_BATCH_FIFO_BATCH_SIZE+sizeof(struct sensorFIFO));
    gBatchFifo->FIFOSize = SCP_SENSOR_BATCH_FIFO_BATCH_SIZE;
    gBatchFifo->rp = gBatchFifo->wp = 0;

    SM_LOG("sensor_manager_init, gBatchFifo->rp = %p, gBatchFifo->wp = %p\n\r", gBatchFifo->rp, gBatchFifo->wp);

    memset(&gManagerDataInfo, 0, sizeof(struct data_info_descriptor_t));

    for (i = 1; i < SENSOR_TYPE_MAX_COUNT + 1; i++) {
        Real_Delay[i] = 200;
        gAlgorithm[i].delay = 200;
        gAlgorithm[i].wakeup_batch_timeout = -1;
        gAlgorithm[i].scp_direct_push_timeout = -1;
        gAlgorithm[i].enable = 0LL;
        xSMSemaphore[i] = xSemaphoreCreateCounting(1, 1);
        if (xSMSemaphore[i] == NULL) {
            SM_ERR("Error: SM created xSMSemaphore fail!!\n\r");
            return SM_ERROR;
        }
    }
    xSMINITSemaphore = xSemaphoreCreateBinary();
    if (xSMINITSemaphore == NULL) {
        SM_ERR("xSemaphoreCreateBinary(xSMINITSemaphore) fail\n");
        return SM_ERROR;
    }

    gSensorManagerQueuehandle = xQueueCreate(SENSOR_MANAGER_QUEUE_LENGTH, SENSOR_MANAGER_ITEM_SIZE);

    SM_LOG("INFO: xSensorManagerInit entry2!!\n\r");

    MSG_RET = xTaskCreate(vSensorDriverInitTaskEntry, "SI", 256, (void *) NULL, 2, NULL);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: SI task cannot be created!!\n\r");
        return ret;
    }

    SM_LOG("INFO: xSensorManagerInit entry3!!\n\r");

    // SMTimer = xTimerCreate("SMTimer", 200, pdTRUE, (void * )10, tSMTimerCallBack);
    /*SMTimer = xTimerCreate("SMTimer", 200, pdFALSE, (void *)10, tSMTimerCallBack);
    if (SMTimer == NULL) {
        SM_ERR("Error: SM timer cannot be created!!\n\r");
        return SM_ERROR;
    }*/

    platform_set_periodic_timer_sensor1(tSMTimerCallBack, NULL, 65535);
    /*scp_direct_push_timer = xTimerCreate("g_batch_Timer", 200, pdFALSE, (void *)10, tDirectPushTimerCallBack);
    if (scp_direct_push_timer == NULL) {
        SM_ERR("Error: g_batch_Timer timer cannot be created!!\n\r");
        return SM_ERROR;
    }*/
    platform_set_periodic_timer_sensor2(tDirectPushTimerCallBack, NULL, 65535);
    SM_LOG("INFO: xSensorManagerInit entry4!!\n\r");

    MSG_RET = xTaskCreate(vSensorManagerEntry, "SM", 2048, (void *) NULL, 3, NULL);
    if (MSG_RET != pdPASS) {
        SM_ERR("Error: SM task cannot be created!!\n\r");
        return ret;
    }

    SM_LOG("INFO: xSensorManagerInit entry5!!\n\r");
    return ret;
}
