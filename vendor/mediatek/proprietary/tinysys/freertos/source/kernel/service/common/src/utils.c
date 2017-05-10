/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
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
 */


#include "FreeRTOS.h"
#include "task.h"
#include "utils.h"

#define USING_CMX_SYSTICK

/* use 32K clock to instead of configCPU_CLOCK_HZ*/
#define configCPU_CLOCK_HZ_TIMESTAMP 32768
#define configCPU_CLOCK_HZ_K         32100 /* tune one tick time*/

static uint64_t ts_b = 0;
static uint64_t ns_per_tick = (1000000000ULL)*configCPU_CLOCK_HZ_K/((configTICK_RATE_HZ)*configCPU_CLOCK_HZ_TIMESTAMP);
#ifdef USING_CMX_SYSTICK
static uint64_t ns_per_cnt = (1000000000ULL)/(uint64_t)configCPU_CLOCK_HZ_TIMESTAMP;
#define portNVIC_SYSTICK_LOAD_REG           ( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG  ( * ( ( volatile uint32_t * ) 0xe000e018 ) )
#endif


typedef void (*initFunc_t)(void);
extern volatile unsigned int __init_array_start;
extern volatile unsigned int __init_array_end;
void section_init_func(void)
{
    volatile unsigned int *fp;
    for(fp = &__init_array_start; fp < &__init_array_end; fp++) {
        ((initFunc_t)*fp)();
    }
}


int timestamp_init(void)
{
    ns_per_tick = (1000000000ULL)*configCPU_CLOCK_HZ_K/((configTICK_RATE_HZ)*configCPU_CLOCK_HZ_TIMESTAMP);
#ifdef USING_CMX_SYSTICK
    ns_per_cnt = (1000000000ULL)/(uint64_t)configCPU_CLOCK_HZ_TIMESTAMP;
#endif
    return 0;
}

/* return timestamp in nano seconds */
uint64_t timestamp_get_ns(void)
{
    uint64_t ts;
    ts = (uint64_t)xTaskGetTickCount();
    //ts *= ns_per_tick;
    ts= ts*((1000000ULL)*configCPU_CLOCK_HZ_K)/configCPU_CLOCK_HZ_TIMESTAMP;

#ifdef USING_CMX_SYSTICK
    ts += (((uint64_t)(portNVIC_SYSTICK_LOAD_REG
                     - portNVIC_SYSTICK_CURRENT_VALUE_REG)) * (1000000000ULL))/configCPU_CLOCK_HZ_TIMESTAMP;
#endif
    /* when second timestamp < first timestamp, return first timestamp */
    if(ts_b > ts)
       ts = ts_b;
    ts_b = ts;
    return ts;
}

uint32_t timestamp_get_info(uint64_t *per_tick, uint64_t *per_cnt)
{
    *per_tick = ns_per_tick;
#ifdef USING_CMX_SYSTICK
    *per_cnt = ns_per_cnt;
#else
    *per_cnt = 0
#endif
    return 0;
}

int timestamp_get_cnt(uint32_t *tick, uint32_t *cnt)
{
    *tick = (uint32_t)xTaskGetTickCount();
#ifdef USING_CMX_SYSTICK
    *cnt = (uint32_t)(portNVIC_SYSTICK_LOAD_REG
                     - portNVIC_SYSTICK_CURRENT_VALUE_REG);
#else
    *cnt = 0
#endif
    return 0;
}

/*
void udelay(uint32_t usec)
{
    uint64_t t0;
    uint64_t nsec = (uint64_t)usec * 1000ULL;
    t0 = timestamp_get_ns();
    while(1) {
        if ((timestamp_get_ns() - t0) >= nsec) {
            break;
        }
    }
}
*/

