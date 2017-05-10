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

/******************************************************************************
*
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2001
*
*******************************************************************************/

#ifndef _MT_CPUXGPT_H_
#define _MT_CPUXGPT_H_


typedef enum cpuxgpt_num {
	CPUXGPT0=0,
	CPUXGPT1,
	CPUXGPT2,
	CPUXGPT3,
	CPUXGPT4,
	CPUXGPT5,
	CPUXGPT6,
	CPUXGPT7,
	CPUXGPTNUMBERS,
}CPUXGPT_NUM;

#define CPUXGPT0_IRQID 96
#define CPUXGPT1_IRQID 97
#define CPUXGPT2_IRQID 98
#define CPUXGPT3_IRQID 99
#define CPUXGPT4_IRQID 100
#define CPUXGPT5_IRQID 101
#define CPUXGPT6_IRQID 102
#define CPUXGPT7_IRQID 103

#define CPUXGPT_IRQID_BASE CPUXGPT0_IRQID

//REG
#define INDEX_CTL_REG  0x000
#define INDEX_STA_REG  0x004
#define INDEX_CNT_L_INIT    0x008
#define INDEX_CNT_H_INIT    0x00C
#define INDEX_IRQ_MASK    0x030 //0~7 bit mask cnt0~cnt7 interrupt

#define INDEX_CMP_BASE  0x034


//CTL_REG SET
#define EN_CPUXGPT 0x01
#define EN_AHLT_DEBUG 0x02
//#define CLK_DIV1  (0b001 << 8)
//#define CLK_DIV2  (0b010 << 8)
//#define CLK_DIV4  (0b100 << 8)
#define CLK_DIV1  (0x1 << 8)
#define CLK_DIV2  (0x2 << 8)
#define CLK_DIV4  (0x4 << 8)
#define CLK_DIV_MASK (~(0x7<<8))

#define CPUX_GPT0_ACK            (1<<0x0)
#define CPUX_GPT1_ACK            (1<<0x1)
#define CPUX_GPT2_ACK            (1<<0x2)
#define CPUX_GPT3_ACK            (1<<0x3)
#define CPUX_GPT4_ACK            (1<<0x4)
#define CPUX_GPT5_ACK            (1<<0x5)
#define CPUX_GPT6_ACK            (1<<0x6)
#define CPUX_GPT7_ACK            (1<<0x7)


void generic_timer_backup(void);
void atf_sched_clock_init(unsigned long long normal_base, unsigned long long atf_base);
unsigned long long atf_sched_clock(void);
void setup_syscnt(void);
void udelay(unsigned long us);
void mdelay(unsigned long ms);
unsigned char check_cpuxgpt_write_permission(unsigned int reg_addr, unsigned int reg_value);

#endif
