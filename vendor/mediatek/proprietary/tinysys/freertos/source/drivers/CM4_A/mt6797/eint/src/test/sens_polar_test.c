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
#include <arch/irq.h>
#include <platform/mt_reg_base.h>
#include <platform/idle.h>
#include <platform/mt_typedefs.h>
#include <kernel/thread.h>
#include <platform/timer.h>
#include <platform/mt_gpt.h>
#include <debug.h>
#include <eint.h>

static int gEint_Count;
static unsigned int gEint_Triggered;

static void eint_test_callback(int eint_idx)
{
    printf("Receive EINT %d\n", eint_idx);
    gEint_Count++;
    gEint_Triggered = eint_idx;
}

int eint_level_pos_pulse_test(unsigned int idx){
    unsigned int timeout;


    //mdelay(500);
    /* for(idx = 0; idx < EINT_MAX_CHANNEL; idx++) */
    {
		mt_eint_set_sens(idx, LEVEL_SENSITIVE);
		mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_test_callback, 0, 0);

        gEint_Count = 0;
        gEint_Triggered = 0xFFFFFFFF;
        //dsb();

        printf("Unmask EINT %d\n", idx);
        mt_eint_unmask(idx);

        printf("Send a pulse to EINT %d\n", idx);
        mt_eint_send_pulse(idx);

        timeout = 1000;
	    do{
		    timeout--;
	    }while(gEint_Count == 0 && timeout != 0);

        if(gEint_Count != 0){
            printf("%s(): Unexpected EINT %d was triggerd\n", "eint_level_pos_pulse_test", gEint_Triggered);
            return -1;
        }

        mt_eint_mask(idx);


	}

    return 0;
}


int eint_level_pos_level_test(unsigned int idx){
    unsigned int timeout;


    //mdelay(500);
    /* for(idx = 0; idx < EINT_MAX_CHANNEL; idx++) */
    {
		mt_eint_set_sens(idx, LEVEL_SENSITIVE);
		mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_test_callback, 0, 0);

        gEint_Count = 0;
        gEint_Triggered = 0xFFFFFFFF;
        //dsb();

        printf("Unmask EINT %d\n", idx);
        mt_eint_unmask(idx);

        printf("Assert EINT %d\n", idx);
        mt_eint_soft_set(idx);

        timeout = 1000;
	    do{
		    timeout--;
            mt_eint_unmask(idx);
	    }while(gEint_Count != 10 && timeout != 0);

        if(gEint_Count < 10){
            printf("%s(): EINT %d was not be triggerd repeatly: count = %d\n", "eint_level_pos_level_test", idx, gEint_Count);
            mt_eint_soft_clr(idx);
            return -1;
        }

        if(gEint_Triggered != idx){
            printf("%s(): Unexpected EINT %d was triggerd\n", "eint_level_pos_level_test", gEint_Triggered);
            mt_eint_soft_clr(idx);
            return -1;
        }

        mt_eint_soft_clr(idx);
	}

    return 0;
}

int eint_edge_pos_pulse_test(unsigned int idx){
    unsigned int timeout;


    //mdelay(500);

    /* for(idx = 0; idx < EINT_MAX_CHANNEL; idx++) */
    {
		mt_eint_set_sens(idx, EDGE_SENSITIVE);
		mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_test_callback, 0, 1);

        gEint_Count = 0;
        gEint_Triggered = 0xFFFFFFFF;
        //dsb();

        printf("Unmask EINT %d\n", idx);
        mt_eint_unmask(idx);

        printf("Send a pulse to EINT %d\n", idx);
        mt_eint_send_pulse(idx);

        timeout = 1000;
	    do{
		    timeout--;
	    }while(gEint_Count == 0 && timeout != 0);

        if(gEint_Count == 0){
            printf("%s(): EINT %d was not be triggerd\n", "eint_edge_pos_pulse_test", idx);
            mt_eint_mask(idx);
            return -1;
        }

        timeout = 1000;
	    do{
		    timeout--;
	    }while(timeout != 0);

        if(gEint_Count > 1 || gEint_Triggered != idx){
            printf("%s(): Unexpected EINT was triggerd: count = %d, number = %d\n", "eint_edge_pos_pulse_test", gEint_Count, gEint_Triggered);
            mt_eint_mask(idx);
            return -1;
        }

        mt_eint_mask(idx);
	}

    return 0;
}

int eint_edge_pos_level_test(unsigned int idx){
    unsigned int timeout;

    //mdelay(500);

    /* for(idx = 0; idx < EINT_MAX_CHANNEL; idx++) */
    {
		mt_eint_set_sens(idx, EDGE_SENSITIVE);
		mt_eint_registration(idx, 0, HIGH_LEVEL_TRIGGER, eint_test_callback, 0, 1);

        gEint_Count = 0;
        gEint_Triggered = 0xFFFFFFFF;
        //dsb();

        printf("Unmask EINT %d\n", idx);
        mt_eint_unmask(idx);

        printf("Assert EINT %d\n", idx);
        mt_eint_soft_set(idx);

        timeout = 1000;
	    do{
		    timeout--;
	    }while(gEint_Count == 0 && timeout != 0);

        if(gEint_Count == 0){
            printf("%s(): EINT %d was not be triggerd\n", "eint_edge_pos_level_test", idx);
            mt_eint_soft_clr(idx);
            mt_eint_mask(idx);
            return -1;
        }

        timeout = 1000;
	    do{
		    timeout--;
	    }while(timeout != 0);

        if(gEint_Count > 1 || gEint_Triggered != idx){
            printf("%s(): Unexpected EINT was triggerd: count = %d, number = %d\n", "eint_edge_pos_level_test", gEint_Count, gEint_Triggered);
            mt_eint_soft_clr(idx);
            mt_eint_mask(idx);
            return -1;
        }

        mt_eint_soft_clr(idx);
        mt_eint_mask(idx);
	}

    return 0;
}

int eint_level_neg_pulse_test(unsigned int idx){
    unsigned int timeout;


    //mdelay(500);


    /* for(idx = 0; idx < EINT_MAX_CHANNEL; idx++) */
    {
		mt_eint_set_sens(idx, LEVEL_SENSITIVE);
		mt_eint_registration(idx, 0, LOW_LEVEL_TRIGGER, eint_test_callback, 0, 0);

        gEint_Count = 0;
        gEint_Triggered = 0xFFFFFFFF;
        //dsb();

        printf("Unmask EINT %d\n", idx);
        mt_eint_unmask(idx);

        printf("Send a pulse to EINT %d\n", idx);
        mt_eint_send_pulse(idx);

        timeout = 1000;
	    do{
		    timeout--;
	    }while(gEint_Count == 0 && timeout != 0);

        if(gEint_Count != 0){
            printf("%s(): Unexpected EINT %d was triggerd\n", "eint_level_neg_pulse_test", gEint_Triggered);
            return -1;
        }

        mt_eint_mask(idx);
	}

    return 0;
}

int eint_level_neg_level_test(unsigned int idx){
    unsigned int timeout;


    //mdelay(500);
    /* for(idx = 0; idx < EINT_MAX_CHANNEL; idx++) */
    {
		mt_eint_set_sens(idx, LEVEL_SENSITIVE);
		mt_eint_registration(idx, 0, LOW_LEVEL_TRIGGER, eint_test_callback, 0, 0);

        gEint_Count = 0;
        gEint_Triggered = 0xFFFFFFFF;
        //dsb();

        printf("Unmask EINT %d\n", idx);
        mt_eint_unmask(idx);

        printf("Assert EINT %d\n", idx);
        mt_eint_soft_set(idx);

        timeout = 1000;
	    do{
		    timeout--;
            mt_eint_unmask(idx);
	    }while(gEint_Count != 10 && timeout != 0);

        if(gEint_Count < 10){
            printf("%s(): EINT %d was not be triggerd repeatly: count = %d\n", "eint_level_neg_level_test", idx, gEint_Count);
            mt_eint_soft_clr(idx);
            return -1;
        }

        if(gEint_Triggered != idx){
            printf("%s(): Unexpected EINT %d was triggerd\n", "eint_level_neg_level_test", gEint_Triggered);
            mt_eint_soft_clr(idx);
            return -1;
        }

        mt_eint_soft_clr(idx);
	}

    return 0;
}

int eint_edge_neg_pulse_test(unsigned int idx){
    unsigned int timeout;


    //mdelay(500);
    /* for(idx = 0; idx < EINT_MAX_CHANNEL; idx++) */
    {
		mt_eint_set_sens(idx, EDGE_SENSITIVE);
		mt_eint_registration(idx, 0, LOW_LEVEL_TRIGGER, eint_test_callback, 0, 1);

        gEint_Count = 0;
        gEint_Triggered = 0xFFFFFFFF;
        //dsb();

        printf("Unmask EINT %d\n", idx);
        mt_eint_unmask(idx);

        printf("Send a pulse to EINT %d\n", idx);
        mt_eint_send_pulse(idx);

        timeout = 1000;
	    do{
		    timeout--;
	    }while(gEint_Count == 0 && timeout != 0);

        if(gEint_Count == 0){
            printf("%s(): EINT %d was not be triggerd\n", "eint_edge_neg_pulse_test", idx);
            mt_eint_mask(idx);
            return -1;
        }

        timeout = 1000;
	    do{
		    timeout--;
	    }while(timeout != 0);

        if(gEint_Count > 1 || gEint_Triggered != idx){
            printf("%s(): Unexpected EINT was triggerd: count = %d, number = %d\n", "eint_edge_neg_pulse_test", gEint_Count, gEint_Triggered);
            mt_eint_mask(idx);
            return -1;
        }

        mt_eint_mask(idx);
	}

    return 0;
}

int eint_edge_neg_level_test(unsigned int idx){
    unsigned int timeout;


    //mdelay(500);
    /* for(idx = 0; idx < EINT_MAX_CHANNEL; idx++) */
    {
		mt_eint_set_sens(idx, EDGE_SENSITIVE);
		mt_eint_registration(idx, 0, LOW_LEVEL_TRIGGER, eint_test_callback, 0, 1);

        gEint_Count = 0;
        gEint_Triggered = 0xFFFFFFFF;
        //dsb();

        printf("Unmask EINT %d\n", idx);
        mt_eint_unmask(idx);

        printf("Assert EINT %d\n", idx);
        mt_eint_soft_set(idx);

        timeout = 1000;
	    do{
		    timeout--;
	    }while(gEint_Count == 0 && timeout != 0);

        if(gEint_Count == 0){
            printf("%s(): EINT %d was not be triggerd\n", "eint_edge_neg_level_test", idx);
            mt_eint_soft_clr(idx);
            mt_eint_mask(idx);
            return -1;
        }

        timeout = 1000;
	    do{
		    timeout--;
	    }while(timeout != 0);

        if(gEint_Count > 1 || gEint_Triggered != idx){
            printf("%s(): Unexpected EINT was triggerd: count = %d, number = %d\n", "eint_edge_neg_level_test", gEint_Count, gEint_Triggered);
            mt_eint_soft_clr(idx);
            mt_eint_mask(idx);
            return -1;
        }

        mt_eint_soft_clr(idx);
        mt_eint_mask(idx);
	}

    return 0;
}

int eint_level_pulse_test(void){
    unsigned int idx;
    int ret;

    for(idx = 0; idx < EINT_MAX_CHANNEL; idx++){
        ret = 0;
        ret += eint_level_pos_pulse_test(idx);
        ret += eint_level_neg_pulse_test(idx);
        if(!ret){
            printf("eint_level_pulse_test: fail on EINT %d\n", idx);
            return -1;
        }
	}

    return 0;
}

int eint_level_level_test(void){
    unsigned int idx;
    int ret;

    for(idx = 0; idx < EINT_MAX_CHANNEL; idx++){
        ret = 0;
        ret += eint_level_pos_level_test(idx);
        ret += eint_level_neg_level_test(idx);
        if(ret < 0){
            printf("eint_level_level_test: fail on EINT %d\n", idx);
            return -1;
        }
	}

    return 0;
}

int eint_edge_pulse_test(void){
    unsigned int idx;
    int ret;

    for(idx = 0; idx < EINT_MAX_CHANNEL; idx++){
        ret = 0;
        ret += eint_edge_pos_pulse_test(idx);
        ret += eint_edge_neg_pulse_test(idx);
        if(ret == -2){
            printf("eint_edge_pulse_test: fail on EINT %d\n", idx);
            return -1;
        }
	}

    return 0;
}

int eint_edge_level_test(void){
    unsigned int idx;
    int ret;

    for(idx = 0; idx < EINT_MAX_CHANNEL; idx++){
        ret = 0;
        ret += eint_edge_pos_level_test(idx);
        ret += eint_edge_neg_level_test(idx);
        if(ret == -2){
            printf("eint_edge_level_test: fail on EINT %d\n", idx);
            return -1;
        }
	}

    return 0;
}
