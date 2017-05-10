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
#include <task.h>
#include <stdio.h>
#include <platform.h>
#include "etm.h"
#include <scp_ipi.h>
unsigned int testdata = 0;
void ETM_Start(void);
void ETM_Stop(void);
void ETM_Start()
{
    testdata++;
    return;
}
void ETM_Stop()
{
    testdata++;
    return;
}
void etm_init(void)
{
    /*Enable ETM */
    *((unsigned int *) (0x400A00A0)) |= 0x2; //release atb resetn
    *((unsigned int *) (0xE000EDFC)) |= 0x01000000; //set DEMCR (Debug Exception and Monitor Control register) bit 24 to enable trace

    ETM->LAR = CS_UNLOCK;

    // Power Up ETM and initialise registers
    ETM->CR &= ~ETM_CR_PWRDN;
    ETM->CR |= ETM_CR_PROGBIT | ETM_CR_ETMEN | ETM_CR_TSEN;
    ETM->FFLR = 0;              // FifoFull not used
    ETM->CNTRLDVR1 = 0;         // Counter1 not used
    ETM->TSEVR = ETM_EVT_NOTA | ETM_EVT_TRUE;   // timestamp event never
    ETM->TRIGGER = ETM_EVT_A | ETM_EVT_DWT0;    // Trigger on DWT0
    ETM->TEEVR = ETM_EVT_A | ETM_EVT_TRUE;  // Trace Always
    //ETM->TRACEIDR = CM4IKMCU_ETM_ID;    // 7 bit trace ID. Must be valid
    ETM->TRACEIDR = 1;    // 7 bit trace ID. Must be valid

    // At least there is one DWT comparator, use as trigger
    DWT->COMP0 = (uint32_t) &testdata;  // testdata address
    DWT->MASK0 = 0x2;           // Mask(0xFFFFFFFC) for Start address
    DWT->FUNCTION0 = DWT_FUNC_TRIG_WR;  // DWT_ETM_TRIG_WR, Function set to Trigger on data write.

    ETM->TECR1 |= ETM_TECR1_USE_SS; // TraceEnable address range comparators
    ETM->TESSEICR =
        (ETM_TESSEICR_ICE2 << ETM_TESSEICR_STOP) | ETM_TESSEICR_ICE1;
    // initialize the TraceEnable Start/Stop Embedded ICE control register

    // set DWT start/stop address
    DWT->COMP1 = (uint32_t) &ETM_Start; // start address
    DWT->MASK1 = 0x2;           // Mask(0xFFFFFFFC) for Start address
    DWT->COMP2 = (uint32_t) &ETM_Stop;  // stop address
    DWT->MASK2 = 0x2;           // Mask(0xFFFFFFFC) for Stop address
    DWT->FUNCTION1 = DWT_FUNC_TRIG_PC;  // Function set to Trigger on PC match
    DWT->FUNCTION2 = DWT_FUNC_TRIG_PC;  // Function set to Trigger on PC match

    ETM->CR &= ~ETM_CR_PROGBIT;
    PRINTF_D("ETM init done. \n\r trace start address:0x%lx,end address:0x%lx \n\r",DWT->COMP1,DWT->COMP2);
    PRINTF_D("Before Trace enable,ETM Control register:0x%lx, Status:0x%lx\n\r",ETM->CR,ETM->SR);
    //-------------------------------------
    // test 2 - Dump trace
    //-------------------------------------
    //
    // Trace start address has been set to the address of Function Start(),
    // therefore, the ETM trace starts here.
    PRINTF_D("start:%x,end:%x \n\r",DWT->COMP1,DWT->COMP2);
    ETM_Start();
    if (ETM->SR&(0x1<<2))
	PRINTF_D("Trace enable,ETM Control register:0x%lx, Status:0x%lx\n\r",ETM->CR,ETM->SR);
    else
	PRINTF_D("Trace disable,ETM Control register:0x%lx, Status:0x%lx\n\r",ETM->CR,ETM->SR);
    //  // Trace stop address has been set to the address of Function Stop(),
    //  // therefore, the ETM trace stops here.
    //ETM_Stop();

}
void dump_etm_status(void)
{
    if (ETM->SR&(0x1<<2))
	PRINTF_D("Trace enable,ETM Control register:0x%lx, Status:0x%lx\n\r",ETM->CR,ETM->SR);
    else
	PRINTF_D("Trace disable,ETM Control register:0x%lx, Status:0x%lx\n\r",ETM->CR,ETM->SR);
}
