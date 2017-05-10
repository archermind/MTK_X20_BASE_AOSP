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

#ifndef SHF_TYPES_H_
#define SHF_TYPES_H_

#include <sys/types.h>
//#include <platform/mt_typedefs.h>
#include <stdint.h>

#ifndef bool_t
typedef unsigned char bool_t;
#endif

#ifndef float_t
typedef float float_t;
#endif

/*
#ifndef uint8_t
typedef unsigned char uint8_t;
#endif

#ifndef uint16_t
typedef unsigned short uint16_t;
#endif

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

#ifndef uint64_t
typedef unsigned long long uint64_t;
#endif

#ifndef uint
typedef unsigned int uint;
#endif
*/
#ifndef status_t
typedef int status_t;
#endif
/*
#ifndef NO_ERROR
#define NO_ERROR (0x00)
#endif

#ifndef
#define ERROR -1
#endif
*/

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

//TODO: should enable it if align 4 bytes for uint64. otherwise, comment it.
#define ALIGN_UINT64
#ifndef ALIGN_UINT64
typedef struct {
    uint32_t time_l;
    uint32_t time_h;
} timestamp_t;
#else
typedef uint64_t timestamp_t;
#endif

bool_t contain(const uint8_t* const list, uint8_t size, uint8_t find);
bool_t unique_add(uint8_t* const list, uint8_t max, uint8_t* size, uint8_t find);

uint64_t timestamp_get(timestamp_t* timestamp);
void timestamp_set(timestamp_t* timestamp, uint64_t value);
void timestamp_set_now(timestamp_t* timestamp);
uint64_t timestamp_get_now();

uint16_t convert_uint8_2_uint16(uint8_t* buf);
uint64_t convert_uint8_2_uint64(uint8_t* buf);

size_t save_uint16_2_uint8(uint16_t v, uint8_t* buf);
size_t save_uint32_2_uint8(uint32_t v, uint8_t* buf);
size_t save_uint64_2_uint8(uint64_t v, uint8_t* buf);
size_t save_float_2_uint8(float v, uint8_t* buf);

#ifdef SHF_UNIT_TEST_ENABLE
void shf_types_unit_test();
#endif

#endif /* SHF_TYPES_H_ */
