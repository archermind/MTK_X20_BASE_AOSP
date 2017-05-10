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
#include <stdint.h>
#include <stdio.h>
#include <arch_helpers.h>
#include "typedefs.h"
#include "platform.h"
#include "mt_cpuxgpt.h"

#define CPUXGPT_BASE 	0x10220000
#define INDEX_BASE  	(CPUXGPT_BASE+0x0674)
#define CTL_BASE    	(CPUXGPT_BASE+0x0670)

#define GPT_BIT_MASK_L 0x00000000FFFFFFFF
#define GPT_BIT_MASK_H 0xFFFFFFFF00000000

__u64	normal_time_base;
__u64	atf_time_base;

#define MT_CPUXGPT_ENABLE
#ifdef MT_CPUXGPT_ENABLE

/*need add volatile keyword*/
static inline uint64_t mt_read_cntpct_el0(void)
{			
	uint64_t v;					
	__asm__ __volatile__("mrs %0,  cntpct_el0" : "=r" (v));	
	return v;					
}

static unsigned int __read_cpuxgpt(unsigned int reg_index )
{
  	unsigned int value = 0;
  	DRV_WriteReg32(INDEX_BASE,reg_index);
 
  	value = DRV_Reg32(CTL_BASE);
  	return value;
}

static void __write_cpuxgpt(unsigned int reg_index,unsigned int value )
{

  	DRV_WriteReg32(INDEX_BASE,reg_index);
  	DRV_WriteReg32(CTL_BASE,value);
}

static void __cpuxgpt_set_init_cnt(unsigned int countH,unsigned int  countL)
{
#if 0
   	__write_cpuxgpt(INDEX_CNT_H_INIT,countH);
   	__write_cpuxgpt(INDEX_CNT_L_INIT,countL); // update count when countL programmed
#endif
}

static void set_cpuxgpt_clk(unsigned int div)
{
	unsigned int tmp = 0;
	tmp = __read_cpuxgpt(INDEX_CTL_REG);
	tmp &= CLK_DIV_MASK;
	tmp |= div;
	__write_cpuxgpt(INDEX_CTL_REG, tmp);

}

static void enable_cpuxgpt(void)
{
	unsigned int tmp = 0;
	tmp = __read_cpuxgpt(INDEX_CTL_REG);
	tmp |= EN_CPUXGPT;
	__write_cpuxgpt(INDEX_CTL_REG, tmp);

	printf("CPUxGPT reg(%x)\n", __read_cpuxgpt(INDEX_CTL_REG));
}

/*
unsigned long long read_cntpct_cpuxgpt(void)
{
	unsigned int cnt[2] = {0, 0};
	unsigned long long counter = 0;
	
	cnt[0] = __read_cpuxgpt(0x74);
	cnt[1] = __read_cpuxgpt(0x78);
	counter = (GPT_BIT_MASK_H &(((unsigned long long) (cnt[1])) << 32)) | (GPT_BIT_MASK_L&((unsigned long long) (cnt[0])));

	return counter;
}
*/

void __delay(unsigned long long cycles)
{
	//volatile unsigned long long i = 0;
	//unsigned long long start = read_cntpct_cpuxgpt();
	unsigned long long start = mt_read_cntpct_el0();
	/*
	do {
		i = read_cntpct_cpuxgpt();
		printf("CPUxGPT pct(%llx)\n", i);
	} while ((i - start) < cycles);
	*/	
	#if 1
	//printf("CPUxGPT (%llx)\n", cycles);
	while ((mt_read_cntpct_el0() - start) < cycles) {
		//printf("CPUxGPT pct(%llx)\n", mt_read_cntpct_el0());
	}
	#endif
}

void udelay(unsigned long us)
{
	unsigned long long loops;

	if(us<2000) {
		loops = us * 13; /*arch timer's freq is 13MHz*/
		__delay(loops);
	}
}

void mdelay(unsigned long ms)
{
	unsigned long loops;

	loops = ms;
	while(loops != 0) {
		udelay(1000);
		loops--;
	}
}

void setup_syscnt(void) 
{
   //set cpuxgpt free run,cpuxgpt always free run & oneshot no need to set
   //set cpuxgpt 13Mhz clock
   set_cpuxgpt_clk(CLK_DIV2);
   // enable cpuxgpt
   enable_cpuxgpt();
}

#else

static void __cpuxgpt_set_init_cnt(unsigned int countH,unsigned int  countL) {}

void setup_syscnt(void) {}

void udelay(unsigned long us) {}

void mdelay(unsigned long ms) {}

#endif

void generic_timer_backup(void)
{
	__u64 cval;

	cval = read_cntpct_el0();
	__cpuxgpt_set_init_cnt((__u32)(cval >> 32), (__u32)(cval & 0xffffffff));
}

void atf_sched_clock_init(unsigned long long normal_base, unsigned long long atf_base)
{
	normal_time_base = normal_base;
	atf_time_base = atf_base;
	return;
}

unsigned long long atf_sched_clock(void)
{
	__u64 cval;
	
	cval = (((read_cntpct_el0() - atf_time_base)*1000)/13) + normal_time_base; 
	return cval;
}

/*
  Return: 0 - Trying to disable the CPUXGPT control bit, and not allowed to disable it.
  Return: 1 - reg_addr is not realted to disable the control bit.
*/
unsigned char check_cpuxgpt_write_permission(unsigned int reg_addr, unsigned int reg_value)
{
	unsigned idx;
	unsigned ctl_val;

	if (reg_addr == CTL_BASE) {
		idx = DRV_Reg32(INDEX_BASE);

		/* idx 0: CPUXGPT system control */
		if (idx == 0) {
			ctl_val = DRV_Reg32(CTL_BASE);
			if (ctl_val & 1) {
				/* if enable bit already set, then bit 0 is not allow to set as 0 */
				if (!(reg_value & 1))
					return 0;
			}
		}
	}
	return 1;
}
