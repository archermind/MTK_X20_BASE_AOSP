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
#include "eint.h"
#include "intrCtrl.h"

int eint_mask_set_clear_test(void){
	unsigned int i;
    unsigned int sta;

	for (i = 0; i < EINT_AP_MAXNUMBER; i++) {
        sta = mt_eint_read_mask(i);
        mt_eint_unmask(i);

        mt_eint_mask(i);
        if(!mt_eint_read_mask(i))
        {
            MSG("EINT %d cannot be masked!\r\n", i);
            return -1;
        }

        mt_eint_unmask(i);
        if(mt_eint_read_mask(i))
        {
            MSG("EINT %d cannot be unmasked!\r\n", i);
            return -1;
        }

        if(sta){
            mt_eint_mask(i);
        }
        else{
            mt_eint_unmask(i);
        }
	}

    return 0;
}

int eint_sensitivity_set_clear_test(void){
	unsigned int i;
    unsigned int sta;

	for (i = 0; i < EINT_AP_MAXNUMBER; i++) {
        sta = mt_eint_read_sens(i);
        mt_eint_set_sens(i, EDGE_SENSITIVE);

        mt_eint_set_sens(i, LEVEL_SENSITIVE);
        if(!mt_eint_read_sens(i))
        {
            MSG("EINT %d cannot be set LEVEL sensitive!\r\n", i);
            return -1;
        }

        mt_eint_set_sens(i, EDGE_SENSITIVE);
        if(mt_eint_read_sens(i))
        {
            MSG("EINT %d cannot be set EDGE sensitive!\r\n", i);
            return -1;
        }

        if(sta){
            mt_eint_set_sens(i, LEVEL_SENSITIVE);
        }
        else{
            mt_eint_set_sens(i, EDGE_SENSITIVE);
        }
	}

    return 0;
}

int eint_polarity_set_clear_test(void){
	unsigned int i;
    unsigned int sta;

	for (i = 0; i < EINT_AP_MAXNUMBER; i++) {
        sta = mt_eint_read_polarity(i);
        mt_eint_set_polarity(i, LOW_LEVEL_TRIGGER);

        mt_eint_set_polarity(i, HIGH_LEVEL_TRIGGER);
        if(!mt_eint_read_polarity(i))
        {
            MSG("EINT %d cannot be set LEVEL sensitive!\r\n", i);
            return -1;
        }

        mt_eint_set_polarity(i, LOW_LEVEL_TRIGGER);
        if(mt_eint_read_polarity(i))
        {
            MSG("EINT %d cannot be set EDGE sensitive!\r\n", i);
            return -1;
        }

        if(sta){
            mt_eint_set_polarity(i, HIGH_LEVEL_TRIGGER);
        }
        else{
            mt_eint_set_polarity(i, LOW_LEVEL_TRIGGER);
        }
	}

    return 0;
}

int eint_softirq_set_clear_test(void){
	unsigned int i;
    unsigned int sta;

	for (i = 0; i < EINT_AP_MAXNUMBER; i++) {
        sta = mt_eint_read_soft(i);
        mt_eint_soft_clr(i);

        mt_eint_soft_set(i);
        if(!mt_eint_read_soft(i))
        {
            MSG("EINT %d cannot be masked!\r\n", i);
            return -1;
        }

        mt_eint_soft_clr(i);
        if(mt_eint_read_soft(i))
        {
            MSG("EINT %d cannot be unmasked!\r\n", i);
            return -1;
        }

        if(sta){
            mt_eint_soft_set(i);
        }
        else{
            mt_eint_soft_clr(i);
        }
	}

    return 0;
}

int eint_debounce_set_clear_test(void){
	unsigned int i;
    unsigned int sta;
    unsigned int bit;

	for (i = 0; i < MAX_HW_DEBOUNCE_CNT; i++) {
        bit = 0x73 << ((i % 4) * 8);
        WRITE_REG(bit, ((i / 4) * 4 + EINT_DBNC_CLR_BASE));

        WRITE_REG(bit, ((i / 4) * 4 + EINT_DBNC_SET_BASE));
        sta = READ_REG(((i / 4) * 4 + EINT_DBNC_BASE));
        if((sta & bit) != (unsigned int)(0x71 << ((i % 4) * 8)))
        {
            MSG("EINT %d debounce control register cannot be set! %x != %x\r\n", i, (sta & bit), (0x71 << ((i % 4) * 8)));
            return -1;
        }

        WRITE_REG(bit, ((i / 4) * 4 + EINT_DBNC_CLR_BASE));
        sta = READ_REG(((i / 4) * 4 + EINT_DBNC_BASE));
        if((sta & bit) != (unsigned int)(0x0))
        {
            MSG("EINT %d debounce control register cannot be clear! %x != %x\r\n", i, (sta & bit), 0x0);
            return -1;
        }
	}

    return 0;
}
