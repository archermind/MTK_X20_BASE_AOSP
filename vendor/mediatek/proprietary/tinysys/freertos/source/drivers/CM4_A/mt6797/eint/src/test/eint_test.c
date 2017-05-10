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

#include <stdio.h>
#include <platform.h>
#include <interrupt.h>
#include <driver_api.h>
#include <utils.h>
#include "FreeRTOS.h"
#include <task.h>
#include <eint.h>
#include <scp_testsuite.h>
#include <ctype.h>
#include <stdlib.h>


#define WRITE_REG(v, a) DRV_WriteReg32(a, v)
#define READ_REG(a) DRV_Reg32(a)
/*global testsuite value*/
SCP_STATUS_T testsuite_return_value;
unsigned int testsuite_return_count;

extern void irq_status_dump(void);
extern char ReadDebugByte(void);
extern void WriteDebugByte(char c);

static int read_line(char *buffer, int len)
{
	int pos = 0;
	char c;
	while ((c = ReadDebugByte()) != '\r'){
	    if (c == 0xff || c == 0x0)
	    {
		continue;
	    }
	    WriteDebugByte(c);

	    buffer[pos++] = c;
		if (c == '\r' || c =='\n')
		{
		    goto done;
		}
		/* end of line. */
		if (pos == (len - 1)) {
			puts("\nerror: line too long\n");
			pos = 0;
			goto done;
		}

	}
done:

	buffer[pos] = 0;
	return pos;
}

static void vEintTestTask(void *pvParameters)
{
	/*intterupt set*/
	int i;
	int j;
	while (1) {
		for(i = 0; i < EINT_MAX_CHANNEL; i++)
		{

			mt_eint_soft_set(i);
			for(j = 0; j < 200000; j++)
			{

			}

		}
	}
}

static void eint_test_callback_func(int eint_idx)
{
    debug_msg("Receive EINT number %d ISR\n\r", eint_idx);
	mt_eint_soft_clr(eint_idx);
	int i;
	   for(i = 0; i < 20000; i++)
    {

    }
	testsuite_return_value = SCP_SUCCESS;
	testsuite_return_count++;
}

static void eint_test_callback_func_testsuite(int eint_idx)
{
    debug_msg("Receive EINT number %d ISR\n\r", eint_idx);
	debug_msg("ERROR in eint_test_callback_func_testsuite ISR\n\r", eint_idx);
	mt_eint_soft_clr(eint_idx);
	testsuite_return_value = SCP_FAIL;

}

SCP_STATUS_T eint_all_test(void *psInput)
{
    SCP_STATUS_T ret = SCP_SUCCESS;
    testsuite_return_value = SCP_SUCCESS;
	testsuite_return_count = 0;

	int i;
	int ms_value = 500; //500ms


	debug_msg("[EINT]===== eint all test start=====\n\r");

    for(i = 0; i < EINT_MAX_CHANNEL; i++)
    {
		debug_msg("[EINT] mt_eint_registration number %d \n\r",i);
		mt_eint_registration(i,LEVEL_SENSITIVE,HIGH_LEVEL_TRIGGER, eint_test_callback_func, EINT_INT_UNMASK, EINT_INT_AUTO_UNMASK_OFF);

    }

	for(i = 0; i < EINT_MAX_CHANNEL; i++)
    {
		debug_msg("[EINT] mt_eint_set_hw_debounce number %d \n\r",i);
		mt_eint_set_hw_debounce(i,ms_value);

    }

	/* verify config settings */
	unsigned int st;
    unsigned int bit;
	unsigned int eint_mask;
	unsigned int eint_sens;
	unsigned int eint_pol;
	unsigned int is_deb_en;
	unsigned int deb_reg_value;

	for(i = 0; i < EINT_MAX_CHANNEL; i++)
    {
		bit = 1 << (i % 32);
		st = READ_REG(((i / 32) * 4 + EINT_MASK_BASE));
		eint_mask = bit & st;
		if( eint_mask > 0)
			eint_mask = 1;
		st = READ_REG(((i / 32) * 4 + EINT_SENS_BASE));
		eint_sens = bit & st;
		if( eint_sens > 0)
			eint_sens = 1;
		st = READ_REG(((i / 32) * 4 + EINT_POL_BASE));
		eint_pol = bit & st;
		if( eint_pol > 0)
			eint_pol = 1;


		deb_reg_value = READ_REG(((i / 4) * 4 + EINT_DBNC_BASE));
		deb_reg_value = deb_reg_value >> ((i % 4) * 8);
		is_deb_en = deb_reg_value & 0x01;
		deb_reg_value = deb_reg_value & 0x70;
		deb_reg_value = deb_reg_value >> 4;


		if(eint_mask != 0){
			debug_msg("[EINT] mask test fail, eint number = %d\n\r",i);
			ret = SCP_FAIL;
			return ret;
		}else if(eint_sens != 1){
			debug_msg("[EINT] sens test fail, eint number = %d\n\r",i);
			ret = SCP_FAIL;
			return ret;
		}else if(eint_pol != 1){
			debug_msg("[EINT] polarity test fail, eint number = %d\n\r",i);
			ret = SCP_FAIL;
			return ret;
		}else if(is_deb_en != 1){
			debug_msg("[EINT] debounce test fail, eint number = %d\n\r",i);
			ret = SCP_FAIL;
			return ret;
		}else if(deb_reg_value != 7){
			debug_msg("[EINT] debounce regs value fail, eint number = %d\n\r",i);
			ret = SCP_FAIL;
			return ret;
		}

	}

	debug_msg("[EINT]===== interrupt test =====\n\r");
	testsuite_return_value = SCP_FAIL;
	testsuite_return_count = 0;
	for(i = 0; i < EINT_MAX_CHANNEL; i++)
    {
		mt_eint_soft_clr(i);
		mt_eint_soft_set(i);
    }


	if(testsuite_return_value == SCP_FAIL || testsuite_return_count != 18){
		debug_msg("[EINT]interrupt test fail, testsuite_return_count = %d\n\r",testsuite_return_count);
		ret = SCP_FAIL;
		return ret;
	}

	debug_msg("[EINT]===== mask test =====\n\r");
	testsuite_return_value = SCP_SUCCESS;
	for(i = 0; i < EINT_MAX_CHANNEL; i++)
    {
		mt_eint_registration(i,LEVEL_SENSITIVE,HIGH_LEVEL_TRIGGER, eint_test_callback_func_testsuite, EINT_INT_MASK, EINT_INT_AUTO_UNMASK_OFF);
		mt_eint_soft_set(i);
    }

	if(testsuite_return_value == SCP_FAIL){
		ret = SCP_FAIL;
		return ret;
	}

	for(i = 0; i < EINT_MAX_CHANNEL; i++)
    {

		mt_eint_soft_set(i);

    }

	/*clear soft interrupt test*/
	for(i = 0; i < EINT_MAX_CHANNEL; i++)
    {
		mt_eint_soft_clr(i);
    }

	/*start task scheduler*/
	debug_msg("[EINT]===== task scheduler test =====\n\r");
	if(testsuite_return_value == SCP_SUCCESS){
		   for(i = 0; i < EINT_MAX_CHANNEL; i++)
		{
			debug_msg("[EINT] mt_eint_registration number %d \n\r",i);
			mt_eint_registration(i,LEVEL_SENSITIVE,HIGH_LEVEL_TRIGGER, eint_test_callback_func, EINT_INT_UNMASK, EINT_INT_AUTO_UNMASK_ON);
		}

		debug_msg("[EINT]===== create FreeRTOS task=====\n\r");
		xTaskCreate(vEintTestTask, "Test", 130, (void *) 3, 2, NULL);
		//xTaskCreate(vEintTestTask, "Test", 130, (void *) 4, 2, NULL);
		//xTaskCreate(vEintTestTask, "Test", 130, (void *) 5, 2, NULL);
		//vTaskStartScheduler();
	}

    return ret;

}

SCP_STATUS_T eint_dump_config_test(void *psInput)
{
	SCP_STATUS_T ret = SCP_SUCCESS;
	int eint_number;
	char buffer[256];
	debug_msg("[EINT]please input eint number:\n\r");
	read_line(buffer, sizeof(buffer));
	eint_number = atoi(buffer);
	debug_msg("[EINT]eint_number = %d\n\r",eint_number);

	mt_eint_dump_config(eint_number);
	return ret;
}

SCP_STATUS_T eint_set_polarity_test(void *psInput)
{
	SCP_STATUS_T ret = SCP_SUCCESS;
	int eint_number;
	int polarity_value;
	char buffer[256];
	char buffer_value[256];
	debug_msg("[EINT]please input eint number:\n\r");
	read_line(buffer, sizeof(buffer));
	eint_number = atoi(buffer);
	debug_msg("[EINT]eint_number = %d\n\r",eint_number);

	debug_msg("[EINT]please input polarity value:\n\r");
	read_line(buffer_value, sizeof(buffer_value));
	polarity_value = atoi(buffer_value);
	debug_msg("[EINT]polarity value = %d\n\r",polarity_value);

	mt_eint_set_polarity(eint_number,polarity_value);

	return ret;
}

SCP_STATUS_T eint_set_sens_test(void *psInput)
{
	SCP_STATUS_T ret = SCP_SUCCESS;
	int eint_number;
	int sens_value;
	char buffer[256];
	char buffer_value[256];
	debug_msg("[EINT]please input eint number:\n\r");
	read_line(buffer, sizeof(buffer));
	eint_number = atoi(buffer);
	debug_msg("[EINT]eint_number = %d\n\r",eint_number);

	debug_msg("[EINT]please input sens value(1: highlevel trigger   0: low lever trigger)\n\r");
	read_line(buffer_value, sizeof(buffer_value));
	sens_value = atoi(buffer_value);
	debug_msg("[EINT]sens value = %d\n\r",sens_value);

	mt_eint_set_sens(eint_number,sens_value);

	return ret;
}

SCP_STATUS_T eint_set_mask_test(void *psInput)
{
	SCP_STATUS_T ret = SCP_SUCCESS;
	int eint_number;
	int mask_value;
	char buffer[256];
	char buffer_value[256];
	debug_msg("[EINT]please input eint number:\n\r");
	read_line(buffer, sizeof(buffer));
	eint_number = atoi(buffer);
	debug_msg("[EINT]eint_number = %d\n\r",eint_number);

	debug_msg("[EINT]please input mask value(1: mask   0: unmask)\n\r");
	read_line(buffer_value, sizeof(buffer_value));
	mask_value = atoi(buffer_value);
	debug_msg("[EINT]mask value = %d\n\r",mask_value);

	if(mask_value == 1)
	{
		mt_eint_mask(eint_number);
	}
	else
	{
		mt_eint_unmask(eint_number);
	}

	return ret;
}

SCP_STATUS_T eint_send_soft_int_test(void *psInput)
{
	SCP_STATUS_T ret = SCP_SUCCESS;
	int eint_number;
	char buffer[256];
	debug_msg("[EINT]please input eint number:\n\r");
	read_line(buffer, sizeof(buffer));
	eint_number = atoi(buffer);
	debug_msg("[EINT]eint_number = %d\n\r",eint_number);

	/*clear soft interrupt */
	int i;
	for(i = 0; i < EINT_MAX_CHANNEL; i++)
    {
		mt_eint_soft_clr(i);
    }

	mt_eint_soft_set(eint_number);

	return ret;
}

SCP_STATUS_T eint_registration_test(void *psInput)
{
	SCP_STATUS_T ret = SCP_SUCCESS;
	int eint_number;
	int polarity_value;
	int sens_value;
	char buffer[256];
	char buffer_value[256];
	char buffer_value2[256];
	debug_msg("[EINT]please input eint number:\n\r");
	read_line(buffer, sizeof(buffer));
	eint_number = atoi(buffer);
	debug_msg("[EINT]eint_number = %d\n\r",eint_number);

	debug_msg("[EINT]please input polarity value:\n\r");
	read_line(buffer_value, sizeof(buffer_value));
	polarity_value = atoi(buffer_value);
	debug_msg("[EINT]polarity value = %d\n\r",polarity_value);

	debug_msg("[EINT]please input sens value(1: highlevel trigger   0: low lever trigger)\n\r");
	read_line(buffer_value2, sizeof(buffer_value2));
	sens_value = atoi(buffer_value2);
	debug_msg("[EINT]sens value = %d\n\r",sens_value);

	mt_eint_registration(eint_number,sens_value,polarity_value, eint_test_callback_func, EINT_INT_UNMASK, EINT_INT_AUTO_UNMASK_OFF);

	return ret;
}


SCP_STATUS_T eint_set_debounce_test(void *psInput)
{
	SCP_STATUS_T ret = SCP_SUCCESS;
	int eint_number;
	int debounce_value;
	int ms_value;
	char buffer[256];
	char buffer_value[256];
	char buffer_value2[256];
	debug_msg("[EINT]please input eint number:\n\r");
	read_line(buffer, sizeof(buffer));
	eint_number = atoi(buffer);
	debug_msg("[EINT]eint_number = %d\n\r",eint_number);

	debug_msg("[EINT]please input debounce value:\n\r");
	read_line(buffer_value, sizeof(buffer_value));
	debounce_value = atoi(buffer_value);
	debug_msg("[EINT]debounce value = %d\n\r",debounce_value);

	debug_msg("[EINT]please input debounce ms(0~512)\n\r");
	read_line(buffer_value2, sizeof(buffer_value2));
	ms_value = atoi(buffer_value2);
	debug_msg("[EINT]ms value = %d\n\r",ms_value);


	if(debounce_value == 1)
	{
		mt_eint_set_hw_debounce(eint_number,ms_value);
	}
	else
	{
		mt_eint_dis_hw_debounce(eint_number);
	}


	return ret;
}


SCP_STATUS_T eint_misc_test(void *psInput)
{
	SCP_STATUS_T ret = SCP_SUCCESS;
	mt_eint_dump_all_config();
	return ret;
}
