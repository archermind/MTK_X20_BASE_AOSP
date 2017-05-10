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

#include <wdt.h>
#include <driver_api.h>
#include <mt_reg_base.h>
#include <FreeRTOS.h> /* log */
#include <tinysys_config.h> /* MTK config */
#include <utils.h>
#include <interrupt.h> /*for debug*/
#include <string.h>

#define SHARESEC     __attribute__ ((section (".share")))
static unsigned long long wdt_time_old = 0;
static unsigned long long wdt_time_new = 0;
static unsigned int times = 0;
static unsigned int wdt_timeout_flag = 0;

unsigned long long wdt_time_old_b SHARESEC;
unsigned long long wdt_time_new_b SHARESEC;
unsigned int wdt_times_b SHARESEC;
unsigned int wdt_timeout_flag_b SHARESEC;

void kick_timer_isr(void)
{
    mtk_wdt_restart();
    return;
}

void mtk_wdt_disable(void)
{
    DRV_WriteReg32(WDT_CFGREG, DISABLE_WDT);
}
void mtk_wdt_enable(void)
{
    DRV_WriteReg32(WDT_CFGREG, START_WDT);
}

int mtk_wdt_set_time_out_value(unsigned int value)
{
    if (value > 0xFFFFF) {
        PRINTF_D("SCP WDT Timeout value overflow\n");
        return -1;
    }
    mtk_wdt_disable();
    DRV_WriteReg32(WDT_CFGREG, 1 << WDT_EN | value);
    return 0;
}

void mtk_wdt_restart(void)
{
    DRV_WriteReg32(WDT_KICKREG, KICK_WDT);
}


struct irq_desc_t irq_desc_debug[IRQ_MAX_CHANNEL];
static unsigned long long wdt_time_new_debug = 0;
static unsigned long long wdt_time_old_debug = 0;
static unsigned int times_debug = 0;

void mtk_wdt_restart_interval(unsigned long long interval)
{
    times = times + 1;
    if (((wdt_time_new - wdt_time_old) >= 30000000000)) {
        /*store some debug information*/
        wdt_time_new_debug = wdt_time_new;
        wdt_time_old_debug = wdt_time_old;
        times_debug = times;
        memcpy(&irq_desc_debug, &irq_desc, sizeof(irq_desc));
        wdt_timeout_flag = 1;
    }
    if (((wdt_time_new - wdt_time_old) > interval) && (times > 20)) {
        //PRINTF_D("timer irq >1ms++++++++++++++++++, %lld\n", (wdt_time_new-wdt_time_old));
        DRV_WriteReg32(WDT_KICKREG, KICK_WDT);
        wdt_time_old = wdt_time_new;
        times = 0;
    }
    wdt_time_new = timestamp_get_ns();
    wdt_times_b = times;
    wdt_time_old_b = wdt_time_old;
    wdt_time_new_b = wdt_time_new;
    wdt_timeout_flag_b = wdt_timeout_flag;
}

void mtk_wdt_init(void)
{
    DRV_WriteReg32(WDT_CFGREG, DISABLE_WDT);
    DRV_WriteReg32(WDT_CFGREG, START_WDT);
    PRINTF_D("SCP mtk_wdt_init: WDT_CFGREG=0x%x \n", DRV_Reg32(WDT_CFGREG));
}
