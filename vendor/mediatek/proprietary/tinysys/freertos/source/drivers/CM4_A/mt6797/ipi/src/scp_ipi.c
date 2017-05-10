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

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <mt_reg_base.h>
#include <driver_api.h>
#include <platform.h>
#include <interrupt.h>
#include "FreeRTOS.h"
#include "task.h"
#include <scp_ipi.h>

static volatile uint32_t g_ipi_count = 0;
static struct ipi_desc_t ipi_desc[NR_IPI];
static struct share_obj *scp_send_obj, *scp_rcv_obj;
static uint32_t ipi_init_ready = 0;

void ipi_status_dump(void)
{
    int32_t id;

    PRINTF_E("id\tname\tcount\tlast\twakeup\n\r");
    for (id = 0; id < NR_IPI; id++) {
        PRINTF_E("%d\t%s\t%d\t%d\t%d\n\r",
                 id, ipi_desc[id].name, ipi_desc[id].irq_count, ipi_desc[id].last_handled, ipi_desc[id].is_wakeup_src);
    }
}

static void ipi_scp2spm(void)
{
    /* There are two cases, SCP will send interrupt to SPM via SCP_TO_SPM_REG:
     * 1. voltage change
     * 2. wakeup AP
     * SPM_SW_RSV_3 is the mailbox use to check which purpose we send ipi to SPM
     * SPM_SW_RSV_3[3: 0] is the DVS mailbox between SCP and SPM
     * SPM_SW_RSV_3[31:4] is zero    : SPM will clean SCP_TO_SPM_REG by itself
     * SPM_SW_RSV_3[31:4] is NOT zero: SPM will not clean SCP_TO_SPM_REG, AP needs to clean it
     * */
#define SPM_BASE    0x400ac000
#define SPM_SW_RSV_3    (SPM_BASE + 0x614)
#define IPC_MD2SPM  (1<<0)
    DRV_WriteReg32(SPM_SW_RSV_3, DRV_Reg32(SPM_SW_RSV_3) | (0xBABE << 16));
    SCP_TO_SPM_REG = IPC_MD2SPM;
}

void scp_ipi_wakeup_ap_registration(enum ipi_id id)
{
    if (id < NR_IPI)
        ipi_desc[id].is_wakeup_src = 1;
}

/*
 * check if the ipi id is a wakeup ipi or not
 * if it is a wakeup ipi, request SPM to wakeup AP
@param id:       IPI ID
*/
static void try_to_wakeup_ap(enum ipi_id id)
{
    if (id < NR_IPI)
        if (ipi_desc[id].is_wakeup_src == 1)
            ipi_scp2spm(); //wake APMCU up
}
/*
 * send ipi to ap
@param id:       IPI ID
*/
static void ipi_scp2host(enum ipi_id id)
{
    try_to_wakeup_ap(id);
    SCP_TO_HOST_REG = IPC_SCP2HOST;
}

void ipi_handler(void)
{
//    printf("in %s\n\r",__func__);
//    printf("ipi id:%d, len:%d\n\r",DRV_Reg32(&scp_rcv_obj->id),DRV_Reg32(&scp_rcv_obj->len));
    if (scp_rcv_obj->id >= NR_IPI) {
        PRINTF_E("wrong id:%d\n", scp_rcv_obj->id);
    } else if (ipi_desc[scp_rcv_obj->id].handler) {
        ipi_desc[scp_rcv_obj->id].irq_count ++;
        ipi_desc[scp_rcv_obj->id].last_handled = xTaskGetTickCountFromISR();
        ipi_desc[scp_rcv_obj->id].handler(scp_rcv_obj->id, scp_rcv_obj->share_buf, scp_rcv_obj->len);
    }
    HOST_TO_SCP_REG = 0x0;
}

void scp_ipi_init(void)
{
    scp_send_obj = (struct share_obj *)SCP_IPC_SHARE_BUFFER;
    scp_rcv_obj = scp_send_obj + 1;
    PRINTF_E("scp_send_obj = %p \n\r", scp_send_obj);
    PRINTF_E("scp_rcv_obj = %p \n\r", scp_rcv_obj);
    PRINTF_E("scp_rcv_obj->share_buf = %p \n\r", scp_rcv_obj->share_buf);

    memset(scp_send_obj, 0, SHARE_BUF_SIZE);
    // memset(ipi_desc,0, sizeof(ipi_desc));

    HOST_TO_SCP_REG = 0x0;
    request_irq(IPC0_IRQn, ipi_handler, "IPC0");
    ipi_init_ready = 1;
#ifdef IPI_AS_WAKEUP_SRC
    //scp_wakeup_src_setup(MT_IPC_HOST_IRQ_ID, 1); //disable for FPGA, can ref from md32_esh\irq.c
#endif
}

/*
@param id:       IPI ID
@param handler:  IPI handler
@param name:     IPI name
*/
ipi_status scp_ipi_registration(enum ipi_id id, ipi_handler_t handler, const char *name)
{
    if (id < NR_IPI && ipi_init_ready == 1) {
        ipi_desc[id].name = name;
        ipi_desc[id].irq_count = 0;
        ipi_desc[id].last_handled = 0;

        if (handler == NULL)
            return ERROR;

        ipi_desc[id].handler = handler;
        return DONE;

    } else {
        PRINTF_E("[IPI] id:%d, ipi_init_ready:%d", id, ipi_init_ready);
        return ERROR;
    }
}

/*
@param id:       IPI ID
@param buf:      the pointer of data
@param len:      data length
@param wait:     If true, wait (atomically) until data have been gotten by Host
*/
ipi_status scp_ipi_send(enum ipi_id id, void* buf, uint32_t len, uint32_t wait, enum ipi_dir dir)
{
    uint32_t ipi_idx;

#ifdef CFG_TESTSUITE_SUPPORT
    /*prevent from infinity wait when run testsuite*/
    wait = 0;
#endif

    if (is_in_isr() && wait) {
        /*prevent from infinity wait when be in isr context*/
        configASSERT(0);
    }
    if (id < NR_IPI) {
        if (len > sizeof(scp_send_obj->share_buf) || buf == NULL)
            return ERROR;

        taskENTER_CRITICAL();

        /*check if there is already an ipi pending in AP*/
        if (SCP_TO_HOST_REG & IPC_SCP2HOST) {
            /*If the following conditions meet,
             * 1)there is an ipi pending in AP
             * 2)the coming IPI is a wakeup IPI
             * so it assumes that AP is in suspend state
             * send a AP wakeup request to SPM
             * */
            /*the coming IPI will be checked if it's a wakeup source*/
            try_to_wakeup_ap(id);
            taskEXIT_CRITICAL();
            return BUSY;
        }

        memcpy((void *)scp_send_obj->share_buf, buf, len);
        scp_send_obj->len = len;
        scp_send_obj->id = id;
        ipi_scp2host(id);

        g_ipi_count++;
        ipi_idx = g_ipi_count;

        taskEXIT_CRITICAL();

        if (wait)
            while ((SCP_TO_HOST_REG & IPC_SCP2HOST) && (ipi_idx == g_ipi_count));
    } else
        return ERROR;

    return DONE;
}
