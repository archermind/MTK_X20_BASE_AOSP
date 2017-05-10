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

#include "main.h"
/*   Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <driver_api.h>
#include <dma.h>
#include <string.h>
#include <interrupt.h>
#include <platform.h>
#include <mpu.h>

#define DMA_DEBUG   0

#if(DMA_DEBUG == 1)
#define dbgmsg PRINTF_D
#else
#define dbgmsg(...)
#endif
/*
 * global variables
 */

static struct dma_ctrl_t dma_ctrl[NR_GDMA_CHANNEL];
#define SEMAPHORE_INIT_ADDR 0xA1015000
#define SEMAPHORE_INIT_VALUE 0x0b160001
#define SEMAPHORE_ADDR 0xA1015434
static int semaphore_count = 0;
/*
 * get_emi_semaphore: must get emi resource before access emi
 */
/* Althoug it will access infra bus here.
 * There is no need to enable infra clock here,
 * the infra clock already enable in prevous function call.
 * */
void get_emi_semaphore()
{
    uint32_t reg;
    int32_t timeout = 10000;

    /*Init HW semaphore : write 0x0b160001 in the physical address 0x11015000
     * (Do it every time)
     * */
    drv_write_reg32(SEMAPHORE_INIT_ADDR, SEMAPHORE_INIT_VALUE);
    do {
        drv_write_reg32(SEMAPHORE_ADDR, 0x1);
        reg = drv_reg32(SEMAPHORE_ADDR);
        timeout --;
        if (timeout < 0) {
            PRINTF_E("[DMA] fail to take semaphore\n");
            break;
        }
    } while (reg == 0x0);/*take semaphore fail, retry*/
    disable_dram_protector();
    semaphore_count ++;
}

/*
 * release_emi_semaphore: must release emi resource after emi access
 */
/* Althoug it will access infra bus here.
 * There is no need to enable infra clock here,
 * the infra clock already enable in prevous function call.
 * */
void release_emi_semaphore()
{
    uint32_t reg;

    configASSERT(semaphore_count);
    semaphore_count --;
    /*only release semaphore when there is no one take the semaphore*/
    if (semaphore_count == 0) {
        drv_write_reg32(SEMAPHORE_ADDR, 0x1);
        reg = drv_reg32(SEMAPHORE_ADDR);
        if (reg != 0x0) { /*make sure the semaphore already release*/
            PRINTF_E("[DMA] fail to release semaphore,retry\n");
            drv_write_reg32(SEMAPHORE_ADDR, 0x1);
        }
        /*prevent DRAM from access without taking semaphore*/
        enable_dram_protector();
    }
}

/*
 * mt_req_gdma: request a general DMA.
 * @chan: specify a channel or not
 * Return channel number for success; return negative errot code for failure.
 */
int32_t mt_req_gdma(DMA_CHAN chan)
{
    int32_t i;

    dbgmsg("GDMA - %s\n", "mt_req_gdma");
    taskENTER_CRITICAL();
    if (dma_ctrl[chan].in_use) {
        i = NR_GDMA_CHANNEL;
    } else {
        i = chan;
        dma_ctrl[chan].in_use = 1;
    }
    taskEXIT_CRITICAL();

    if (i < NR_GDMA_CHANNEL) {
        mt_reset_gdma_conf(i);
        return i;
    } else {
        return -DMA_ERR_NO_FREE_CH;
    }
}

/*
 * mt_start_gdma: start the DMA stransfer for the specified GDMA channel
 * @channel: GDMA channel to start
 * Return 0 for success; return negative errot code for failure.
 */
int32_t mt_start_gdma(int32_t channel)
{
    uint32_t dma_channel;

    dma_channel = DMA_BASE_CH(channel);

    if ((channel < GDMA_START) || (channel >= (GDMA_START + NR_GDMA_CHANNEL))) {
        return -DMA_ERR_INVALID_CH;
    } else if (dma_ctrl[channel].in_use == 0) {
        return -DMA_ERR_CH_FREE;
    }

    dbgmsg("GDMA_%d - %s\n", channel, "mt_start_gdma");
    get_emi_semaphore();/*get semaphore before access emi*/
    drv_write_reg32(DMA_ACKINT(dma_channel), DMA_ACK_BIT);
    drv_write_reg32(DMA_START(dma_channel), DMA_START_BIT);

    return 0;
}

/*
 * mt_polling_gdma: wait the DMA to finish for the specified GDMA channel
 * @channel: GDMA channel to polling
 * @timeout: polling timeout in ms
 * Return 0 for success; return negative errot code for failure.
 */
int32_t mt_polling_gdma(int32_t channel, uint32_t timeout)
{
    if (channel < GDMA_START) {
        return -DMA_ERR_INVALID_CH;
    }

    if (channel >= (GDMA_START + NR_GDMA_CHANNEL)) {
        return -DMA_ERR_INVALID_CH;
    }

    if (dma_ctrl[channel].in_use == 0) {
        return -DMA_ERR_CH_FREE;
    }

    dbgmsg("GDMA_%d - %s\n", channel, "mt_polling_gdma");

    //timeout = 0x10000;//jiffies + ((HZ * timeout) / 1000);

    dbgmsg("GDMA_%ld: GBLSTA:0x%x\n", drv_reg32(DMA_GLBSTA));
    while ((drv_reg32(DMA_GLBSTA) & DMA_GLBSTA_RUN(channel))) {
        timeout--;
        if (!timeout) {
            dbgmsg("GDMA_%ld polling timeout!!!!\n", channel + 1);
            return 1;
        }
    }

    return 0;
}

/*
 * mt_stop_gdma: stop the DMA stransfer for the specified GDMA channel
 * @channel: GDMA channel to stop
 * Return 0 for success; return negative errot code for failure.
 */
int32_t mt_stop_gdma(int32_t channel)
{
    uint32_t dma_channel;

    dma_channel = DMA_BASE_CH(channel);
    if (channel < GDMA_START) {
        return -DMA_ERR_INVALID_CH;
    }

    if (channel >= (GDMA_START + NR_GDMA_CHANNEL)) {
        return -DMA_ERR_INVALID_CH;
    }

    if (dma_ctrl[channel].in_use == 0) {
        return -DMA_ERR_CH_FREE;
    }

    dbgmsg("GDMA_%d - %s\n", channel, "mt_stop_gdma");

    drv_write_reg32(DMA_ACKINT(dma_channel), DMA_ACK_BIT);
    drv_write_reg32(DMA_START(dma_channel), DMA_START_CLR_BIT);

    return 0;
}

/*
 * mt_config_gdma: configure the given GDMA channel.
 * @channel: GDMA channel to configure
 * @config: pointer to the mt_gdma_conf structure in which the GDMA configurations store
 * @flag: ALL, SRC, DST, or SRC_AND_DST.
 * Return 0 for success; return negative errot code for failure.
 */
int32_t mt_config_gdma(int32_t channel, struct mt_gdma_conf *config, DMA_CONF_FLAG flag)
{
    uint32_t dma_con = 0x0;
    uint32_t dma_channel;

    dma_channel = DMA_BASE_CH(channel);

    if ((channel < GDMA_START) || (channel >= (GDMA_START + NR_GDMA_CHANNEL))) {
        return -DMA_ERR_INVALID_CH;
    }

    if (dma_ctrl[channel].in_use == 0) {
        return -DMA_ERR_CH_FREE;
    }

    if (!config) {
        return -DMA_ERR_INV_CONFIG;
    }

    if (config->count > MAX_TRANSFER_LEN) {
        dbgmsg("count:0x%x over 0x%x\n", config->count, MAX_TRANSFER_LEN);
        return -DMA_ERR_INV_CONFIG;
    }

    if (config->count <= 0) {
        dbgmsg("count:0x%x, not valid value\n", config->count);
        return -DMA_ERR_INV_CONFIG;
    }

    if (config->limiter) {
        dbgmsg("counter over 0x%x\n", MAX_SLOW_DOWN_CNTER);
        return -DMA_ERR_INV_CONFIG;
    }

    dbgmsg("GDMA_%d - %s\n", channel, "config");

    switch (flag) {
        case ALL:
            /* Control Register */
            drv_write_reg32(DMA_SRC(dma_channel), config->src);
            drv_write_reg32(DMA_DST(dma_channel), config->dst);
            drv_write_reg32(DMA_COUNT(dma_channel), (config->count & MAX_TRANSFER_LEN));

            if (config->wpen) {
                dbgmsg("wpen err\n");
                return -DMA_ERR_INV_CONFIG;
                //dma_con |= DMA_CON_WPEN;
            }
            if (config->wpen) {
                dbgmsg("wpen err\n");
                return -DMA_ERR_INV_CONFIG;
                //dma_con |= DMA_CON_WPEN;
            }
            if (config->wpen) {
                dbgmsg("wpen err\n");
                return -DMA_ERR_INV_CONFIG;
                //dma_con |= DMA_CON_WPEN;
            }

            if (config->wpsd) {
                dbgmsg("wpsd err\n");
                return -DMA_ERR_INV_CONFIG;
                //dma_con |= DMA_CON_WPSD;
            }
            if (config->wpsd) {
                dbgmsg("wpsd err\n");
                return -DMA_ERR_INV_CONFIG;
                //dma_con |= DMA_CON_WPSD;
            }
            if (config->wpsd) {
                dbgmsg("wpsd err\n");
                return -DMA_ERR_INV_CONFIG;
                //dma_con |= DMA_CON_WPSD;
            }

            if (config->iten) {
                dma_ctrl[channel].isr_cb = config->isr_cb;
                dma_ctrl[channel].data = config->data;
                dma_con |= (config->iten << DMA_CON_ITEN);
            } else {
                dma_ctrl[channel].isr_cb = NULL;
                dma_ctrl[channel].data = NULL;
            }
            dma_con |= (config->sinc << DMA_CON_SINC);
            dma_con |= (config->dinc << DMA_CON_DINC);
            dma_con |= (config->size_per_count << DMA_CON_SIZE);
            dma_con |= (config->burst << DMA_CON_BURST);

            drv_write_reg32(DMA_CON(dma_channel), dma_con);
            dbgmsg("src:0x%x, dst:0x%x, count:0x%x, con:0x%x\n\r", config->src, config->dst, config->count, dma_con);
            break;

        case SRC:
            drv_write_reg32(DMA_SRC(dma_channel), config->src);

            break;

        case DST:
            drv_write_reg32(DMA_DST(dma_channel), config->dst);
            break;

        case SRC_AND_DST:
            drv_write_reg32(DMA_SRC(dma_channel), config->src);
            drv_write_reg32(DMA_DST(dma_channel), config->dst);
            break;

        default:
            break;
    }

    /* use the data synchronization barrier to ensure that all writes are completed */
    //dsb();

    return 0;
}


/*
 * mt_free_gdma: free a general DMA.
 * @channel: channel to free
 * Return 0 for success; return negative errot code for failure.
 */
int32_t mt_free_gdma(int32_t channel)
{

    if (channel < GDMA_START) {
        return -DMA_ERR_INVALID_CH;
    }

    if (channel >= (GDMA_START + NR_GDMA_CHANNEL)) {
        return -DMA_ERR_INVALID_CH;
    }

    if (dma_ctrl[channel].in_use == 0) {
        return -DMA_ERR_CH_FREE;
    }

    dbgmsg("GDMA_%d - %s\n", channel, "mt_free_gdma");

    mt_stop_gdma(channel);

    dma_ctrl[channel].isr_cb = NULL;
    dma_ctrl[channel].data = NULL;
    dma_ctrl[channel].in_use = 0;
    release_emi_semaphore();/*release semaphore after access emi*/

    return 0;
}


/*
 * mt_dump_gdma: dump registers for the specified GDMA channel
 * @channel: GDMA channel to dump registers
 * Return 0 for success; return negative errot code for failure.
 */
int32_t mt_dump_gdma(int32_t channel)
{
    uint32_t i, reg;
    uint32_t j = 0;

    dbgmsg("GDMA> reg_dump 0x%X=\n\t", DMA_BASE_CH(channel));
    for (i = 0; i <= GDMA_REG_BANK_SIZE; i += 4) {
        reg = drv_reg32(DMA_BASE_CH(channel) + i);
        PRINTF_D("0x%lx ", reg);

        if (j++ >= 3) {
            dbgmsg("\n");
            dbgmsg("GDMA> reg_dump 0x%X=\n\t", DMA_BASE_CH(channel) + i + 4);
            j = 0;
        }
    }
    return 0;
}

/*
 * mt_reset_gdma_conf: reset the config of the specified DMA channel
 * @iChannel: channel number of the DMA channel to reset
 */
void mt_reset_gdma_conf(const uint32_t iChannel)
{
    struct mt_gdma_conf conf;

    dbgmsg("GDMA_%d - %s\n", iChannel, "mt_reset_gdma_conf");

    memset(&conf, 0, sizeof(struct mt_gdma_conf));

    if (mt_config_gdma(iChannel, &conf, ALL) != 0) {
        return;
    }

    return;
}

static void MT_DMA_IRQHandler(void)
{
    PRINTF("in %s\n\r", __func__);

    volatile unsigned glbsta = drv_reg32(DMA_GLBSTA);
    PRINTF_D("[DMA] GLBSTA:0x%x\n\r", glbsta);
    int32_t channel;
    uint32_t dma_channel;
    for (channel = 0; channel < NR_GDMA_CHANNEL; channel++) {
        if (glbsta & (0x2 << (channel * 2))) {
            if (dma_ctrl[channel].isr_cb)
                dma_ctrl[channel].isr_cb(dma_ctrl[channel].data);
            dma_channel = DMA_BASE_CH(channel);
            drv_write_reg32(DMA_ACKINT(dma_channel), DMA_ACK_BIT);
            drv_write_reg32(DMA_START(dma_channel), DMA_START_CLR_BIT);
        } else {
            //PRINTF_D("[DMA] discard interrupt\n");
        }
    }
}

/*
 * mt_init_dma: initialize DMA.
 * Always return 0.
 */
int32_t mt_init_dma(void)
{
    int32_t i;
    for (i = 0; i < NR_GDMA_CHANNEL; i++) {
        mt_reset_gdma_conf(i);
    }

    request_irq(DMA_IRQn, MT_DMA_IRQHandler, "DMA");
    dbgmsg("Init DMA OK\n");

    return 0;
}
