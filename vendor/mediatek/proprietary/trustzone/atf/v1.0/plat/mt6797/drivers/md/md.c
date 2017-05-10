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
 * MediaTek Inc. (C) 2014. All rights reserved.
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

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <runtime_svc.h>
#include <debug.h>
#include <sip_svc.h>
#include <sip_error.h>
#include <platform.h>
#include <mmio.h>
#include <console.h> //set_uart_flag(), clear_uart_flag();
#include "plat_private.h"   //for atf_arg_t_ptr
#include "mt_cpuxgpt.h"
#include <emi_drv.h>


#define LTE_OP_ZERO_2_TWO     (0x00000)
#define LTE_OP_THREE_2_FIVE   (0x00001)
#define LTE_OP_SIX_SEVEN      (0x00002)

#define C2K_OP_ZERO_2_TWO     (0x10000)
#define C2K_OP_THREE_2_FIVE   (0x10001)
#define C2K_OP_SIX_SEVEN      (0x10002)

#define LTE_SBC_LOCK_VAL (0x0000659E)
#define C2K_SBC_LOCK_VAL (0x0000C275)

/*******************************************************************************
 * SMC Call for register write (for LTE C2K public key hash)
 ******************************************************************************/

uint64_t sip_write_md_regs(uint32_t cmd_type, uint32_t val1,uint32_t val2, uint32_t val3)
{
    // cmd_type parsing
    switch (cmd_type){
	case LTE_OP_ZERO_2_TWO:
		mmio_write_32(LTE_SBC_PUBK_HASH0, val1);
		mmio_write_32(LTE_SBC_PUBK_HASH1, val2);
		mmio_write_32(LTE_SBC_PUBK_HASH2, val3);
		printf("Write addr  %0x, val %0x \t addr  %0x, val %0x \t addr  %0x, val %0x \n ",LTE_SBC_PUBK_HASH0, val1,LTE_SBC_PUBK_HASH1, val2,LTE_SBC_PUBK_HASH2, val3);
	break;

	case LTE_OP_THREE_2_FIVE:
		mmio_write_32(LTE_SBC_PUBK_HASH3, val1);
		mmio_write_32(LTE_SBC_PUBK_HASH4, val2);
		mmio_write_32(LTE_SBC_PUBK_HASH5, val3);
	break;
	case LTE_OP_SIX_SEVEN:
		mmio_write_32(LTE_SBC_PUBK_HASH6, val1);
		mmio_write_32(LTE_SBC_PUBK_HASH7, val2);
		 // lock
		mmio_write_32(LTE_SBC_LOCK, LTE_SBC_LOCK_VAL);
	break;

	case C2K_OP_ZERO_2_TWO:
		mmio_write_32(C2K_SBC_PUBK_HASH0, val1);
		mmio_write_32(C2K_SBC_PUBK_HASH1, val2);
		mmio_write_32(C2K_SBC_PUBK_HASH2, val3);
		printf("Write addr  %0x, val %0x \t addr  %0x, val %0x \t addr  %0x, val %0x \n ",C2K_SBC_PUBK_HASH0, val1,C2K_SBC_PUBK_HASH1, val2,C2K_SBC_PUBK_HASH2, val3);
	break;

	case C2K_OP_THREE_2_FIVE:
		mmio_write_32(C2K_SBC_PUBK_HASH3, val1);
		mmio_write_32(C2K_SBC_PUBK_HASH4, val2);
		mmio_write_32(C2K_SBC_PUBK_HASH5, val3);
	break;

	case C2K_OP_SIX_SEVEN:
		mmio_write_32(C2K_SBC_PUBK_HASH6, val1);
		mmio_write_32(C2K_SBC_PUBK_HASH7, val2);
		 // lock
		mmio_write_32(C2K_SBC_LOCK, C2K_SBC_LOCK_VAL);
	break;

	default:
		return SIP_SVC_E_NOT_SUPPORTED;
    }

    uint32_t test_val = 0;
    switch (cmd_type){
	case LTE_OP_SIX_SEVEN:
		mmio_write_32(LTE_SBC_PUBK_HASH7, test_val);
		dsb();
		if (test_val == mmio_read_32(LTE_SBC_PUBK_HASH7))
			return SIP_SVC_E_LOCK_FAIL;
		printf("LTE lock test pass \n");
	break;

	case C2K_OP_SIX_SEVEN:
		mmio_write_32(C2K_SBC_PUBK_HASH7, test_val);
		dsb();
		if (test_val == mmio_read_32(C2K_SBC_PUBK_HASH7))
			return SIP_SVC_E_LOCK_FAIL;
		printf("C2K lock test pass \n");
	break;

	default:
		return SIP_SVC_E_NOT_SUPPORTED;

    }

    return SIP_SVC_E_SUCCESS;
}


