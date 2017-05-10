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

#include "CTP_type.h"
#include "CTP_shell.h"
#include "mt_spm.h"
#include "sys_cirq.h"
#include "eint.h"
#include "intrCtrl.h"
#include "common.h"
#include "api.h"
#include "gpio.h"


#include "mt_spm.h"

static unsigned int gSYS_CIRQ_Count;
static unsigned int gSYS_CIRQ_Triggered;


/*
 * Enable IRQs
 */
static inline void arch_local_irq_enable(void)
{
	unsigned long temp;
	asm volatile(
		"	mrs	%0, cpsr	@ arch_local_irq_enable\n"
		"	bic	%0, %0, #128\n"
		"	msr	cpsr_c, %0"
		: "=r" (temp)
		:
		: "memory", "cc");
}

/*
 * Disable IRQs
 */
static inline void arch_local_irq_disable(void)
{
	unsigned long temp;
	asm volatile(
		"	mrs	%0, cpsr	@ arch_local_irq_disable\n"
		"	orr	%0, %0, #128\n"
		"	msr	cpsr_c, %0"
		: "=r" (temp)
		:
		: "memory", "cc");
}


static int gEint_Count;
static unsigned int gEint_Triggered;

static void eint_gpio_test_callback(int eint_idx)
{
    must_print("Receive EINT %d\n", eint_idx);
    gEint_Count++;
    gEint_Triggered = eint_idx;
    mt_eint_mask(eint_idx);
}

static void eint_test_wakeup_src_callback(int eint_idx)
{
    /*Eint 1, gpio 50*/
    unsigned long gpio_pin = GPIO50;
    unsigned long gpio_mode = GPIO_MODE_02;
    unsigned int cpu_id;
	__asm__ __volatile__ ("MRC   p15, 0, %0, c0, c0, 5" :"=r"(cpu_id) );

    cpu_id &= 0xf;

    dbg_print("CPU%d Wakeup Src Receive EINT %d\n", cpu_id, eint_idx);

    mt_set_gpio_inversion(gpio_pin, GPIO_DATA_INV_DEFAULT);
    mt_set_gpio_mode(gpio_pin, GPIO_MODE_00);

    mt_gic_cfg_irq2cpu(EINT_IRQ_ID, 1, 0); /* clear eint irq sent to cpu 1 */
    mt_gic_cfg_irq2cpu(EINT_IRQ_ID, 0, 1); /* Set eint irq sent to cpu 0 */

}

int eint_gpio_connection_test(void){
    unsigned int idx, gpio_idx, timeout;
    int ret = 0;
    char azInput[32];
    for(idx = 0; idx < EINT_MAX_CHANNEL; idx++){

        for(gpio_idx = 0; gpio_idx < EINT_GPIO_MAXNUMBER; gpio_idx++){
            if(eint_gpio_line[idx][gpio_idx] == GPIO_UNSUPPORTED || eint_gpio_line[idx][gpio_idx] == GPIO_MODE_UNSUPPORTED)
                break;

            dbg_print("\nEINT %u - GPIO %d, MODE %d\n", idx, eint_gpio_line[idx][gpio_idx], eint_gpio_mode[idx][gpio_idx]);
            dbg_print("IES = %d\n", mt_get_gpio_ies(eint_gpio_line[idx][gpio_idx]));

            /* Set one EINT to trigger EINT interrupt */
            mt_eint_mask_all();

            mt_set_gpio_mode(eint_gpio_line[idx][gpio_idx], eint_gpio_mode[idx][gpio_idx]);
            mt_set_gpio_dir(eint_gpio_line[idx][gpio_idx], GPIO_DIR_IN);
            mt_set_gpio_inversion(eint_gpio_line[idx][gpio_idx], GPIO_DATA_UNINV);

            mt_eint_set_sens(idx, LEVEL_SENSITIVE);
            mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_gpio_test_callback, 0, 0);
            mt_eint_set_polarity(idx, HIGH_LEVEL_TRIGGER);

            mt_eint_ack(idx);
            mt_eint_mask(idx);
            mt_eint_domain0_set(idx);

            gEint_Count = 0;
            gEint_Triggered = 0xFFFFFFFF;
            dsb();

            dbg_print("Assert EINT %d\n", idx);

            mt_set_gpio_inversion(eint_gpio_line[idx][gpio_idx], GPIO_DATA_INV);

            mt_eint_unmask(idx);


            timeout = 0x00001000;
	        do{
                CTP_Wait_msec(1);
		        timeout--;
                if(!(timeout % 10))
                {
                    /* try reverse signal */
                    unsigned long inv;
                    inv = mt_get_gpio_inversion(eint_gpio_line[idx][gpio_idx]) == GPIO_DATA_INV
                        ? GPIO_DATA_UNINV : GPIO_DATA_INV;
                    mt_set_gpio_inversion(eint_gpio_line[idx][gpio_idx], inv);
                }
	        }while(gEint_Count == 0 && timeout);

            mt_eint_mask(idx);

            mt_set_gpio_inversion(eint_gpio_line[idx][gpio_idx], GPIO_DATA_INV_DEFAULT);
            mt_set_gpio_mode(eint_gpio_line[idx][gpio_idx], GPIO_MODE_00);

            if(eint_gpio_line[idx][gpio_idx] == GPIO93 ||
               eint_gpio_line[idx][gpio_idx] == GPIO94) // uart 2
            {
                GPIO_SetPinMode(GPIO93, GPIO_MODE_01);		//set URXD2
                GPIO_SetPinMode(GPIO94, GPIO_MODE_01);		//set UTXD2
                GPIO_SetPinDir(GPIO93, GPIO_DIR_IN);		//set input
                GPIO_SetPinDir(GPIO94, GPIO_DIR_OUT);		//set output
                GPIO_SetPinIES(GPIO93, GPIO_IES_ENABLE);		//set IES=1
                GPIO_SetPinIES(GPIO94, GPIO_IES_ENABLE);		//set IES=1
                GPIO_SetPinPullEnable(GPIO93, GPIO_PULL_ENABLE);	//set pellen=1
                GPIO_SetPinPullSelect(GPIO93, GPIO_PULL_UP);	//set pullsel=1
            }

            if(gEint_Count == 0){
                must_print("EINT %d was not be triggered\n", idx);
                ret = -1;
                //CTP_GetUserCommand (azInput);

                mt_set_gpio_mode(eint_gpio_line[idx][gpio_idx], GPIO_MODE_00);
                mt_set_gpio_dir(eint_gpio_line[idx][gpio_idx], GPIO_DIR_IN);
                mt_set_gpio_inversion(eint_gpio_line[idx][gpio_idx], GPIO_DATA_INV);

                dbg_print("\nChecking EINT %u - GPIO %d\n", idx, eint_gpio_line[idx][gpio_idx]);
                dbg_print("MODE = %d\n", mt_get_gpio_mode(eint_gpio_line[idx][gpio_idx]));
                dbg_print("DIR  = %d\n", mt_get_gpio_dir(eint_gpio_line[idx][gpio_idx]));
                dbg_print("IES  = %d\n", mt_get_gpio_ies(eint_gpio_line[idx][gpio_idx]));

                CTP_Wait_msec(100);

                dbg_print("get GPIO_DATA_INV data = %d\n", mt_get_gpio_out(eint_gpio_line[idx][gpio_idx]));
                mt_set_gpio_inversion(eint_gpio_line[idx][gpio_idx], GPIO_DATA_UNINV);

                CTP_Wait_msec(100);
                must_print("get GPIO_DATA_UNINV data = %d\n", mt_get_gpio_out(eint_gpio_line[idx][gpio_idx]));

            }

            else if(gEint_Triggered != idx){
                must_print("Unexpected %d EINT was triggered\n", gEint_Triggered);
                ret = -1;
            }
            else
            {
                must_print("EINT %d was triggered\n", gEint_Triggered);
            }

            CTP_Wait_msec(100);
        }
	}

    return ret;
}

static void wakeup_src_sys_cirq_test_lisr(void)
{
    unsigned int index;
    unsigned int intr_num;
    unsigned int status;
    unsigned int status_check;
    unsigned long gpio_pin = GPIO50;
    unsigned long gpio_mode = GPIO_MODE_02;

    dbg_print("Get CIRQ event_b interrupt\n");

    for(index = 0; index < SYS_CIRQ_MAX_CHANNEL; index++)
    {
        /* read status register every 32 interrupts */
        if(!(index % 32))
        {
            status = mt_syscirq_get_status_reg((index / 32));
            //dbg_print("SYS_CIRQ Module - SYS_CIRQ_STA%d = 0x%x\n", (index / 32), status);
        }

        status_check = status & (1 << (index % 32));

        if(status_check)
        {
            gSYS_CIRQ_Count++;
            gSYS_CIRQ_Triggered = index;

            dbg_print("SYS_CIRQ Module - SYS_CIRQ %d is actived\n", index);

            mt_syscirq_ack(index);

            intr_num = index + GIC_SPI_START + SYS_CIRQ_SPI_START;

            dbg_print("Interrupt %d is actived\n", intr_num);

            switch(intr_num){
            case EINT_DIRECT1_IRQ_ID:
                dbg_print("Get EINT_DIRECT1_IRQ_ID\n");
                break;
            case EINT0_IRQ_ID:
                dbg_print("Get EINT0_IRQ_ID\n");
                break;
            }
        }
    }

    mt_eint_mask(1);//test eint 1
    mt_set_gpio_inversion(gpio_pin, GPIO_DATA_INV_DEFAULT);
    mt_set_gpio_mode(gpio_pin, GPIO_MODE_00);

    IRQMask(SYS_CIRQ_EVENT_IRQ_ID);

    return;
}


int wakeup_src_event_b_setup_only(void)
{

    int ret = 0;
    unsigned long gpio_pin = GPIO50;
    unsigned long gpio_mode = GPIO_MODE_02;

    unsigned int idx;
    for(idx = GIC_SPI_START; idx < (GIC_SPI_START + NUM_SPI_SOURCES); ++idx){
        IRQMask(idx);
    }

    idx = 1; /* use eint 1*/

    /* Set one EINT to trigger EINT interrupt */
    mt_eint_mask_all();


    mt_set_gpio_mode(gpio_pin, gpio_mode);
    mt_set_gpio_dir(gpio_pin, GPIO_DIR_IN);
    mt_set_gpio_inversion(gpio_pin, GPIO_DATA_UNINV);


    mt_eint_set_sens(idx, LEVEL_SENSITIVE);
    mt_eint_set_polarity(idx, HIGH_LEVEL_TRIGGER);
    mt_eint_ack(idx);
    mt_eint_unmask(idx);
    mt_eint_domain0_set(idx);


    /* Set and register SYS_CIRQ_EVENT_IRQ on GIC */
    IRQSensitivity(SYS_CIRQ_EVENT_IRQ_ID, LEVEL_SENSITIVE);
    IRQPolarity(SYS_CIRQ_EVENT_IRQ_ID, LOW_LEVEL_TRIGGER);
    IRQ_Register_LISR(SYS_CIRQ_EVENT_IRQ_ID, wakeup_src_sys_cirq_test_lisr, "SYS_CIRQ");
    IRQUnmask(SYS_CIRQ_EVENT_IRQ_ID);

    /* Set all interrupts on SYS_CIRQ */
    mt_syscirq_mask_all();
    mt_syscirq_set_pol_inactive_all();
    mt_syscirq_set_sens_edge_all();
    mt_syscirq_ack_all();
    mt_syscirq_unmask_all();

    mt_syscirq_enable();

    mt_syscirq_mask(EINT_EVENT_IRQ_ID - GIC_SPI_START - SYS_CIRQ_SPI_START);

    delay_a_while(2000);


    gSYS_CIRQ_Count = 0;
    gSYS_CIRQ_Triggered = 0xFFFFFFFF;

    mt_syscirq_unmask(EINT_IRQ_ID - GIC_SPI_START - SYS_CIRQ_SPI_START);
#if 0
    while(!gSYS_CIRQ_Count)
    {
        must_print("EINT %d in is %d\n", idx, mt_get_gpio_in(gpio_pin));
        delay_a_while(10000);
    }


    if(gSYS_CIRQ_Count == 0){
        must_print("sys_cirq_event_b was not triggered\n");
        ret = -1;
    }
    else if(gSYS_CIRQ_Count != 1 && gSYS_CIRQ_Triggered != (EINT_IRQ_ID - GIC_SPI_START - SYS_CIRQ_SPI_START)){
        must_print("Unexpected interrupt was triggered: count = %d, sys_cirq_id = %d\n", gSYS_CIRQ_Count, gSYS_CIRQ_Triggered);
        ret = -1;
    }
    else{
        dbg_print("sys_cirq_event_b was triggered by edge-sensitive interrupt successfully\n");
    }
#endif
    return ret;
}

int wakeup_src_eint_gpio_setup_only(void)
{
    int idx = 1;
    int ret = 0;
    unsigned int timeout;
    /*Eint 1, gpio 50*/
    unsigned long gpio_pin = GPIO50;
    unsigned long gpio_mode = GPIO_MODE_02;

    mt_eint_mask_all();
    mt_eint_mask(idx);
    mt_eint_set_sens(idx, LEVEL_SENSITIVE);
    mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_test_wakeup_src_callback, 0, 0);

    mt_set_gpio_mode(gpio_pin, gpio_mode);
    mt_set_gpio_dir(gpio_pin, GPIO_DIR_IN);
    mt_set_gpio_inversion(gpio_pin, GPIO_DATA_UNINV);
    mt_eint_set_polarity(idx, HIGH_LEVEL_TRIGGER);

    must_print("EINT %d set high level trigger\n", idx);

    gEint_Count = 0;
    gEint_Triggered = 0xFFFFFFFF;
    dsb();

    mt_eint_unmask(idx);


    must_print("EINT %d in is %d\n", idx, mt_get_gpio_in(gpio_pin));
    must_print("!!! Remember set input high to trigger interrupt\n");

    return ret;
}


int wakeup_src_cpu2_only(void)
{
    int idx = 1;
    int ret = 0;
    unsigned int timeout;
    /*Eint 1, gpio 50*/
    unsigned long gpio_pin = GPIO50;
    unsigned long gpio_mode = GPIO_MODE_02;

    bootup_slave_cpu();

    mt_eint_mask_all();
    mt_eint_mask(idx);
    mt_eint_set_sens(idx, LEVEL_SENSITIVE);
    mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_test_wakeup_src_callback, 0, 0);

    mt_set_gpio_mode(gpio_pin, gpio_mode);
    mt_set_gpio_dir(gpio_pin, GPIO_DIR_IN);
    mt_set_gpio_inversion(gpio_pin, GPIO_DATA_UNINV);
    mt_eint_set_polarity(idx, HIGH_LEVEL_TRIGGER);

    /* set up to slave cpu */
    must_print("Setup EINT Sent to CPU 1\n", idx);
    mt_gic_cfg_irq2cpu(EINT_IRQ_ID, 0, 0); /* clear eint irq sent to cpu 0 */
    mt_gic_cfg_irq2cpu(EINT_IRQ_ID, 1, 1); /* Set eint irq sent to cpu 1 */


    must_print("EINT %d set high level trigger\n", idx);

    gEint_Count = 0;
    gEint_Triggered = 0xFFFFFFFF;
    dsb();

    mt_eint_unmask(idx);


    must_print("EINT %d in is %d\n", idx, mt_get_gpio_in(gpio_pin));
    must_print("!!! Remember set input high to trigger interrupt\n");

    return ret;
}


int eint_gpio_wakeup_src_test(void)
{
    int idx = 1;
    int ret = 0;
    unsigned int timeout;
    /*Eint 1, gpio 50*/
    unsigned long gpio_pin = GPIO50;
    unsigned long gpio_mode = GPIO_MODE_02;

    mt_eint_mask_all();

    mt_eint_mask(idx);
    mt_eint_set_sens(idx, LEVEL_SENSITIVE);
    mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_test_wakeup_src_callback, 0, 0);

    mt_set_gpio_mode(gpio_pin, gpio_mode);
    mt_set_gpio_dir(gpio_pin, GPIO_DIR_IN);
    mt_set_gpio_inversion(gpio_pin, GPIO_DATA_UNINV);
    mt_eint_set_polarity(idx, HIGH_LEVEL_TRIGGER);

    must_print("EINT %d set high level trigger\n", idx);

    gEint_Count = 0;
    gEint_Triggered = 0xFFFFFFFF;
    dsb();

    mt_eint_unmask(idx);


    must_print("EINT %d in is %d\n", idx, mt_get_gpio_in(gpio_pin));
    must_print("!!! Remember set input high to trigger interrupt\n");


    if(setup_eint_wakeup_src() < 0)
    {
        ret = -1;
        must_print("Wakeup Fail\n");
    }

    mt_eint_mask(idx);
    mt_set_gpio_inversion(gpio_pin, GPIO_DATA_INV_DEFAULT);
    mt_set_gpio_mode(gpio_pin, GPIO_MODE_00);
    return ret;
}


int setup_eint_wakeup_src(void)
{
    unsigned int wakesrc = WAKE_ID_EINT;
    unsigned int mask;
    int ret = 0;
    wake_status_t* wake_status;

    const char *result;

    mask = spm_wakesrc_mask_all(SPM_PCM_KERNEL_SUSPEND);

    if(!spm_wakesrc_set(SPM_PCM_KERNEL_SUSPEND,wakesrc))
    {
        dbg_print("Invalid wakesrc: %d\r\n",wakesrc);
        return -1;
    }

    dbg_print("wakesrc: %d\r\n",wakesrc);

    dbg_print("System is going to sleep immediately. Please trigger the wakeup source NOW!!\r\n");

    power_off_cpu1();

    arch_local_irq_disable();

    spm_go_to_sleep();//Power off AP MCU

    power_on_cpu1();

    InitDebugSerial();//Init UART

    arch_local_irq_enable();

    /*  Trigger your wakeup event here by Hand */

    result = spm_get_wake_up_result(SPM_PCM_KERNEL_SUSPEND);
    wake_status = spm_get_last_wakeup_status();

    dbg_print("%s",result);

    if((wake_status->wakeup_event &(1U<<wakesrc)) && wake_status->wake_reason == WR_WAKE_SRC)
        ret = 0;
    else
        ret = -1;

    spm_wakesrc_restore(SPM_PCM_KERNEL_SUSPEND,mask);
    return ret;
}

static void eint_gpio_one_pin_test_call_back(int eint_idx)
{
    unsigned long gpio_pin = GPIO50;
    unsigned long gpio_mode = GPIO_MODE_02;

    dbg_print("=== IN %s ===\n", __func__);
    dbg_print("eint_idx = %d\n", eint_idx);
    mt_eint_mask(eint_idx);
    mt_set_gpio_inversion(gpio_pin, GPIO_DATA_INV_DEFAULT);
    mt_set_gpio_mode(gpio_pin, GPIO_MODE_00);

    gSYS_CIRQ_Count++;
    return;
}

int eint_gpio_one_pin_test(void)
{

    int ret = 0;
    unsigned timeout;
    unsigned long gpio_pin = GPIO50;
    unsigned long gpio_mode = GPIO_MODE_02;

    unsigned int idx;

    idx = 1; /* use eint 1*/

    /* Set one EINT to trigger EINT interrupt */
    mt_eint_mask_all();

    mt_set_gpio_mode(gpio_pin, gpio_mode);
    mt_set_gpio_dir(gpio_pin, GPIO_DIR_IN);
    mt_set_gpio_inversion(gpio_pin, GPIO_DATA_UNINV);


    mt_eint_set_sens(idx, LEVEL_SENSITIVE);
    mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_gpio_one_pin_test_call_back, 0, 0);

    mt_eint_set_polarity(idx, HIGH_LEVEL_TRIGGER);

    mt_eint_ack(idx);
    mt_eint_mask(idx);
    mt_eint_domain0_set(idx);

    gSYS_CIRQ_Count = 0;
    timeout = 0x1000;


    mt_eint_unmask(idx);
    dbg_print("Assert EINT %d\n", idx);
    mt_set_gpio_inversion(gpio_pin, GPIO_DATA_INV);

    while(!gSYS_CIRQ_Count && timeout)
    {
        CTP_Wait_usec(100);
        timeout--;
    }

    if(gSYS_CIRQ_Count)
    {
        must_print("EINT %d was triggered\n", idx);
    }
    else
    {
        must_print("EINT %d wat not triggered\n", idx);
        ret = -1;

    }
    return ret;
}
