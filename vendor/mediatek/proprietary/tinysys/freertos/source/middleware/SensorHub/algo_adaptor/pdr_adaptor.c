/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */
#ifndef __PDR_ADAPTOR_C_INCLUDED__
#define __PDR_ADAPTOR_C_INCLUDED__

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "mpe_cm4_API.h"
#include "algo_adaptor.h"
#include "sensor_manager.h"

#define PDR_LOG
#ifdef PDR_LOG
#define PLOG(fmt, args...)    PRINTF_D("[PDR]: "fmt, ##args)
#else
#define PLOG(fmt, args...)
#endif

#define ONE_MS_INTERVAL      1000000   //1ms
//#define ACC_DATA_INTERVAL  10000000 //10ms, 10000000 ns --> no fifo (100Hz baud rate)
#define ACC_DATA_INTERVAL 60       //40ms,  40000000 ns --> with acc fifo (25Hz baud rate) (low power test)
#define GYRO_DATA_INTERVAL 20000000 //20ms, 20000000ns -->fifo 4 data (200Hz baud rate)
#define MAG_DATA_INTERVAL  20000000 //20ms, 20000000ns --> no fifo (50Hz baud rate)

mtk_uint32 Algo_reg_union = 0;
mtk_uint32 Algo_reg_group = 0;
uint64_t ttick_acc_first = 0;
uint64_t ttick_gyro_first = 0;
uint64_t ttick_mag_first = 0;
uint64_t ttick_baro_first = 0;
uint64_t ttick_acc_last = 0;
uint64_t ttick_gyro_last = 0;
uint64_t ttick_mag_last = 0;
uint64_t ttick_baro_last = 0;
MPE_SENSOR_COMMON acceleration;
MPE_SENSOR_COMMON gyro;
MPE_SENSOR_COMMON mag;
MPE_SENSOR_COMMON baro;
UINT32 timestamp_gyro;
UINT32 adr_timestamp;
gps_vec_t gps_raw;
MPE_ADR_RESULT adr_result;
mtk_int32 adr_flag;

/**************************************************************************/
/* Virtual sensor set data */
/**************************************************************************/
int PDR_init()
{
    mpe_init();
    memset(&acceleration, 0, sizeof(MPE_SENSOR_COMMON));
    memset(&gyro, 0, sizeof(MPE_SENSOR_COMMON));
    memset(&mag, 0, sizeof(MPE_SENSOR_COMMON));
    memset(&baro, 0, sizeof(MPE_SENSOR_COMMON));
    memset(&gps_raw, 0, sizeof(gps_vec_t));
    adr_flag = -1;
    return 0;
}

static int set_PDR_data(const struct data_t *input_list, void *reserve)
{
    int i;
    struct data_unit_t *data_start = input_list->data;
    //MPE_SENSOR_COMMON local_mag,
    MPE_SENSOR_COMMON local_gyro;

    if ((input_list == NULL) || (input_list->data_exist_count < 1)) {
        PLOG("input list invalid");
        return -1;
    }

    //PLOG("algo union :%d, group:%d \n",Algo_reg_union,Algo_reg_group);
    //extract data
    for (i = 0; i < (input_list->data_exist_count); i++) {
        if (data_start->sensor_type == SENSOR_TYPE_ACCELEROMETER) {
            if (ttick_acc_last == 0) { //first acc to be recorded
                ttick_acc_first = data_start->time_stamp;
                ttick_acc_last = ttick_acc_first;
                acceleration.x = data_start->accelerometer_t.x;
                acceleration.y = data_start->accelerometer_t.y;
                acceleration.z = data_start->accelerometer_t.z;
                acceleration.timestamp = data_start->time_stamp;
                acceleration.accuracy = data_start->accelerometer_t.status;
            } else {
                ttick_acc_first = data_start->time_stamp;
                if ((ttick_acc_first <= ttick_acc_last)) { //redundancy input
                    return 0;
                } else {
                    ttick_acc_last = ttick_acc_first;
                    acceleration.x = data_start->accelerometer_t.x;
                    acceleration.y = data_start->accelerometer_t.y;
                    acceleration.z = data_start->accelerometer_t.z;
                    acceleration.timestamp = data_start->time_stamp;
                    acceleration.accuracy = data_start->accelerometer_t.status;
                }
            }
            if (Algo_reg_group == REG_AM_ALGO) {
            } else {
                continue;
            }
            //PLOG("dbg am sensor\n");
        } else if (data_start->sensor_type == SENSOR_TYPE_GYROSCOPE) {
            if (ttick_gyro_last == 0) { //first gyro to be recorded
                ttick_gyro_first = data_start->time_stamp;
                ttick_gyro_last = ttick_gyro_first;
                gyro.x = data_start->gyroscope_t.x;
                gyro.y = data_start->gyroscope_t.y;
                gyro.z = data_start->gyroscope_t.z;
                gyro.accuracy = data_start->gyroscope_t.status;
                gyro.timestamp = data_start->time_stamp;
            } else {
                ttick_gyro_first = data_start->time_stamp;
                if ((ttick_gyro_first <= ttick_gyro_last)) {//redundancy input
                    return 0;
                } else {
                    ttick_gyro_last = ttick_gyro_first;
                    gyro.x = data_start->gyroscope_t.x;
                    gyro.y = data_start->gyroscope_t.y;
                    gyro.z = data_start->gyroscope_t.z;
                    gyro.accuracy = data_start->gyroscope_t.status;
                    gyro.timestamp = data_start->time_stamp;
                }
            }
        } else if (data_start->sensor_type == SENSOR_TYPE_MAGNETIC_FIELD) {
            if (ttick_mag_last == 0) { //first gyro to be recorded
                ttick_mag_first = data_start->time_stamp;
                ttick_mag_last = ttick_mag_first;
                mag.x = data_start->magnetic_t.x;
                mag.y = data_start->magnetic_t.y;
                mag.z = data_start->magnetic_t.z;
                mag.accuracy = data_start->magnetic_t.status;
            } else {
                ttick_mag_first = data_start->time_stamp;
                if ((ttick_mag_first <= ttick_mag_last)) {//redundancy input
                    return 0;
                } else {
                    ttick_mag_last = ttick_mag_first;
                    mag.x = data_start->magnetic_t.x;
                    mag.y = data_start->magnetic_t.y;
                    mag.z = data_start->magnetic_t.z;
                    mag.accuracy = data_start->magnetic_t.status;
                }
            }
            continue;
        } else if (data_start->sensor_type == SENSOR_TYPE_GPS) {
            gps_raw.lla_in[0] = data_start->gps_t.lla_in[0];
            gps_raw.lla_in[1] = data_start->gps_t.lla_in[1];
            gps_raw.lla_in[2] = data_start->gps_t.lla_in[2];
            gps_raw.vned_in[0] = data_start->gps_t.vned_in[0];
            gps_raw.vned_in[1] = data_start->gps_t.vned_in[1];
            gps_raw.vned_in[2] = data_start->gps_t.vned_in[2];
            gps_raw.lla_in_acc[0] = data_start->gps_t.lla_in_acc[0];
            gps_raw.lla_in_acc[1] = data_start->gps_t.lla_in_acc[1];
            gps_raw.lla_in_acc[2] = data_start->gps_t.lla_in_acc[2];
            gps_raw.vned_in_acc[0] = data_start->gps_t.vned_in_acc[0];
            gps_raw.vned_in_acc[1] = data_start->gps_t.vned_in_acc[1];
            gps_raw.vned_in_acc[2] = data_start->gps_t.vned_in_acc[2];
            gps_raw.gps_sec = data_start->gps_t.gps_sec;
            gps_raw.leap_sec = data_start->gps_t.leap_sec;
            //adr_timestamp = data_start->time_stamp;
            ///TODO: run adr here, relay data to flp service
        } else if (data_start->sensor_type == SENSOR_TYPE_PRESSURE) {
            if (ttick_baro_last == 0) { //first baro to be recorded
                ttick_baro_first = data_start->time_stamp;
                ttick_baro_last = ttick_baro_first;
                baro.x = data_start->pressure_t.pressure;
                baro.accuracy = data_start->pressure_t.status;
                baro.timestamp = data_start->time_stamp;
            } else {
                ttick_baro_first = data_start->time_stamp;
                if ((ttick_baro_first <= ttick_baro_last)) {//redundancy input
                    return 0;
                } else {
                    ttick_baro_last = ttick_baro_first;
                    baro.x = data_start->pressure_t.pressure;
                    baro.accuracy = data_start->pressure_t.status;
                    baro.timestamp = data_start->time_stamp;
                }
            }
            continue;
        }

        //Can only run algo after AGM first data is available
        //PLOG("adaptor dbg, type :%d, ttick: %lld,%lld,%lld\n",Algo_reg_group,ttick_acc_last,ttick_gyro_last,ttick_mag_last);
        if ((Algo_reg_group == (REG_AGM_ALGO))  //type 1: AGM
                || (Algo_reg_group == (REG_AGM_ALGO + REG_AG_ALGO)) //type 3: AGM +AG
                || (Algo_reg_group == (REG_AGM_ALGO + REG_AM_ALGO)) //type 5: AGM +AM
                || (Algo_reg_group == (REG_AG_ALGO + REG_AM_ALGO)) //type 6:AG + AM
                || (Algo_reg_group == (REG_AGM_ALGO + REG_AG_ALGO + REG_AM_ALGO))) {//type 7: AGM +AG + AM
            if ((ttick_acc_last != 0) && (ttick_gyro_last != 0) && (ttick_mag_last != 0)) {
                mpe_data_entry(&acceleration, 1, (U1)SENSOR_ACC);
                mpe_data_entry(&gyro, 1, (U1)SENSOR_GYRO);
                mpe_data_entry(&mag, 1, (U1)SENSOR_MAG);
                mpe_data_entry(&baro, 1, (U1)SENSOR_BARO);
                mpe_data_synch(Algo_reg_union, Algo_reg_group);
                mpe_get_adr_flag(&adr_result);
                if (adr_flag != adr_result.adr_status) {
                }
            }
        } else if (Algo_reg_group == REG_AG_ALGO) { //type 2:AG
            //PLOG("algo ag, %lld, %lld\n",ttick_acc_last, ttick_gyro_last);
            if ((ttick_acc_last != 0) && (ttick_gyro_last != 0)) {
                mpe_data_entry(&acceleration, 1, (U1)SENSOR_ACC);
                mpe_data_entry(&gyro, 1, (U1)SENSOR_GYRO);
                mpe_data_entry(&mag, 1, (U1)SENSOR_MAG);

                mpe_data_synch(Algo_reg_union, REG_AG_ALGO);
            }
        } else if (Algo_reg_group == (REG_AM_ALGO)) { //type 4:AM
            //PLOG("algo am, %lld, %lld\n", ttick_acc_last, ttick_mag_last);
            if ((ttick_acc_last != 0) && (ttick_mag_last != 0)) {
                mpe_data_entry(&acceleration, 1, (U1)SENSOR_ACC);
                mpe_data_entry(&mag, 1, (U1)SENSOR_MAG);
                //local gyro
                local_gyro.x = 3000;
                local_gyro.y = 6000;
                local_gyro.z = 9000;
                local_gyro.accuracy = 3;
                local_gyro.timestamp = acceleration.timestamp;
                mpe_data_entry(&local_gyro, 1, (U1)SENSOR_GYRO);
                mpe_data_synch(Algo_reg_union, REG_AM_ALGO);
            }
        }
        data_start++;
    }
    return 0;
}

/**************************************************************************/
/* Virtual sensor run algorithm*/
/**************************************************************************/
/* 1. SENSOR_TYPE_ORIENTATION */
static int run_Orient(struct data_t * const output)
{
    MPE_SENSOR_RESULT result;

    //Call mpe main function to extract mpe data
    memset(&result, 0, sizeof(MPE_SENSOR_RESULT));
    mpe_start_run1(&result, ORIENTATION_ALGO);
    PLOG("run_Orient,%d,%d,%d\n", (INT32)((result.x)*DATA_OUTPUT_SF_100), (INT32)((result.y)*DATA_OUTPUT_SF_100),
         (INT32)((result.z)*DATA_OUTPUT_SF_100));

    if (output == NULL)
        return -1;

    output->data->orientation_t.azimuth = (INT32)((result.x) * DATA_OUTPUT_SF_100);
    output->data->orientation_t.pitch = (INT32)((result.y) * DATA_OUTPUT_SF_100);
    output->data->orientation_t.roll = (INT32)((result.z) * DATA_OUTPUT_SF_100);
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->sensor_type = SENSOR_TYPE_ORIENTATION;
    output->data_exist_count = 1;
    return 1;
}

static int run_Gravity(struct data_t * const output)
{
    MPE_SENSOR_RESULT result;

    //Call mpe main function to extract mpe data
    memset(&result, 0, sizeof(MPE_SENSOR_RESULT));
    mpe_start_run1(&result, GRAVITY_ALGO);
    PLOG("run_Gravity,%d,%d,%d\n", (INT32)((result.x)*DATA_OUTPUT_SF_1000), (INT32)((result.y)*DATA_OUTPUT_SF_1000),
         (INT32)((result.z)*DATA_OUTPUT_SF_1000));

    if (output == NULL)
        return -1;

    output->data->accelerometer_t.x = (INT32)((result.x) * DATA_OUTPUT_SF_1000);
    output->data->accelerometer_t.y = (INT32)((result.y) * DATA_OUTPUT_SF_1000);
    output->data->accelerometer_t.z = (INT32)((result.z) * DATA_OUTPUT_SF_1000);
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->sensor_type = SENSOR_TYPE_GRAVITY;
    output->data_exist_count = 1;
    return 1;
}

static int run_Rotvec(struct data_t * const output)
{
    MPE_SENSOR_RESULT result;
    float vector_4;

    //Call mpe main function to extract mpe data
    memset(&result, 0, sizeof(MPE_SENSOR_RESULT));
    mpe_start_run1(&result, ROT_VEC_ALGO);
    PLOG("run_Rotvec,%d,%d,%d\n", (INT32)(result.x * DATA_OUTPUT_SF_1000), (INT32)(result.y * DATA_OUTPUT_SF_1000),
         (INT32)(result.z * DATA_OUTPUT_SF_1000));

    if (output == NULL)
        return -1;

    output->data->orientation_t.azimuth = (INT32)((result.x * DATA_OUTPUT_SF_1000000));
    output->data->orientation_t.pitch = (INT32)((result.y * DATA_OUTPUT_SF_1000000));
    output->data->orientation_t.roll = (INT32)((result.z * DATA_OUTPUT_SF_1000000));
    vector_4 = (1 - result.x * result.x - result.y * result.y - result.z * result.z);
    vector_4 = (vector_4 > 0.0f) ? sqrtf(vector_4) : 0.0f;
    output->data->orientation_t.scalar = (INT32)((vector_4) * DATA_OUTPUT_SF_1000000);
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->sensor_type = SENSOR_TYPE_ROTATION_VECTOR;
    output->data_exist_count = 1;
    return 1;
}

static int run_Geomag(struct data_t * const output)
{
    MPE_SENSOR_RESULT result;
    float vector_4;

    //Call mpe main function to extract mpe data
    memset(&result, 0, sizeof(MPE_SENSOR_RESULT));
    mpe_start_run1(&result, GEOMAG_ROT_VEC_ALGO);
    PLOG("run_Geomag,%d,%d,%d\n", (INT32)(result.x * DATA_OUTPUT_SF_1000), (INT32)(result.y * DATA_OUTPUT_SF_1000),
         (INT32)(result.z * DATA_OUTPUT_SF_1000));

    if (output == NULL)
        return -1;

    output->data->magnetic_t.azimuth = (INT32)((result.x * DATA_OUTPUT_SF_1000000));
    output->data->magnetic_t.pitch = (INT32)((result.y * DATA_OUTPUT_SF_1000000));
    output->data->magnetic_t.roll = (INT32)((result.z * DATA_OUTPUT_SF_1000000));
    vector_4 = (1 - result.x * result.x - result.y * result.y - result.z * result.z);
    vector_4 = (vector_4 > 0.0f) ? sqrtf(vector_4) : 0.0f;
    output->data->magnetic_t.scalar = (INT32)((vector_4) * DATA_OUTPUT_SF_1000000);
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->sensor_type = SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR;
    output->data_exist_count = 1;
    return 1;
}

static int run_GameRot(struct data_t * const output)
{
    MPE_SENSOR_RESULT result;
    float vector_4;

    //Call mpe main function to extract mpe data
    memset(&result, 0, sizeof(MPE_SENSOR_RESULT));
    mpe_start_run1(&result, GAME_ROT_VEC);
    PLOG("run_GameRot,%d,%d,%d\n", (INT32)(result.x * DATA_OUTPUT_SF_1000), (INT32)(result.y * DATA_OUTPUT_SF_1000),
         (INT32)(result.z * DATA_OUTPUT_SF_1000));

    if (output == NULL)
        return -1;

    output->data->orientation_t.azimuth = (INT32)((result.x) * DATA_OUTPUT_SF_1000000);
    output->data->orientation_t.pitch = (INT32)((result.y) * DATA_OUTPUT_SF_1000000);
    output->data->orientation_t.roll = (INT32)((result.z) * DATA_OUTPUT_SF_1000000);
    vector_4 = (1 - result.x * result.x - result.y * result.y - result.z * result.z);
    vector_4 = (vector_4 > 0.0f) ? sqrtf(vector_4) : 0.0f;
    output->data->orientation_t.scalar = (INT32)((vector_4) * DATA_OUTPUT_SF_1000000);
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->sensor_type = SENSOR_TYPE_GAME_ROTATION_VECTOR;
    output->data_exist_count = 1;
    return 1;
}

static int run_LinearAcc(struct data_t * const output)
{
    MPE_SENSOR_RESULT result;

    //Call mpe main function to extract mpe data
    memset(&result, 0, sizeof(MPE_SENSOR_RESULT));
    mpe_start_run1(&result, LINEAR_ACC_ALGO);
    PLOG("run_LinearAcc,%d,%d,%d\n", (INT32)((result.x)*DATA_OUTPUT_SF_1000), (INT32)((result.y)*DATA_OUTPUT_SF_1000),
         (INT32)((result.z)*DATA_OUTPUT_SF_1000));

    if (output == NULL)
        return -1;

    output->data->accelerometer_t.x = (INT32)((result.x) * DATA_OUTPUT_SF_1000);
    output->data->accelerometer_t.y = (INT32)((result.y) * DATA_OUTPUT_SF_1000);
    output->data->accelerometer_t.z = (INT32)((result.z) * DATA_OUTPUT_SF_1000);
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->sensor_type = SENSOR_TYPE_LINEAR_ACCELERATION;
    output->data_exist_count = 1;
    return 1;
}

static int run_PDR(struct data_t * const output)
{
    MPE_SENSOR_RESULT result;

    //Call mpe main function to extract mpe data
    memset(&result, 0, sizeof(MPE_SENSOR_RESULT));
    mpe_start_run1(&result, PDR_ALGO);
    PLOG("run_PDR,%d,%d,%d\n", (INT32)(result.x), (INT32)(result.y), (INT32)(result.z));

    if (output == NULL)
        return -1;

    output->data->pdr_event.x = (INT32)((result.x));
    output->data->pdr_event.y = (INT32)((result.y));
    output->data->pdr_event.z = (INT32)((result.z));
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->sensor_type = SENSOR_TYPE_PDR;
    output->data_exist_count = 1;
    return 1;
}

/**************************************************************************/
/*Registration and initialization of virtual sensor*/
/**************************************************************************/
/* 1. SENSOR_TYPE_ORIENTATION */
int Orientation_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            PLOG("Orientation_operation command ACTIVATE: %d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PLOG("Enable Orientation_operation error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    PLOG("Disable Orientation!\n\r");
                    Algo_reg_union &= (~REG_ORIENTATION_ALGO);
                    Algo_reg_group &= (~REG_AGM_ALGO);
                } else {
                    Algo_reg_union |= REG_ORIENTATION_ALGO;
                    Algo_reg_group |= REG_AGM_ALGO;
                    mpe_re_initialize();
                }
            }
            break;
        default:
            break;
    }
    return err;
}

int Orientation_register()
{
    int ret;
    struct SensorDescriptor_t  Orient_descriptor_t;
    struct input_list_t input_comp_acc;
    struct input_list_t input_comp_gyro;
    struct input_list_t input_comp_mag;

    //Orientation description setting
    Orient_descriptor_t.sensor_type = SENSOR_TYPE_ORIENTATION;
    Orient_descriptor_t.version = 1;
    Orient_descriptor_t.report_mode = continus;
    Orient_descriptor_t.hw.max_sampling_rate = 1;
    Orient_descriptor_t.hw.support_HW_FIFO = 0;
    Orient_descriptor_t.input_list = &input_comp_acc; //children list: acc + gyro +mag
    Orient_descriptor_t.operate = Orientation_operation;
    Orient_descriptor_t.run_algorithm = run_Orient;
    Orient_descriptor_t.set_data = set_PDR_data;
    Orient_descriptor_t.accumulate = 200;

    //Register Acc
    input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
    //input_comp_acc.sampling_delay = 20; //50Hz
    input_comp_acc.sampling_delay = ACC_DATA_INTERVAL; //25Hz

    //Register Gyro
    input_comp_gyro.input_type = SENSOR_TYPE_GYROSCOPE;
    input_comp_gyro.sampling_delay = 20;  //200Hz, inject 4data every 20ms
    input_comp_acc.next_input = &input_comp_gyro;

    //Register Mag
    input_comp_mag.input_type = SENSOR_TYPE_MAGNETIC_FIELD;
    input_comp_mag.sampling_delay = 20;  //50Hz
    input_comp_gyro.next_input = &input_comp_mag;
    input_comp_mag.next_input = NULL;

    mpe_re_initialize();
    ret = sensor_subsys_algorithm_register_type(&Orient_descriptor_t);
    sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_ORIENTATION, DR_OUTPUT_RATE);
    if (ret < 0) {
        PLOG("fail to register Orientation sensor\r\n");
    } /*else {
    Algo_reg_union |= REG_ORIENTATION_ALGO;
    Algo_reg_group |= REG_AGM_ALGO;
  }*/
    return ret;
}

/* 2. SENSOR_TYPE_GRAVITY */
int Gravity_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            PLOG("Gravity_operation command ACTIVATE: %d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PLOG("Enable Gravity_operation error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    PLOG("Disable Gravity!\n\r");
                    Algo_reg_union &= (~REG_GRAVITY_ALGO);
                    Algo_reg_group &= (~REG_AGM_ALGO);
                } else {
                    Algo_reg_union |= REG_GRAVITY_ALGO;
                    Algo_reg_group |= REG_AGM_ALGO;
                    mpe_re_initialize();
                }
            }
            break;
        default:
            break;
    }
    return err;
}

int Gravity_register()
{
    int ret;
    struct SensorDescriptor_t  Gravity_descriptor_t;
    struct input_list_t input_comp_acc;
    struct input_list_t input_comp_gyro;
    struct input_list_t input_comp_mag;

    //Orientation description setting
    Gravity_descriptor_t.sensor_type = SENSOR_TYPE_GRAVITY;
    Gravity_descriptor_t.version = 1;
    Gravity_descriptor_t.report_mode = continus;
    Gravity_descriptor_t.hw.max_sampling_rate = 1;
    Gravity_descriptor_t.hw.support_HW_FIFO = 0;
    Gravity_descriptor_t.input_list = &input_comp_acc; //children list: acc + gyro +mag
    Gravity_descriptor_t.operate = Gravity_operation;
    Gravity_descriptor_t.run_algorithm = run_Gravity;
    Gravity_descriptor_t.set_data = set_PDR_data;
    Gravity_descriptor_t.accumulate = 200;

    //Register Acc
    input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
    //input_comp_acc.sampling_delay = 20; //50Hz
    input_comp_acc.sampling_delay = ACC_DATA_INTERVAL; //25Hz

    //Register Gyro
    input_comp_gyro.input_type = SENSOR_TYPE_GYROSCOPE;
    input_comp_gyro.sampling_delay = 20;  //200Hz,inject 4data every 20ms
    input_comp_acc.next_input = &input_comp_gyro;

    //Register Mag
    input_comp_mag.input_type = SENSOR_TYPE_MAGNETIC_FIELD;
    input_comp_mag.sampling_delay = 20;  //50Hz
    input_comp_gyro.next_input = &input_comp_mag;
    input_comp_mag.next_input = NULL;

    mpe_re_initialize();
    ret = sensor_subsys_algorithm_register_type(&Gravity_descriptor_t);
    sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_GRAVITY, DR_OUTPUT_RATE);
    if (ret < 0) {
        PLOG("fail to register Gravity sensor\r\n");
    } /*else {
      Algo_reg_union |= REG_GRAVITY_ALGO;
      Algo_reg_group |= REG_AGM_ALGO;
  }*/
    return ret;
}

/* 3. SENSOR_TYPE_ROTATION_VECTOR */
int Rotation_vec_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            PLOG("Rotation_vec_operation command ACTIVATE: %d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PLOG("Enable Rotation_vec_operation error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    PLOG("Disable Rotation vector!\n\r");
                    Algo_reg_union &= (~REG_ROT_VEC_ALGO);
                    Algo_reg_group &= (~REG_AGM_ALGO);
                } else {
                    Algo_reg_union |= REG_ROT_VEC_ALGO;
                    Algo_reg_group |= REG_AGM_ALGO;
                    mpe_re_initialize();
                }
            }
            break;
        default:
            break;
    }
    return err;
}

int Rotation_vec_register()
{
    int ret;
    struct SensorDescriptor_t  Rot_descriptor_t;
    struct input_list_t input_comp_acc;
    struct input_list_t input_comp_gyro;
    struct input_list_t input_comp_mag;

    //Orientation description setting
    Rot_descriptor_t.sensor_type = SENSOR_TYPE_ROTATION_VECTOR;
    Rot_descriptor_t.version = 1;
    Rot_descriptor_t.report_mode = continus;
    Rot_descriptor_t.hw.max_sampling_rate = 1;
    Rot_descriptor_t.hw.support_HW_FIFO = 0;
    Rot_descriptor_t.input_list = &input_comp_acc; //children list: acc + gyro +mag
    Rot_descriptor_t.operate = Rotation_vec_operation;
    Rot_descriptor_t.run_algorithm = run_Rotvec;
    Rot_descriptor_t.set_data = set_PDR_data;
    Rot_descriptor_t.accumulate = 200;

    //Register Acc
    input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
    //input_comp_acc.sampling_delay = 20; //50Hz
    input_comp_acc.sampling_delay = ACC_DATA_INTERVAL; //25Hz

    //Register Gyro
    input_comp_gyro.input_type = SENSOR_TYPE_GYROSCOPE;
    input_comp_gyro.sampling_delay = 20;  //200Hz,inject 4data every 20ms
    input_comp_acc.next_input = &input_comp_gyro;

    //Register Mag
    input_comp_mag.input_type = SENSOR_TYPE_MAGNETIC_FIELD;
    input_comp_mag.sampling_delay = 20;  //50Hz
    input_comp_gyro.next_input = &input_comp_mag;
    input_comp_mag.next_input = NULL;

    mpe_re_initialize();
    ret = sensor_subsys_algorithm_register_type(&Rot_descriptor_t);
    sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_ROTATION_VECTOR, DR_OUTPUT_RATE);
    if (ret < 0) {
        PLOG("fail to register rotation vector sensor\r\n");
    } /*else {
    Algo_reg_union |= REG_ROT_VEC_ALGO;
    Algo_reg_group |= REG_AGM_ALGO;
  }*/
    return ret;
}

/* 4. SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR */
int Geomag_vec_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            PLOG("Geomag_vec_operation command ACTIVATE: %d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PLOG("Enable Geomag_vec_operation error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    Algo_reg_union &= (~REG_GEOMAG_ROT_VEC_ALGO);
                    Algo_reg_group &= (~REG_AM_ALGO);
                } else {
                    Algo_reg_union |= REG_GEOMAG_ROT_VEC_ALGO;
                    Algo_reg_group |= REG_AM_ALGO;
                }
            }
            break;
        default:
            break;
    }
    return err;
}

int Geomag_vec_register()
{
    int ret;
    struct SensorDescriptor_t  Geomag_descriptor_t;
    struct input_list_t input_comp_acc;
    struct input_list_t input_comp_mag;

    //Orientation description setting
    Geomag_descriptor_t.sensor_type = SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR;
    Geomag_descriptor_t.version = 1;
    Geomag_descriptor_t.report_mode = continus;
    Geomag_descriptor_t.hw.max_sampling_rate = 1;
    Geomag_descriptor_t.hw.support_HW_FIFO = 0;
    Geomag_descriptor_t.input_list = &input_comp_acc; //children list: acc + gyro +mag
    Geomag_descriptor_t.operate = Geomag_vec_operation;
    Geomag_descriptor_t.run_algorithm = run_Geomag;
    Geomag_descriptor_t.set_data = set_PDR_data;
    Geomag_descriptor_t.accumulate = 200;

    //Register Acc
    input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
    //input_comp_acc.sampling_delay = 20; //50Hz
    input_comp_acc.sampling_delay = ACC_DATA_INTERVAL; //25Hz

    //Register Mag
    input_comp_mag.input_type = SENSOR_TYPE_MAGNETIC_FIELD;
    input_comp_mag.sampling_delay = 20;  //50Hz
    input_comp_acc.next_input = &input_comp_mag;
    input_comp_mag.next_input = NULL;

    ret = sensor_subsys_algorithm_register_type(&Geomag_descriptor_t);
    sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR, DR_OUTPUT_RATE);
    if (ret < 0) {
        PLOG("fail to register geomag sensor\r\n");
    } /*else {
    Algo_reg_union |= REG_GEOMAG_ROT_VEC_ALGO;
    Algo_reg_group |= REG_AM_ALGO;
  }*/
    return ret;
}

/* 5. SENSOR_TYPE_GAME_ROTATION_VECTOR */
int Game_rot_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            PLOG("Game_rot_operation command ACTIVATE: %d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PLOG("Enable Game_rot_operation error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    Algo_reg_union &= (~REG_GAME_ROT_VEC);
                    Algo_reg_group &= (~REG_AG_ALGO);
                } else {
                    Algo_reg_union |= REG_GAME_ROT_VEC;
                    Algo_reg_group |= REG_AG_ALGO;
                    mpe_re_initialize();
                }
            }
            break;
        default:
            break;
    }
    return err;
}

int Game_rot_register()
{
    int ret;
    struct SensorDescriptor_t  GameRot_descriptor_t;
    struct input_list_t input_comp_acc;
    struct input_list_t input_comp_gyro;

    //Orientation description setting
    GameRot_descriptor_t.sensor_type = SENSOR_TYPE_GAME_ROTATION_VECTOR;
    GameRot_descriptor_t.version = 1;
    GameRot_descriptor_t.report_mode = continus;
    GameRot_descriptor_t.hw.max_sampling_rate = 1;
    GameRot_descriptor_t.hw.support_HW_FIFO = 0;
    GameRot_descriptor_t.input_list = &input_comp_acc; //children list: acc + gyro +mag
    GameRot_descriptor_t.operate = Game_rot_operation;
    GameRot_descriptor_t.run_algorithm = run_GameRot;
    GameRot_descriptor_t.set_data = set_PDR_data;
    GameRot_descriptor_t.accumulate = 200;

    //Register Acc
    input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
    //input_comp_acc.sampling_delay = 20; //50Hz
    input_comp_acc.sampling_delay = ACC_DATA_INTERVAL; //25Hz

    //Register Gyro
    input_comp_gyro.input_type = SENSOR_TYPE_GYROSCOPE;
    input_comp_gyro.sampling_delay = 20;  //200Hz, inject every 20ms
    input_comp_acc.next_input = &input_comp_gyro;
    input_comp_gyro.next_input = NULL;

    mpe_re_initialize();
    ret = sensor_subsys_algorithm_register_type(&GameRot_descriptor_t);
    sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_GAME_ROTATION_VECTOR, DR_OUTPUT_RATE);
    if (ret < 0) {
        PLOG("fail to register game rotation sensor\r\n");
    } /*else {
    Algo_reg_union |= REG_GAME_ROT_VEC;
    Algo_reg_group |= REG_AG_ALGO;
  }*/
    return ret;
}

/* 6. SENSOR_TYPE_LINEAR_ACCELERATION */
int Linear_acc_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            PLOG("Linear_acc_operation command ACTIVATE: %d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PLOG("Enable Linear_acc_operation error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    Algo_reg_union &= (~REG_LINEAR_ACC_ALGO);
                    Algo_reg_group &= (~REG_AGM_ALGO);
                } else {
                    Algo_reg_union |= REG_LINEAR_ACC_ALGO;
                    Algo_reg_group |= REG_AGM_ALGO;
                    mpe_re_initialize();
                }
            }
            break;
        default:
            break;
    }
    return err;
}

int Linear_acc_register()
{
    int ret;
    struct SensorDescriptor_t  LinearAcc_descriptor_t;
    struct input_list_t input_comp_acc;
    struct input_list_t input_comp_gyro;
    struct input_list_t input_comp_mag;

    //Orientation description setting
    LinearAcc_descriptor_t.sensor_type = SENSOR_TYPE_LINEAR_ACCELERATION;
    LinearAcc_descriptor_t.version = 1;
    LinearAcc_descriptor_t.report_mode = continus;
    LinearAcc_descriptor_t.hw.max_sampling_rate = 1;
    LinearAcc_descriptor_t.hw.support_HW_FIFO = 0;
    LinearAcc_descriptor_t.input_list = &input_comp_acc; //children list: acc + gyro +mag
    LinearAcc_descriptor_t.operate = Linear_acc_operation;
    LinearAcc_descriptor_t.run_algorithm = run_LinearAcc;
    LinearAcc_descriptor_t.set_data = set_PDR_data;
    LinearAcc_descriptor_t.accumulate = 200;

    //Register Acc
    input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
    //input_comp_acc.sampling_delay = 20; //50Hz
    input_comp_acc.sampling_delay = ACC_DATA_INTERVAL; //25Hz

    //Register Gyro
    input_comp_gyro.input_type = SENSOR_TYPE_GYROSCOPE;
    input_comp_gyro.sampling_delay = 20;  //200Hz, inject every 20ms
    input_comp_acc.next_input = &input_comp_gyro;

    //Register Mag
    input_comp_mag.input_type = SENSOR_TYPE_MAGNETIC_FIELD;
    input_comp_mag.sampling_delay = 20;  //50Hz
    input_comp_gyro.next_input = &input_comp_mag;
    input_comp_mag.next_input = NULL;

    mpe_re_initialize();
    ret = sensor_subsys_algorithm_register_type(&LinearAcc_descriptor_t);
    sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_LINEAR_ACCELERATION, DR_OUTPUT_RATE);
    if (ret < 0) {
        PLOG("fail to register linear acc sensor\r\n");
    } /*else {
    Algo_reg_union |= REG_LINEAR_ACC_ALGO;
    Algo_reg_group |= REG_AGM_ALGO;
  }*/
    return ret;
}

/* 9. SENSOR_TYPE_PDR */
int PDR_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out)
{
    int err = 0;
    int value = 0;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            PLOG("PDR_operation command ACTIVATE: %d\n\r", *(int *)buffer_in);
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                PLOG("Enable PDR_operation error!\n\r");
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) {
                    Algo_reg_union ^= REG_PDR_ALGO;
                    Algo_reg_union ^= REG_ADR_ALGO; //register adr type for baro height retrieval
                    Algo_reg_group ^= REG_AGM_ALGO;
                } else {
                    Algo_reg_union |= REG_PDR_ALGO;
                    Algo_reg_union |= REG_ADR_ALGO; //register adr type for baro height retrieval
                    Algo_reg_group |= REG_AGM_ALGO;
                }
            }
            break;
        default:
            break;
    }
    return err;

}

int PDR_register()
{
    int ret;
    struct SensorDescriptor_t  PDR_descriptor_t;
    struct input_list_t input_comp_acc;
    struct input_list_t input_comp_gyro;
    struct input_list_t input_comp_mag;
    struct input_list_t input_comp_baro;

    //PDR description setting
    PDR_descriptor_t.sensor_type = SENSOR_TYPE_PDR;
    PDR_descriptor_t.version = 1;
    PDR_descriptor_t.report_mode = continus;
    PDR_descriptor_t.hw.max_sampling_rate = 1;
    PDR_descriptor_t.hw.support_HW_FIFO = 0;
    PDR_descriptor_t.input_list = &input_comp_acc; //children list: acc + gyro +mag
    PDR_descriptor_t.operate = PDR_operation;
    PDR_descriptor_t.run_algorithm = run_PDR;
    PDR_descriptor_t.set_data = set_PDR_data;
    PDR_descriptor_t.accumulate = 200;

    //Register Acc
    input_comp_acc.input_type = SENSOR_TYPE_ACCELEROMETER;
    //input_comp_acc.sampling_delay = 20; //50Hz
    input_comp_acc.sampling_delay = ACC_DATA_INTERVAL; //25Hz

    //Register Gyro
    input_comp_gyro.input_type = SENSOR_TYPE_GYROSCOPE;
    input_comp_gyro.sampling_delay = 20;  //200Hz, inject every 20ms
    input_comp_acc.next_input = &input_comp_gyro;

    //Register Mag
    input_comp_mag.input_type = SENSOR_TYPE_MAGNETIC_FIELD;
    input_comp_mag.sampling_delay = 20;  //50Hz
    input_comp_gyro.next_input = &input_comp_mag;

    //Register Baro
    input_comp_baro.input_type = SENSOR_TYPE_PRESSURE;
    input_comp_baro.sampling_delay = 60; //50Hz
    input_comp_mag.next_input = &input_comp_baro;
    input_comp_baro.next_input = NULL;

#if 0
    //Register GPS
    input_comp_gps.input_type = (INT32)SENSOR_TYPE_GPS;
    input_comp_gps.sampling_delay = 1000; //1Hz
    input_comp_mag.next_input = &input_comp_gps;
    input_comp_gps.next_input = NULL;
#endif

    ret = sensor_subsys_algorithm_register_type(&PDR_descriptor_t);
    sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_PDR, DR_OUTPUT_RATE);
    if (ret < 0) {
        PLOG("fail to register PDR \r\n");
    } /*else {
    Algo_reg_union |= REG_PDR_ALGO;
    Algo_reg_union |= REG_ADR_ALGO; //register adr type for baro height retrieval
    Algo_reg_group |= REG_AGM_ALGO;
  }*/
    return ret;
}


MODULE_DECLARE(virt_orientation, MOD_VIRT_SENSOR, Orientation_register);
MODULE_DECLARE(virt_gravity, MOD_VIRT_SENSOR, Gravity_register);
MODULE_DECLARE(virt_rotation_vec, MOD_VIRT_SENSOR, Rotation_vec_register);
MODULE_DECLARE(virt_geomag_vec, MOD_VIRT_SENSOR, Geomag_vec_register);
MODULE_DECLARE(virt_game_rot, MOD_VIRT_SENSOR, Game_rot_register);
MODULE_DECLARE(virt_linear_acc, MOD_VIRT_SENSOR, Linear_acc_register);
MODULE_DECLARE(virt_pdr, MOD_VIRT_SENSOR, PDR_register);
MODULE_DECLARE(virt_pdr_init, MOD_VIRT_SENSOR, PDR_init);


#endif /* __PDR_ADAPTOR_C_INCLUDED__ */
