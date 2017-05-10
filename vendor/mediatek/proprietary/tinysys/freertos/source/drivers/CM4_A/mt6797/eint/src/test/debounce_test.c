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

#include "common.h"
#include "api.h"
#include "intrCtrl.h"
#include "eint.h"
#include "gpio.h"


static int gEint_Count;
static unsigned int gEint_Triggered;
#ifdef __SLT__
#define DEBOUNCE_SETTING_NUMBER     (6)
static const unsigned int gDBNC_Setting_Table[DEBOUNCE_SETTING_NUMBER] = {0, 1, 16, 32, 64, 128};

#else
#define DEBOUNCE_SETTING_NUMBER     (8)
static const unsigned int gDBNC_Setting_Table[DEBOUNCE_SETTING_NUMBER] = {0, 1, 16, 32, 64, 128, 256, 512};
#endif

static void eint_hw_debounce_test_callback(int eint_idx)
{
    must_print("%s, Receive EINT %d\n", __func__, eint_idx);
    gEint_Count++;
    gEint_Triggered = eint_idx;
    mt_eint_mask(eint_idx);
}


int eint_hw_debounce_low_level_test(void)
{
    unsigned int status;
    int idx = 1;
    int ret = 0;
    unsigned int timeout;
    /*Eint 1, gpio 50*/
    unsigned long gpio_pin = GPIO50;
    unsigned long gpio_mode = GPIO_MODE_02;
    unsigned int cycle_cnt, cycle_cnt_prev, dbnc_idx;
#ifdef MTKDRV_DVFS
    int cpu_freq = CPU_freq_output();
#endif
    int level_trigger[2] = {LOW_LEVEL_TRIGGER, HIGH_LEVEL_TRIGGER};
    int lv;

    dbg_print("\nEINT %u - GPIO %d, MODE %d\n", idx, gpio_pin, gpio_mode);

    for(lv = 0; lv < 2; lv++)
    {
        mt_eint_dis_hw_debounce(idx);

        mt_eint_mask_all();
        mt_eint_mask(idx);
        mt_eint_domain0_set(idx);

        mt_eint_set_sens(idx, LEVEL_SENSITIVE);
        //mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_hw_debounce_test_callback, 0, 0);
        mt_eint_registration(idx, 0, level_trigger[lv], eint_hw_debounce_test_callback, 0, 0);
        dbg_print("===== Testing %s trigger ===== \n",
                  level_trigger[lv] == HIGH_LEVEL_TRIGGER ? "high" : "low");

        mt_set_gpio_mode(gpio_pin, gpio_mode);
        mt_set_gpio_dir(gpio_pin, GPIO_DIR_IN);
        mt_set_gpio_inversion(gpio_pin, GPIO_DATA_UNINV);

        if(mt_get_gpio_in(gpio_pin) != 0)
        {
            must_print("EINT %d, GPIO IN is %d, Must in Low for testing!\n", idx);
            ret = -1;
            continue;
        }

        for(dbnc_idx = 0; dbnc_idx < DEBOUNCE_SETTING_NUMBER; dbnc_idx++){
            unsigned long gpio_inv_en = lv == LOW_LEVEL_TRIGGER ? GPIO_DATA_INV : GPIO_DATA_UNINV;

            //mt_set_gpio_inversion(gpio_pin, GPIO_DATA_UNINV);
            mt_set_gpio_inversion(gpio_pin, gpio_inv_en);

#if 1
            /* Set one EINT to trigger EINT interrupt */
            mt_eint_set_hw_debounce(idx, gDBNC_Setting_Table[dbnc_idx]);

            //reset counter
            *(volatile unsigned int *)0x1000B600 = (0x2) << (idx * 8);
            dsb();
            CTP_Wait_msec(1);


            mt_eint_mask(idx); /*because mt_eint_set_hw_debounce unmask eint, we need to mask it again*/

            CTP_Wait_msec(10);
            mt_eint_ack(idx);
            mt_eint_mask(idx);
#else
            mt_eint_mask(idx);
            mt_eint_ack(idx);

            /*set up eint

              EINT0~3, low level trigger debounce的register settings:

              AP_EINT_SENS_SET (0x1000B180) = 0x0000000f
              AP_EINT_POL_CLR (0x1000B380) = 0x0000000f
              AP_EINT_DBNC_SET_3_0 (0x1000B600) = 0x01010101    //enable debounce 0.5ms
              //wait for 3*32k (~100us)
              AP_EINT_DBNC_SET_3_0 (0x1000B600) = 0x03030303   //reset debounce
              //wait for 3*32k (~100us)
              AP_EINT_INTACK (0x1000B040) = 0x0000000f
              AP_EINT_MASK_CLR (0x1000B100) = 0x0000000f
            */

            *(volatile unsigned int *)0x1000B180 = (0x1 << idx);

            if(level_trigger[lv] == HIGH_LEVEL_TRIGGER)
                *(volatile unsigned int *)0x1000B340 = (0x1 << idx); /* AP_EINT_POL_SET */
            else
                *(volatile unsigned int *)0x1000B380 = (0x1 << idx); /* AP_EINT_POL_CLR */
            dsb();

            //clear reg
            *(volatile unsigned int *)0x1000B700 = (0xFF) << (idx * 8);
            dsb();
            CTP_Wait_msec(1);

            //set decounce value
            *(volatile unsigned int *)0x1000B600 = (0x1 | (dbnc_idx << 4)) << (idx * 8);
            dsb();
            CTP_Wait_msec(1);

            dbg_print("Enable debounce\n");
            status = mt_eint_get_status(idx);

            dbg_print("EINT Module - index:%d,EINT_STA = 0x%x\n", idx, status);
#if 1
            //reset counter
            *(volatile unsigned int *)0x1000B600 = (0x2) << (idx * 8);
            dsb();
            CTP_Wait_msec(1);
#endif

            *(volatile unsigned int *) 0x1000B040 = (0x1 << idx);
            dsb();
            //*(volatile unsigned int *) 0x1000B100 = (0x1 << idx);

            dbg_print("Reset debounce\n");

            status = mt_eint_get_status(idx);

#endif

            gEint_Count = 0;
            gEint_Triggered = 0xFFFFFFFF;
            dsb();
#if 0
            mt_eint_unmask(idx);
            mt_eint_ack(idx);
            mt_eint_mask(idx);
#endif
            cycle_count_start();
            mt_eint_unmask(idx);
            mt_set_gpio_inversion(gpio_pin, !gpio_inv_en);

            timeout = 0xFFFFFFFF;
            do{
                timeout--;
                if(!(timeout % 0x100000))
                {
                    must_print("0x1000B500=0x%x\n", *(volatile unsigned int *)0x1000B500);
                    must_print("EINT %d GPIO in is %d\n", idx, mt_get_gpio_in(gpio_pin));
                }
            }while(gEint_Count==0);

            cycle_cnt = cycle_count_end();
            mt_eint_mask(idx);

            if(gEint_Count == 0){
                must_print("EINT %d was not be triggerd\n", idx);
                ret = -1;
            }
            else
            {
#ifdef MTKDRV_DVFS
                unsigned int max_us;
                unsigned int exe_us = cycle_cnt / cpu_freq;
                if(gDBNC_Setting_Table[dbnc_idx] == 0)
                {
                    max_us = 1500;
                }
                else
                {
                    max_us = gDBNC_Setting_Table[dbnc_idx] * 2 * 1000;
                }


                dbg_print("Check Debounce: %d ms\n", gDBNC_Setting_Table[dbnc_idx]);
                dbg_print("time %d.%d ms\n", exe_us/1000, exe_us%1000);
                if(exe_us > max_us)
                {
                    must_print("EINT debounce %u work abnormally\n", idx);
                    ret = -1;
                }
            }
#endif
            CTP_Wait_msec(10);

        }
        mt_eint_dis_hw_debounce(idx);
    }
    return ret;
}



int eint_hw_debounce_test(void){
    unsigned int idx, gpio_idx, dbnc_idx, timeout;
    int ret = 0;
    unsigned int i;
    unsigned int cycle_cnt, cycle_cnt_prev;
    unsigned int gpio_inv[EINT_MAX_CHANNEL];
#ifdef MTKDRV_DVFS
    int cpu_freq = CPU_freq_output();
#endif

#ifdef __SLT__
    /* slt only test eint 0 */
    for(idx = 0; idx < 1; idx++)
#else
        for(idx = 0; idx < EINT_MAX_CHANNEL; idx++)
#endif
        {
            gpio_idx = 0;
            if(eint_gpio_line[idx][gpio_idx] == GPIO_UNSUPPORTED && eint_gpio_line[idx][gpio_idx] == GPIO_MODE_UNSUPPORTED){
                continue;
            }

            cycle_cnt_prev = 0;
            dbg_print("\nEINT %u - GPIO %d, MODE %d\n", idx, eint_gpio_line[idx][gpio_idx], eint_gpio_mode[idx][gpio_idx]);
            /* Set one EINT to trigger EINT interrupt */
            mt_eint_dis_hw_debounce(idx);

            mt_eint_mask_all();
            mt_set_gpio_mode(eint_gpio_line[idx][gpio_idx], eint_gpio_mode[idx][gpio_idx]);
            mt_set_gpio_dir(eint_gpio_line[idx][gpio_idx], GPIO_DIR_IN);
            mt_set_gpio_inversion(eint_gpio_line[idx][gpio_idx], GPIO_DATA_UNINV);

            mt_eint_set_sens(idx, LEVEL_SENSITIVE);
            mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_hw_debounce_test_callback, 0, 0);
            mt_eint_set_polarity(idx, HIGH_LEVEL_TRIGGER);

            mt_eint_ack(idx);
            mt_eint_mask(idx);
            mt_eint_domain0_set(idx);

            gEint_Count = 0;
            gEint_Triggered = 0xFFFFFFFF;
            dsb();

            /* get set correct gpio setting first */
            gpio_inv[idx] = GPIO_DATA_INV;


            dbg_print("Assert EINT %d\n", idx);

            mt_set_gpio_inversion(eint_gpio_line[idx][gpio_idx], gpio_inv[idx]);

            mt_eint_unmask(idx);

            timeout = 0x00001000;
            do{
                CTP_Wait_msec(1);
                timeout--;
                if(!(timeout % 10))
                {
                    /* try reverse signal */
                    gpio_inv[idx] = (gpio_inv[idx] == GPIO_DATA_INV) ? GPIO_DATA_UNINV : GPIO_DATA_INV;
                    mt_set_gpio_inversion(eint_gpio_line[idx][gpio_idx], gpio_inv[idx]);
                }
            }while(gEint_Count == 0 && timeout);

            mt_eint_mask(idx);

            if(gEint_Count == 0){
                must_print("EINT %d was not be triggered\n", idx);
                ret = -1;
            }

            else if(gEint_Triggered != idx){
                must_print("Unexpected %d EINT was triggered\n", gEint_Triggered);
                ret = -1;
            }
            else
            {
                must_print("EINT %d was triggered\n", gEint_Triggered);
            }

            for(dbnc_idx = 0; dbnc_idx < DEBOUNCE_SETTING_NUMBER; dbnc_idx++){
                /* Set one EINT to trigger EINT interrupt */
                mt_eint_mask_all();

                mt_set_gpio_mode(eint_gpio_line[idx][gpio_idx], eint_gpio_mode[idx][gpio_idx]);
                mt_set_gpio_dir(eint_gpio_line[idx][gpio_idx], GPIO_DIR_IN);
                mt_set_gpio_inversion(eint_gpio_line[idx][gpio_idx], GPIO_DATA_UNINV);

                mt_eint_set_sens(idx, LEVEL_SENSITIVE);
                mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_hw_debounce_test_callback, 0, 0);
                mt_eint_set_polarity(idx, HIGH_LEVEL_TRIGGER);

                mt_eint_ack(idx);
                mt_eint_mask(idx);
                mt_eint_domain0_set(idx);

                gEint_Count = 0;
                gEint_Triggered = 0xFFFFFFFF;
                dsb();

                mt_eint_set_hw_debounce(idx, gDBNC_Setting_Table[dbnc_idx]);
                mt_eint_mask(idx); /*because mt_eint_set_hw_debounce unmask eint, we need to mask it again*/
                mt_eint_ack(idx);

                dbg_print("Assert EINT %d\n", idx);

                mt_set_gpio_inversion(eint_gpio_line[idx][0], gpio_inv[idx]);

                cycle_count_start();
                mt_eint_unmask(idx);

                timeout = 0xFFFFFFFF;
                do{
                    timeout--;
                }while(gEint_Count == 0);

                cycle_cnt = cycle_count_end();
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
                    must_print("EINT %d was not be triggerd\n", idx);
                    ret = -1;
                }

                else if(gEint_Triggered != idx){
                    must_print("Unexpected %d EINT was triggerd\n", gEint_Triggered);
                    ret = -1;
                }

                else
                {
#ifdef MTKDRV_DVFS
                    unsigned int max_us;
                    unsigned int exe_us = cycle_cnt / cpu_freq;
                    if(gDBNC_Setting_Table[dbnc_idx] == 0)
                    {
                        max_us = 1500;
                    }
                    else
                    {
                        max_us = gDBNC_Setting_Table[dbnc_idx] * 2 * 1000;
                    }


                    dbg_print("Check Debounce: %d ms\n", gDBNC_Setting_Table[dbnc_idx]);
                    dbg_print("time %d.%d ms\n", exe_us/1000, exe_us%1000);
                    if(exe_us > max_us)
                    {
                        must_print("EINT debounce %u work abnormally\n", idx);
                        ret = -1;
                    }
#endif

                }


                if(cycle_cnt < cycle_cnt_prev && dbnc_idx > 1){
                    must_print("EINT debounce %u work abnormally: cycle count cur = %u, prev = %u\n", gDBNC_Setting_Table[dbnc_idx], cycle_cnt, cycle_cnt_prev);
                    ret = -1;
                }

                cycle_cnt_prev = cycle_cnt;
                CTP_Wait_msec(5);

            }

            mt_eint_dis_hw_debounce(idx);
        }

    return ret;
}
