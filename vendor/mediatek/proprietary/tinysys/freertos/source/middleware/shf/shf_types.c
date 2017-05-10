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

#include "shf_debug.h"
#include "shf_types.h"

#include "utils.h"

//#include <platform.h>

#ifdef SHF_UNIT_TEST_ENABLE
#include "mock_device.h"
#include "shf_unit_test.h"
#endif

bool_t contain(const uint8_t* const list, uint8_t size, uint8_t find)
{
    uint8_t i;
    for (i = 0; i < size; i++) {
        if (*(list + i) == find) {
            return TRUE;
        }
    }
    return FALSE;
}

bool_t unique_add(uint8_t* const list, uint8_t max, uint8_t* size, uint8_t find)
{
    if (!contain(list, *size, find) && *size < max) {
        list[(*size)++] = find;
        return TRUE;
    } else {
        return FALSE;
    }
}

uint64_t timestamp_get(timestamp_t* timestamp)
{
    return convert_uint8_2_uint64((uint8_t*)timestamp);
}

void timestamp_set(timestamp_t* timestamp, uint64_t value)
{
    save_uint64_2_uint8(value, (uint8_t*)timestamp);
}

void timestamp_set_now(timestamp_t* timestamp)
{
    timestamp_set(timestamp, timestamp_get_now());
}

uint64_t timestamp_get_now()
{
    return timestamp_get_ns();//current_time();
}

uint16_t convert_uint8_2_uint16(uint8_t* buf)
{
    return (((uint16_t) buf[0]) & 0x00FF) + ((((uint16_t) buf[1]) & 0x00FF) << 8);
}

uint64_t convert_uint8_2_uint64(uint8_t* buf)
{
    return (((uint64_t) buf[0]) & 0x00000000000000FF) +
           ((((uint64_t) buf[1]) & 0x00000000000000FF) << 8) +
           ((((uint64_t) buf[2]) & 0x00000000000000FF) << 16) +
           ((((uint64_t) buf[3]) & 0x00000000000000FF) << 24) +
           ((((uint64_t) buf[4]) & 0x00000000000000FF) << 32) +
           ((((uint64_t) buf[5]) & 0x00000000000000FF) << 40) +
           ((((uint64_t) buf[6]) & 0x00000000000000FF) << 48) +
           ((((uint64_t) buf[7]) & 0x00000000000000FF) << 56);
}

size_t save_uint16_2_uint8(uint16_t v, uint8_t* buf)
{
    buf[0] = ((v >> 0) & 0x00FF);
    buf[1] = ((v >> 8) & 0x00FF);
    return 2;
}

size_t save_uint32_2_uint8(uint32_t v, uint8_t* buf)
{
    buf[0] = ((v >> 0) & 0x000000FF);
    buf[1] = ((v >> 8) & 0x000000FF);
    buf[2] = ((v >> 16) & 0x000000FF);
    buf[3] = ((v >> 24) & 0x000000FF);
    return 4;
}

size_t save_uint64_2_uint8(uint64_t v, uint8_t* buf)
{
    buf[0] = ((v >> 0) & 0x00000000000000FF);
    buf[1] = ((v >> 8) & 0x00000000000000FF);
    buf[2] = ((v >> 16) & 0x00000000000000FF);
    buf[3] = ((v >> 24) & 0x00000000000000FF);
    buf[4] = ((v >> 32) & 0x00000000000000FF);
    buf[5] = ((v >> 40) & 0x00000000000000FF);
    buf[6] = ((v >> 48) & 0x00000000000000FF);
    buf[7] = ((v >> 56) & 0x00000000000000FF);
    return 8;
}

size_t save_float_2_uint8(float v, uint8_t* buf)
{
    //uint32_t tv = *((uint32_t*)(&v));
    uint32_t tv = (uint32_t)v;
    buf[0] = ((tv >> 0) & 0x000000FF);
    buf[1] = ((tv >> 8) & 0x000000FF);
    buf[2] = ((tv >> 16) & 0x000000FF);
    buf[3] = ((tv >> 24) & 0x000000FF);
    return 4;
}

#ifdef SHF_UNIT_TEST_ENABLE
void shf_types_unit_test()
{
    uint8_t inbuf[8] = {0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8};
    uint16_t in16 = 0xF2F1;
    uint32_t in32 = 0xF4F3F2F1;
    uint64_t in64 = 0xF8F7F6F5F4F3F2F1;
    uint8_t outbuf[8];
    uint16_t out16;
    uint64_t out64;
    int size;
    int i;

    out16 = convert_uint8_2_uint16(inbuf);
    unit_assert("uint8_2_uint16", 0xF2F1, out16);

    out64 = convert_uint8_2_uint64(inbuf);
    unit_assert_u64("uint8_2_uint64", 0xF8F7F6F5F4F3F2F1, out64);

    size = save_uint16_2_uint8(in16, outbuf);
    for (i = 0; i < size; i++) {
        if (inbuf[i] != outbuf[i]) {
            logw("uint16_2_uint8: index=%d failed. %u!=%u", i, inbuf[i], outbuf[i]);
        }
    }

    size = save_uint32_2_uint8(in32, outbuf);
    for (i = 0; i < size; i++) {
        if (inbuf[i] != outbuf[i]) {
            logw("uint32_2_uint8: index=%d failed. %u!=%u", i, inbuf[i], outbuf[i]);
        }
    }

    size = save_uint64_2_uint8(in64, outbuf);
    for (i = 0; i < size; i++) {
        if (inbuf[i] != outbuf[i]) {
            logw("uint64_2_uint8: index=%d failed. %u!=%u", i, inbuf[i], outbuf[i]);
        }
    }
}
#endif
