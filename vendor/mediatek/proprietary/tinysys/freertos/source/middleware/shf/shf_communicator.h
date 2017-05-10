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

#ifndef SHF_COMMUNICATOR_H_
#define SHF_COMMUNICATOR_H_

#include "shf_define.h"

/**
 * Now, we only support max length is 255(uint8),
 * but, we define API length is size_t,
 * so if we want to support more size,
 * we can only modify .c file.
 */

#define SHF_AP_BUFFER_BYTES (SHF_IPI_PROTOCOL_BYTES * 2)

#ifdef SHF_CONSYS_TYPE_UART
#define SHF_CONSYS_BUFFER_BYTES (SHF_UART_PROTOCOL_BYTES * 1)
#else
#define SHF_CONSYS_BUFFER_BYTES (SHF_IPI_PROTOCOL_BYTES * 1)
#endif

#define SHF_PROTOCOL_SEND_BUFFER_BYTES (256)

typedef uint8_t shf_device_t;
// device enum @{
#define SHF_DEVICE_AP              (0x01)
#define SHF_DEVICE_CONSYS          (0x02)
#define SHF_DEVICE_TOUCH_PANEL     (0x03)
#define SHF_DEVICE_DRAM            (0x04)
//@}

typedef void (*communicator_handler_t)(void * data, size_t size);

status_t shf_communicator_send_message(shf_device_t device, void* data, size_t size);
status_t shf_communicator_receive_message(shf_device_t device, communicator_handler_t handler);
status_t shf_communicator_wakeup(shf_device_t device);
status_t shf_communicator_enable_disable(shf_device_t device, bool_t enable);
void  shf_communicator_run();
void shf_communicator_init();

#ifdef SHF_UNIT_TEST_ENABLE
void shf_communicator_unit_test();
#endif

#ifdef SHF_INTEGRATION_TEST_ENABLE
void shf_communicator_it();
void shf_communicator_it2();
#endif

#endif /* SHF_COMMUNICATOR_H_ */
