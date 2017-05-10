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

#include <FreeRTOS.h>
#include <stdio.h>
#include <string.h>
#include <platform.h>
#include <driver_api.h>
#include <task.h>


#include <core_cm4.h>
#include <dwt.h>

#define DWT_DEBUG   1


#if(DWT_DEBUG == 1)
#define dbgmsg PRINTF_D
#else
#define dbgmsg(...)
#endif
extern UBaseType_t uxTaskGetEndOfStack(TaskHandle_t xTask);



typedef struct {
    int enable;
    uint32_t comp_val;
    uint32_t mask;
    uint32_t mask_size;
    DWT_FUNC_TYPE func;
} dwt_comp_setting_t;

static dwt_comp_setting_t dwt_comp[4];
static int DWT_NUMCOMP;


/* reset all comparators' setting */
static void dwt_reset(void)
{
    DWT->MASK0 = 0;
    DWT->MASK1 = 0;
    DWT->MASK2 = 0;
    DWT->MASK3 = 0;
    DWT->COMP0 = 0;
    DWT->COMP1 = 0;
    DWT->COMP2 = 0;
    DWT->COMP3 = 0;
    DWT->FUNCTION0 &= !DWT_FUNCTION_FUNCTION_Msk;
    DWT->FUNCTION1 &= !DWT_FUNCTION_FUNCTION_Msk;
    DWT->FUNCTION2 &= !DWT_FUNCTION_FUNCTION_Msk;
    DWT->FUNCTION3 &= !DWT_FUNCTION_FUNCTION_Msk;
}

void dwt_init(void)
{
    /* enable debug monitor mode    */
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_MON_EN_Msk)) {
        CoreDebug->DEMCR |= (CoreDebug_DEMCR_MON_EN_Msk | CoreDebug_DEMCR_TRCENA_Msk) ;
    }

    DWT_NUMCOMP = DWT->CTRL >> DWT_CTRL_NUMCOMP_Pos;

    /*
    The maximum mask size is IMPLEMENTATION DEFINED. A debugger can write 0b11111 to this
    field and then read the register back to determine the maximum mask size supported.
    */

    DWT->MASK0 |= 0x1f;
    DWT->MASK1 |= 0x1f;
    DWT->MASK2 |= 0x1f;
    DWT->MASK3 |= 0x1f;

    dwt_comp[0].mask_size = DWT->MASK0;
    dwt_comp[1].mask_size = DWT->MASK1;
    dwt_comp[2].mask_size = DWT->MASK2;
    dwt_comp[3].mask_size = DWT->MASK3;

    dbgmsg("DWT has %d comparators, those supported mask size are: [0]%lx [1]%lx [2]%lx [3]%lx \n\r",
           DWT_NUMCOMP, dwt_comp[0].mask_size, dwt_comp[1].mask_size, dwt_comp[2].mask_size, dwt_comp[3].mask_size);

    dwt_reset();
    PRINTF_I("Init DWT OK! \n\r");

    return;
}

void dump_dwt_status(void)
{
    PRINTF_D("DHCSR:0x%lx, DEMCR:0x%lx \n\r", CoreDebug->DHCSR, CoreDebug->DEMCR);

    PRINTF_I("DWT_CTRL: 0x%lx \n\r", DWT->CTRL);
    PRINTF_I("COMP0: %8lx \t MASK0: %8lx \t FUNC0: %8lx \n\r", DWT->COMP0, DWT->MASK0, DWT->FUNCTION0);
    PRINTF_I("COMP1: %8lx \t MASK1: %8lx \t FUNC1: %8lx \n\r", DWT->COMP1, DWT->MASK1, DWT->FUNCTION1);
    PRINTF_I("COMP2: %8lx \t MASK2: %8lx \t FUNC2: %8lx \n\r", DWT->COMP2, DWT->MASK2, DWT->FUNCTION2);
    PRINTF_I("COMP3: %8lx \t MASK3: %8lx \t FUNC3: %8lx \n\r", DWT->COMP3, DWT->MASK3, DWT->FUNCTION3);

    return;
}


/*
@param addr_base:   address for data accesses or instruction fetches
@param addr_mask: the size of the ignore mask applied to address range matching
@param func:        which kind of compared accesses will generate watchpoint debug event
@return val:        compator N for address comparison functions

!! Note: the addr_base should be 2^(addr_mask) byte alignment, otherwise the behavior is UNPREDICTABLE !!

*/
int request_dwt_watchpoint(uint32_t addr_base, uint32_t addr_mask, DWT_FUNC_TYPE func)
{
    int index;
    if (addr_base & addr_mask) {
        PRINTF_W("Fail: The address %lx is not 2^%d alignment \n\r", addr_base, (int)addr_mask);
        return -1;
    }
    for (index = 0; index < DWT_NUMCOMP; index++) {
        if (!dwt_comp[index].enable) {
            break;
        }
    }

    if (index >= DWT_NUMCOMP) {
        PRINTF_W("Fail: All DWT compators are already in used \n\r");
        return -1;
    }

    dwt_comp[index].comp_val = addr_base;
    dwt_comp[index].mask = (addr_mask > dwt_comp[index].mask_size) ? dwt_comp[index].mask_size : addr_mask;
    dwt_comp[index].func = func;

    return index;
}


/*
@param index:           compator N
*/
int enable_dwt_comp(int index)
{
    uint32_t offset;

    if (index > DWT_NUMCOMP) {
        printf("DWT do NOT support comparator[%d]! \n\r ", index);
        return -1;
    }

    offset = (0x10 * index) / 4; // pointer size = 4 bytes
    *(&DWT->COMP0 + offset) = dwt_comp[index].comp_val;
    *(&DWT->MASK0 + offset) = dwt_comp[index].mask;
    *(&DWT->FUNCTION0 + offset) = (*(&DWT->FUNCTION0 + offset) & DWT_FUNCTION_FUNCTION_Msk) | dwt_comp[index].func;
    dwt_comp[index].enable = 1;

    return 0;
}


/*
@param index:           compator N
*/
int disable_dwt_comp(int index)
{
    uint32_t offset;

    if (index > DWT_NUMCOMP) {
        printf("DWT do NOT support comparator[%d]! \n\r ", index);
        return -1;
    }

    offset = (0x10 * index) / 4; // pointer size = 4 bytes
    if (*(&DWT->FUNCTION0 + offset)) {
        *(&DWT->FUNCTION0 + offset) &= !DWT_FUNCTION_FUNCTION_Msk;
        dwt_comp[index].enable = 0;
    } else {
        PRINTF_W("DWT comparator[%d] is not enabled! \n\r ", index);
    }

    dbgmsg("COMPATOR %d is disabled \n\r", index);

    return 0;
}


int32_t stack_protector(TaskHandle_t task_handler)
{
    uint32_t stack_end_address;
    uint32_t compator;

    stack_end_address = uxTaskGetEndOfStack(task_handler);
    PRINTF_D("stack address:0x%x\n", stack_end_address);
    compator = request_dwt_watchpoint(stack_end_address, 0x2, WDE_DATA_WO);
    if (compator >= 0) {
        printf("Request COMPATOR %d \n\r", compator);
        enable_dwt_comp(compator);
#if 0
        dump_dwt_status();
#endif
    } else {
        PRINTF_W("Request COMPATOR Fail! \n\r");
    }

    return 0;
}
