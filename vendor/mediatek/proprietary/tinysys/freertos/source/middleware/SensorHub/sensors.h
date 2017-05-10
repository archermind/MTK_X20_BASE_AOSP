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
#ifndef __SENSORS__H__
#define __SENSORS__H__

#include "typedefs.h"
#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include "timers.h"
#include "utils.h"
#include "scp_ipi.h"
#include "semphr.h"
#include <tinysys_config.h>

/*------------------------------------------------------------*/
#define SENSOR_TYPE_ACCELEROMETER           1
#define SENSOR_TYPE_MAGNETIC_FIELD          2
#define SENSOR_TYPE_ORIENTATION             3 /*Add kernel driver*/
#define SENSOR_TYPE_GYROSCOPE               4
#define SENSOR_TYPE_LIGHT               5
#define SENSOR_TYPE_PRESSURE                6
#define SENSOR_TYPE_TEMPERATURE             7
#define SENSOR_TYPE_PROXIMITY               8
#define SENSOR_TYPE_GRAVITY             9 /*Add kernel driver*/
#define SENSOR_TYPE_LINEAR_ACCELERATION         10 /*Add kernel driver*/
#define SENSOR_TYPE_ROTATION_VECTOR         11 /*Add kernel driver*/
#define SENSOR_TYPE_RELATIVE_HUMIDITY           12 /*Add kernel driver*/
#define SENSOR_TYPE_AMBIENT_TEMPERATURE         13 /*Add kernel driver*/
#define SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED     14 /*Add kernel driver*/
#define SENSOR_TYPE_GAME_ROTATION_VECTOR        15 /*Add kernel driver*/
#define SENSOR_TYPE_GYROSCOPE_UNCALIBRATED      16 /*Add kernel driver*/
#define SENSOR_TYPE_SIGNIFICANT_MOTION          17
#define SENSOR_TYPE_STEP_DETECTOR           18
#define SENSOR_TYPE_STEP_COUNTER            19
#define SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR     20 /*Add kernel driver*/
#define SENSOR_TYPE_HEART_RATE              21
#define SENSOR_TYPE_TILT_DETECTOR           22
#define SENSOR_TYPE_WAKE_GESTURE            23 /* these three type can be confirguratured */
#define SENSOR_TYPE_GLANCE_GESTURE          24 /* these three type can be confirguratured */
#define SENSOR_TYPE_PICK_UP_GESTURE         25 /* these three type can be confirguratured */
#define SENSOR_TYPE_WRIST_TITL_GESTURE      26 /* these three type can be confirguratured */
#define SENSOR_TYPE_PEDOMETER               35
#define SENSOR_TYPE_INPOCKET                36
#define SENSOR_TYPE_ACTIVITY                37
#define SENSOR_TYPE_PDR                 38 /*Add kernel driver*/
#define SENSOR_TYPE_FREEFALL                39
#define SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED      40 /*Add kernel driver*/
/*******************************RESEVERED SENSOR FOR EXPANDING************************************/
#define SENSOR_TYPE_RESERVED1               41
#define SENSOR_TYPE_RESERVED2               42
#define SENSOR_TYPE_RESERVED3               43
#define SENSOR_TYPE_ANSWER_CALL_GESTURE             44
#define SENSOR_TYPE_STATIONARY_GESTURE      45
/*------------sensors will only used on SCP------------*/ /* these sensor will not appered on AP side sensor list*/
#define SENSOR_TYPE_GPS                 46
#define SENSOR_TYPE_MODEM_1             47
#define SENSOR_TYPE_TAP                 48
#define SENSOR_TYPE_TWIST               49
#define SENSOR_TYPE_FLIP                50
#define SENSOR_TYPE_SNAPSHOT                51
#define SENSOR_TYPE_PICK_UP             52
#define SENSOR_TYPE_SHAKE               53
#define SENSOR_TYPE_ANSWER_CALL         54
#define SENSOR_TYPE_STATIONARY          55
#define SENSOR_TYPE_INPK                56
#define SENSOR_TYPE_MODEM_3             57
#define SENSOR_TYPE_ALL                    (58)
#define SENSOR_TYPE_MAX_COUNT              (58)

#define INVALID_DELAY 0xFFFF
#define DEFAULT_DELAY 200/*unit mirco second*/

#define SENSOR_INVALID_VALUE -1
#define SCP_SENSOR_HUB_TEMP_BUFSIZE     256
#define SCP_SENSOR_HUB_FIFO_SIZE        32768

/*
 * flags for (*batch)()
 * Availability: SENSORS_DEVICE_API_VERSION_1_0
 * see (*batch)() documentation for details
 */
enum {
    SENSORS_BATCH_DRY_RUN               = 0x00000001,
    SENSORS_BATCH_WAKE_UPON_FIFO_FULL   = 0x00000002
};

/*+++++++++++++++++++++++sensor data structure defination+++++++++++++++++++++++*/
typedef struct {
    union {
        INT32 v[3];
        struct {
            INT32 x;
            INT32 y;
            INT32 z;
        };
        struct {
            INT32 azimuth;
            INT32 pitch;
            INT32 roll;
        };
    };
    union {
        INT32 scalar;
        union {
            INT16 bias[3];
            struct {
                INT16 x_bias;
                INT16 y_bias;
                INT16 z_bias;
            };
        };
    };
    UINT16 status;
} sensor_vec_t;

typedef struct {
    union {
        INT32 uncali[3];
        struct {
            INT32 x_uncali;
            INT32 y_uncali;
            INT32 z_uncali;
        };
    };
    union {
        INT32 bias[3];
        struct {
            INT32 x_bias;
            INT32 y_bias;
            INT32 z_bias;
        };
    };
} uncalibrated_event_t;

typedef struct {
    INT32 bpm;
    INT32 status;
} heart_rate_event_t;

typedef enum {
    STILL,
    STANDING,
    SITTING,
    LYING,
    ON_FOOT,
    WALKING,
    RUNNING,
    CLIMBING,
    ON_BICYCLE,
    IN_VEHICLE,
    TILTING,
    UNKNOWN,
    ACTIVITY_MAX
} activity_type_t;

typedef struct {
    uint8_t probability[ACTIVITY_MAX];    // 0~100
} activity_t;

typedef enum {
    GESTURE_NONE,
    SHAKE,
    TAP,
    TWIST,
    FLIP,
    SNAPSHOT,
    ANSWERCALL,
    PICKUP,
    GESTURE_MAX
} gesture_type_t;

typedef struct {
    int32_t probability;
} gesture_t;

typedef struct {
//200~300 bytes per second.(per timestamp)
//malloc by driver
    double lla_in[3];/*input latitude, longitude, and altitude in radian*/
    float vned_in[3];/*input velocities in NED frame [vn, ve, vd] in meter/s */
    float lla_in_acc[3];/*position accuracies in meter*/
    float vned_in_acc[3]; /*velocity accuracies meter/s*/
    INT32 gps_sec;/*GPS second*/
    INT32 leap_sec;/*current leap second*/
} gps_vec_t;

#ifdef CFG_FLP_SUPPORT
typedef struct {
    UINT32 length;
} MTK_MDM_HEADER_T;

typedef struct {
    UINT16 MDM_TYPE;
    UINT16 MCC;
    UINT16 NET;
    UINT16 AREA;
    UINT16 OTHER;
    UINT16 SIGNAL_STRENGTH;
    UINT32 CELL_ID;
    UINT32 isCamping;
} MTK_MDM_DATA_T;

typedef struct {
    MTK_MDM_HEADER_T header;;
    MTK_MDM_DATA_T data[10];
} modem_vec_t;
#endif

typedef struct {
    UINT32 accumulated_step_count;
    UINT32 accumulated_step_length;
    UINT32 step_frequency;
    UINT32 step_length;
} pedometer_event_t;

typedef struct {
    UINT32 steps;
    INT32 oneshot;
} proximity_vec_t;

typedef struct {
    INT32 pressure; // Pa, i.e. hPa * 100
    INT32 temperature;
    UINT32 status;
} pressure_vec_t;

typedef struct {
    INT32 relative_humidity;
    INT32 temperature;
    UINT32 status;
} relative_humidity_vec_t;


typedef struct {
    INT32 state; // sleep, restless, awake
} sleepmonitor_event_t;

typedef enum {
    FALL_NONE,
    FALL,
    FLOP,
    FALL_MAX
} fall_type;

typedef struct {
    UINT8 probability[FALL_MAX];  // 0~100
} fall_t;

typedef struct {
    INT32 state;//0,1
} tilt_event_t;

typedef struct {
    INT32 state;//0,1
} in_pocket_event_t;

typedef struct {
    INT32 state;
} significant_motion_event_t;

typedef struct {
    UINT32 accumulated_step_count;
} step_counter_event_t;

typedef struct {
    UINT32 step_detect;
} step_detector_event_t;

struct data_unit_t {
    UINT8  sensor_type;
    UINT8  reserve[3];
    UINT64 time_stamp; //ms on CM4 time kick
    UINT64 time_stamp_gpt;//ms for sensor GPT AP SCP sync time
    union {
        union {
            sensor_vec_t accelerometer_t;
            sensor_vec_t gyroscope_t;
            sensor_vec_t magnetic_t;
            sensor_vec_t orientation_t;
            sensor_vec_t pdr_event;

            INT32 light;
            proximity_vec_t proximity_t;
            INT32 temperature;
            pressure_vec_t pressure_t;
            relative_humidity_vec_t relative_humidity_t;

            sensor_vec_t uncalibrated_acc_t;
            sensor_vec_t uncalibrated_mag_t;
            sensor_vec_t uncalibrated_gyro_t;

            pedometer_event_t pedometer_t;

            gps_vec_t gps_t;
#ifdef CFG_FLP_SUPPORT
            modem_vec_t modem_t;
#endif
            heart_rate_event_t heart_rate_t;

            activity_t activity_data_t;
            gesture_t gesture_data_t;
            fall_t fall_data_t;
            significant_motion_event_t smd_t;
            step_detector_event_t step_detector_t;
            step_counter_event_t step_counter_t;
            tilt_event_t tilt_event;
            in_pocket_event_t inpocket_event;
            INT32 value[16];
        };
        union {
            UINT64 data[8];
            UINT64 step_counter;
        };
    };
};

struct data_t {
    struct data_unit_t *data; //data fifo pointer
    UINT32 fifo_max_size;
    UINT32 data_exist_count;
    struct data_t *next_data;
};

struct sensorFIFO {
    UINT32 rp;
    UINT32 wp;
    UINT32                FIFOSize;
    UINT32                reserve;
    struct data_unit_t   data[0];
};

/*------------------------------IPI commands Related Structures-----------------------------*/

/* SCP_NOTIFY EVENT */
#define    SCP_INIT_DONE            0
#define    SCP_FIFO_FULL            1
#define    SCP_NOTIFY               2
#define    SCP_BATCH_TIMEOUT        3
#define    SCP_DIRECT_PUSH          4


/* SCP_ACTION */
#define    SENSOR_HUB_ACTIVATE      0
#define    SENSOR_HUB_SET_DELAY     1
#define    SENSOR_HUB_GET_DATA      2
#define    SENSOR_HUB_BATCH         3
#define    SENSOR_HUB_SET_CONFIG    4
#define    SENSOR_HUB_SET_CUST      5
#define    SENSOR_HUB_NOTIFY        6
#define    SENSOR_HUB_BATCH_TIMEOUT 7
#define    SENSOR_HUB_SET_TIMESTAMP 8
#define    SENSOR_HUB_POWER_NOTIFY  9


typedef struct {
    UINT8    sensorType;
    UINT8    action;
    UINT8    event;
    UINT8    reserve;
    UINT32   data[11];
} SCP_SENSOR_HUB_REQ;

typedef struct {
    UINT8    sensorType;
    UINT8    action;
    INT8     errCode;
    UINT8    reserve[1];
} SCP_SENSOR_HUB_RSP;

typedef struct {
    UINT8    sensorType;
    UINT8    action;
    UINT8    reserve[2];
    UINT32   enable;  //0 : disable ; 1 : enable
} SCP_SENSOR_HUB_ACTIVATE_REQ;

typedef SCP_SENSOR_HUB_RSP SCP_SENSOR_HUB_ACTIVATE_RSP;

typedef struct {
    UINT8    sensorType;
    UINT8    action;
    UINT8    reserve[2];
    UINT32    delay;  //ms
} SCP_SENSOR_HUB_SET_DELAY_REQ;

typedef SCP_SENSOR_HUB_RSP SCP_SENSOR_HUB_SET_DELAY_RSP;

typedef struct {
    UINT8    sensorType;
    UINT8    action;
    UINT8    reserve[2];
} SCP_SENSOR_HUB_GET_DATA_REQ;

typedef struct {
    UINT8    sensorType;
    UINT8    action;
    INT8     errCode;
    UINT8    reserve[1];
    union {
        INT8      int8_Data[0];
        INT16     int16_Data[0];
        INT32     int32_Data[0];
    } data;
} SCP_SENSOR_HUB_GET_DATA_RSP;

typedef struct {
    UINT8    sensorType;
    UINT8    action;
    UINT8    flag;           //see SENSORS_BATCH_WAKE_UPON_FIFO_FULL definition in hardware/libhardware/include/hardware/sensors.h
    UINT8    reserve[1];
    UINT32    period_ms;      //batch reporting time in ms
    UINT32    timeout_ms;     //sampling time in ms
} SCP_SENSOR_HUB_BATCH_REQ;

typedef SCP_SENSOR_HUB_RSP SCP_SENSOR_HUB_BATCH_RSP;

typedef struct {
    UINT8            sensorType;
    UINT8            action;
    UINT8            reserve[2];
    struct sensorFIFO   *bufferBase;
    UINT32            bufferSize;
    INT64             ap_timestamp;
} SCP_SENSOR_HUB_SET_CONFIG_REQ;

typedef SCP_SENSOR_HUB_RSP SCP_SENSOR_HUB_SET_CONFIG_RSP;
typedef enum {
    CUST_ACTION_SET_CUST = 1,
    CUST_ACTION_SET_CALI,
    CUST_ACTION_RESET_CALI,
    CUST_ACTION_SET_TRACE,
    CUST_ACTION_SET_DIRECTION,
    CUST_ACTION_SHOW_REG,
    CUST_ACTION_GET_RAW_DATA,
    CUST_ACTION_SET_PS_THRESHOLD,
    CUST_ACTION_SHOW_ALSLV,
    CUST_ACTION_SHOW_ALSVAL,
    CUST_ACTION_SET_FACTORY,
} CUST_ACTION;

typedef struct {
    CUST_ACTION    action;
} SCP_SENSOR_HUB_CUST;

typedef struct {
    CUST_ACTION    action;
    INT32     data[0];
} SCP_SENSOR_HUB_SET_CUST;

typedef struct {
    CUST_ACTION    action;
    int trace;
} SCP_SENSOR_HUB_SET_TRACE;

typedef struct {
    CUST_ACTION    action;
    int         direction;
} SCP_SENSOR_HUB_SET_DIRECTION;

typedef struct {
    CUST_ACTION    action;
    unsigned int    factory;
} SCP_SENSOR_HUB_SET_FACTORY;

typedef struct {
    CUST_ACTION    action;
    union {
        INT8        int8_data[0];
        UINT8       uint8_data[0];
        INT16       int16_data[0];
        UINT16      uint16_data[0];
        INT32       int32_data[0];
        UINT32      uint32_data[3];
    };
} SCP_SENSOR_HUB_SET_CALI;

typedef SCP_SENSOR_HUB_CUST SCP_SENSOR_HUB_RESET_CALI;
typedef struct {
    CUST_ACTION    action;
    INT32     threshold[2];
} SCP_SENSOR_HUB_SETPS_THRESHOLD;

typedef SCP_SENSOR_HUB_CUST SCP_SENSOR_HUB_SHOW_REG;
typedef SCP_SENSOR_HUB_CUST SCP_SENSOR_HUB_SHOW_ALSLV;
typedef SCP_SENSOR_HUB_CUST SCP_SENSOR_HUB_SHOW_ALSVAL;

typedef struct {
    CUST_ACTION    action;
    union {
        INT8        int8_data[0];
        UINT8       uint8_data[0];
        INT16       int16_data[0];
        UINT16      uint16_data[0];
        INT32       int32_data[0];
        UINT32      uint32_data[3];
    };
} SCP_SENSOR_HUB_GET_RAW_DATA;
typedef struct {
    union {
        SCP_SENSOR_HUB_CUST             cust;
        SCP_SENSOR_HUB_SET_CUST         setCust;
        SCP_SENSOR_HUB_SET_CALI         setCali;
        SCP_SENSOR_HUB_RESET_CALI       resetCali;
        SCP_SENSOR_HUB_SET_TRACE        setTrace;
        SCP_SENSOR_HUB_SET_DIRECTION    setDirection;
        SCP_SENSOR_HUB_SHOW_REG         showReg;
        SCP_SENSOR_HUB_GET_RAW_DATA     getRawData;
        SCP_SENSOR_HUB_SETPS_THRESHOLD  setPSThreshold;
        SCP_SENSOR_HUB_SHOW_ALSLV       showAlslv;
        SCP_SENSOR_HUB_SHOW_ALSVAL      showAlsval;
        SCP_SENSOR_HUB_SET_FACTORY      setFactory;
    };
} CUST_SET_REQ, *CUST_SET_REQ_P;
typedef struct {
    UINT8    sensorType;
    UINT8    action;
    UINT8    reserve[2];
    union {
        // notify: align to SensorManagerActionInfo!!!!!!!!!!, when xSensorFrameworkSetCust, we memcpy 8*4 bytes
        UINT32          custData[8];
        CUST_SET_REQ    cust_set_req;
    };
} SCP_SENSOR_HUB_SET_CUST_REQ, *SCP_SENSOR_HUB_SET_CUST_REQ_P;

typedef struct {
    UINT8    sensorType;
    UINT8    action;
    INT8     errCode;
    UINT8    reserve[1];
    union {
        UINT32                          custData[11];
        SCP_SENSOR_HUB_GET_RAW_DATA     getRawData;
    };
} SCP_SENSOR_HUB_SET_CUST_RSP;

typedef struct {
    UINT8            sensorType;
    UINT8            action;
    UINT8            event;
    UINT8            reserve[1];
    union {
        INT8      int8_Data[0];
        INT16     int16_Data[0];
        INT32     int32_Data[0];
    } data;
} SCP_SENSOR_HUB_NOTIFY_RSP;

typedef union {
    SCP_SENSOR_HUB_REQ req;
    SCP_SENSOR_HUB_RSP rsp;
    SCP_SENSOR_HUB_ACTIVATE_REQ activate_req;
    SCP_SENSOR_HUB_ACTIVATE_RSP activate_rsp;
    SCP_SENSOR_HUB_SET_DELAY_REQ set_delay_req;
    SCP_SENSOR_HUB_SET_DELAY_RSP set_delay_rsp;
    SCP_SENSOR_HUB_GET_DATA_REQ get_data_req;
    SCP_SENSOR_HUB_GET_DATA_RSP get_data_rsp;
    SCP_SENSOR_HUB_BATCH_REQ batch_req;
    SCP_SENSOR_HUB_BATCH_RSP batch_rsp;
    SCP_SENSOR_HUB_SET_CONFIG_REQ set_config_req;
    SCP_SENSOR_HUB_SET_CONFIG_RSP set_config_rsp;
    SCP_SENSOR_HUB_SET_CUST_REQ set_cust_req;
    SCP_SENSOR_HUB_SET_CUST_RSP set_cust_rsp;
    SCP_SENSOR_HUB_NOTIFY_RSP notify_rsp;
} SCP_SENSOR_HUB_DATA, *SCP_SENSOR_HUB_DATA_P;
/*----------------------------------------------------------------------------------*/
#define GYROSCOPE_INCREASE_NUM_AP       131
#define GYROSCOPE_INCREASE_NUM_SCP      1000

#endif
