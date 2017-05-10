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
#ifndef __GPS_DRIVER_C_INCLUDED__
#define __GPS_DRIVER_C_INCLUDED__

#include <string.h>
#include <stdio.h>
#include "module.h"
#include "gps_driver.h"
#include "sensor_manager.h"

#define GPS_DBG
#define GLOGE(fmt, args...)    PRINTF_D("[GPS] ERR: "fmt, ##args)
#ifdef GPS_DBG
#define GLOGD(fmt, args...)    PRINTF_D("[GPS] DBG: "fmt, ##args)
#else
#define GLOGD(fmt, args...)
#endif

/************************************************************************/
//  Global Variable
/************************************************************************/
static gps_vec_t Gps_info;

/************************************************************************/
//  Function Body
/************************************************************************/
int GPS_init() {
   memset(&Gps_info,0,sizeof(gps_vec_t));
   return 0;
}

static int GPS_run_algorithm(struct data_t * const output) {
  if(output == NULL)
    return -1;

  output->data->sensor_type = SENSOR_TYPE_GPS;
  output->data->time_stamp = timestamp_get_ns();
  output->data_exist_count = 1;
  output->data->gps_t.lla_in[0] = Gps_info.lla_in[0];
  output->data->gps_t.lla_in[1] = Gps_info.lla_in[1];
  output->data->gps_t.lla_in[2] = Gps_info.lla_in[2];
  output->data->gps_t.vned_in[0] = Gps_info.vned_in[0];
  output->data->gps_t.vned_in[1] = Gps_info.vned_in[1];
  output->data->gps_t.vned_in[2] = Gps_info.vned_in[2];
  output->data->gps_t.lla_in_acc[0] = Gps_info.lla_in_acc[0];
  output->data->gps_t.lla_in_acc[1] = Gps_info.lla_in_acc[1];
  output->data->gps_t.lla_in_acc[2] = Gps_info.lla_in_acc[2];
  output->data->gps_t.vned_in_acc[0] = Gps_info.vned_in_acc[0];
  output->data->gps_t.vned_in_acc[1] = Gps_info.vned_in_acc[1];
  output->data->gps_t.vned_in_acc[2] = Gps_info.vned_in_acc[2];
  output->data->gps_t.gps_sec = Gps_info.gps_sec;
  output->data->gps_t.leap_sec = Gps_info.leap_sec;
  return 1;
}

int GPS_set_data(gps_vec_t gps_in) {
  Gps_info.lla_in[0] = gps_in.lla_in[0];
  Gps_info.lla_in[1] = gps_in.lla_in[1];
  Gps_info.lla_in[2] = gps_in.lla_in[2];
  Gps_info.vned_in[0] = gps_in.vned_in[0];
  Gps_info.vned_in[1] = gps_in.vned_in[1];
  Gps_info.vned_in[2] = gps_in.vned_in[2];
  Gps_info.lla_in_acc[0] = gps_in.lla_in_acc[0];
  Gps_info.lla_in_acc[1] = gps_in.lla_in_acc[1];
  Gps_info.lla_in_acc[2] = gps_in.lla_in_acc[2];
  Gps_info.vned_in_acc[0]= gps_in.lla_in_acc[0];
  Gps_info.vned_in_acc[0]= gps_in.lla_in_acc[0];
  Gps_info.vned_in_acc[0]= gps_in.lla_in_acc[0];
  Gps_info.gps_sec = gps_in.gps_sec;
  Gps_info.leap_sec= gps_in.leap_sec;
  GLOGD("gps info %lf, %lf %lf\n", Gps_info.lla_in[0], Gps_info.lla_in[1], Gps_info.lla_in[2]);
  return 0;
}

int GPS_operation(Sensor_Command command, void *buffer_in, int size_in, void *buffer_out, int size_out) {
  return 0;
}

int GPS_register() {
  int ret;
  struct SensorDescriptor_t  GPS_descriptor_t;

  //Orientation description setting
  GPS_descriptor_t.sensor_type = SENSOR_TYPE_GPS;
  GPS_descriptor_t.version = 1;
  GPS_descriptor_t.report_mode = continus;
  GPS_descriptor_t.hw.max_sampling_rate = 1;
  GPS_descriptor_t.hw.support_HW_FIFO = 0;
  GPS_descriptor_t.input_list = NULL;
  GPS_descriptor_t.operate = GPS_operation;
  GPS_descriptor_t.run_algorithm = GPS_run_algorithm;
  GPS_descriptor_t.set_data = NULL;
  GPS_descriptor_t.accumulate = 1000;

  ret = sensor_subsys_algorithm_register_type(&GPS_descriptor_t);
  sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_GPS, 1);
  if(ret<0) {
    GLOGD("fail to register GPS sensor\r\n");
  }
  return ret;
}


MODULE_DECLARE(flp_gps, MOD_PHY_SENSOR, GPS_register );

#endif //#ifndef __GPS_DRIVER_C_INCLUDED__
