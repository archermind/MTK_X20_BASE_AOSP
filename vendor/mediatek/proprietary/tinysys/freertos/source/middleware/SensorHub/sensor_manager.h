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
#ifndef __SENSOR_MANAGER_H__
#define __SENSOR_MANAGER_H__


#include "typedefs.h"
#include "sensors.h"
#include "scp_sem.h"
#include "mt_reg_base.h"
#include "dma.h"
#include "module.h"
#include <vcore_dvfs.h>
#include <mt_gpt.h>
/* timer related definations*/
#define tmrNO_DELAY     ( TickType_t ) 0U

#define SENSOR_DISABLE 0
#define SENSOR_ENABLE 1

#define MAX_GYROSCOPE_FIFO_SIZE         10
#define GYROSCOPE_FIFO_THREASHOLD       6
#define MAX_ACCELERATION_FIFO_SIZE      10
#define ACCELERATION_FIFO_THREASHOLD    6
#define FIFO_ENABLE 1
#define FIFO_DISABLE 0
#define USE_IN_FACTORY 1
#define NOT_USE_IN_FACTORY 0

#define ACC_DELAY_PER_FIFO_LOOP 60
#define ACC_EVENT_COUNT_PER_FIFO_LOOP 3
#define GYRO_DELAY_PER_FIFO_LOOP 20
#define GYRO_EVENT_COUNT_PER_FIFO_LOOP 4

#define ANDROID_SENSOR_GAME_SPEED 20
#define ANDROID_SENSOR_UI_SPEED 66
#define ANDROID_SENSOR_NORMAL_SPEED 200

#define SCP_SENSOR_BATCH_FIFO_BATCH_SIZE    (20480)
#define SCP_SENSOR_BATCH_FIFO_THRESHOLD     (512)
//#define DRAM_SENSOR_BATCH_FIFO_THRESHOLD    (SCP_SENSOR_BATCH_FIFO_BATCH_SIZE)
#define TIME_DEVIATION                      5
enum report_mode {
    on_change = 1,
    one_shot = 2,
    continus = 4
};

typedef enum {
    ACTIVATE = 1,
    SETDELAY = 2,
    SETCUST = 3,
    ENABLEFIFO = 4
} Sensor_Command;

struct sensor_ability_t {
    UINT32 max_sampling_rate; //Hz
    UINT32 support_HW_FIFO; //0:NOT support, otherwise FIFO size
};

struct input_list_t {
    UINT8 input_type;/*char* sensor name.*/
    UINT8 count;
    UINT8 reserved[2];
    INT32 sampling_delay; //input data feed faster than this, if sampling < 0, same as parent
    struct input_list_t *next_input;
};

struct output_list_t {
    UINT8 output_type;/*char* sensor name.*/
    UINT8 count;
    UINT8 reserved[2];
    INT32 sampling_delay; //input data feed faster than this, if sampling < 0, same as parent
    struct output_list_t *next_output;
};

struct SensorDescriptor_t {
    UINT8 sensor_type; /*char* sensor name.*/
    int version;
    int report_mode; /*one-shot&on-change&continus, bitwise code, set from enum report_mode */
    struct sensor_ability_t hw; /*for sensor to register ability*/
    struct input_list_t *input_list; /*all children*/
    int (*operate)(Sensor_Command command, void* buffer_in, int size_in, void* buffer_out,
                   int size_out);/*sensor related HW operations*/
    int (*run_algorithm)(struct data_t* output);/*run register algorithm to generate sensor data*/
    int (*set_data)(const struct data_t *input_list, void *reserve); /*async input/output*/

    /*
      the ability of algorithm to handle a batch of data
      can save more power when physical sensor support HW FIFO
      ex: althought step count need 50 HZ input accumulate data
          but step count only need update at 4Hz(human step count max frequency)
          this accumulate can set 250.
     */
    UINT32 accumulate; //ms.
};

/*
    for sensor manager to maintain global variable, index 0 is reserved and not used,
     there will be SENSOR_TYPE_MAX_COUNT numbers of this struct in sensor manager
*/
struct algorithm_descriptor_t {
    /*
     single sensor related
     */
    struct SensorDescriptor_t algo_desp;
    /*
     sensor type: i
     enable[i] means enable/disable
     */
    UINT64 enable; //bitwise, save which ancestor need this algorithm enable,
    /*
        current delay value of this sensor running
        current piroid of batch if enabled
    */
    UINT32 delay; //ap/PDCA/FLP set delay
    int wakeup_batch_timeout;
    int scp_direct_push_timeout;
    /*
    excuted, check whether this sensor has been performed in this loop
    */
    bool excuted;

    UINT8 last_accuracy;
    struct data_t *newest; //maintain latest input data for all algorithm
    struct output_list_t *next_output; //ancestor, for related sensor delay modification
    int wakeup_batch_counter;
    int scp_direct_push_counter;
    int exist_data_count;
};

struct batch_info_descriptor_t {
    UINT64 ap_batch_activate; //bit map parmeter for SCP sensors batch enable info
    UINT64 scp_direct_push_activate; //bit map parmeter for SCP sensors batch enable info
    UINT64 really_batch_flag; //bit map parmeter for SCP sensors AP timeout notify info
    UINT64 wake_up_on_fifo_full;
    UINT32 ap_batch_timeout;
    UINT32 wakeup_batch_timeout;
    UINT32 scp_direct_push_timeout;
};

struct data_info_descriptor_t {
    UINT64 last_report_time[SENSOR_TYPE_MAX_COUNT + 1];
    UINT64 last_real_time[SENSOR_TYPE_MAX_COUNT + 1];
    UINT64 data_ready_bit;
    UINT64 accuracy_change_bit;
    INT64 timestamp_offset_to_ap;
    UINT64 intr_used_bit;//for sensor polling rate < 10ms enable interrupt mode
    struct sensorFIFO *bufferBase; //AP DRAM Buffer Start Address
    UINT32 bufferSize;//AP DRAM Buffer Size
};

int sensor_subsys_algorithm_register_type(struct SensorDescriptor_t *desp);
int sensor_subsys_algorithm_register_data_buffer(UINT8 sensor_type, int exist_data_count);
int sensor_subsys_algorithm_notify(UINT8 sensor_type);

extern struct algorithm_descriptor_t gAlgorithm[SENSOR_TYPE_MAX_COUNT + 1];
extern struct batch_info_descriptor_t gBatchInfo;
extern struct data_info_descriptor_t gManagerDataInfo;
extern SemaphoreHandle_t xSMSemaphore[SENSOR_TYPE_MAX_COUNT + 1];
extern SemaphoreHandle_t xSMINITSemaphore;

extern UINT8 xSensorTypeMappingToAP(UINT8 sensortype);
extern UINT32 Real_Delay[SENSOR_TYPE_MAX_COUNT + 1];
extern int xUpdateRealDelay(UINT8 sensor_type, UINT32 *this_sensor_delay, int enable);
#endif
