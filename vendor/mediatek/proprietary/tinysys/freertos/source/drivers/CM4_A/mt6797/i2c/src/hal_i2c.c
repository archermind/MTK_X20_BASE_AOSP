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

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <driver_api.h>
#include <platform.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <scp_sem.h>
#include <hal_i2c.h>
#include <interrupt.h>
#include <task.h>
#include <vcore_dvfs.h>

SemaphoreHandle_t i2c_mutex[I2C_NR];/* For semaphore handling */
SemaphoreHandle_t i2c_irq_semaphore[I2C_NR];/* For interrupt handling */
static uint32_t irq_stat[I2C_NR];

/* ****************************internal API****************************************************** */
/* #define mt_reg_sync_writel(v, a)  *(volatile unsigned int *)(a) = (v) */
#define mt_reg_sync_writel(v, a)  *(volatile unsigned int *)(a) = (v)
#define I2C_PMIC_WR(addr, data)   pwrap_write((uint32_t)addr, data)
#define I2C_PMIC_RD(addr)         ({ \
    uint32_t ext_data; \
    (pwrap_read((uint32_t)addr, &ext_data) != 0) ?  -1:ext_data; })

static inline void i2c_writel(mt_i2c *i2c, uint8_t offset, uint16_t value)
{
    /* __raw_writew(value, (i2c->base) + (offset)); */
    mt_reg_sync_writel(value, (i2c->base) + (offset));
}

static inline uint32_t i2c_readl(mt_i2c *i2c, uint8_t offset)
{
    return DRV_Reg32((i2c->base) + (offset));
}

/* *********************************declare  API************************ */
/* static void mt_i2c_clock_enable(mt_i2c *i2c);
static void mt_i2c_clock_disable(mt_i2c *i2c); */
#if 1
static inline int i2c_clock_enable()
{
    drv_set_reg32(SCP_CLK_CTRL + OFFSET_CLK_CG_CTRL, 0b11 << 3);
    return 0;
}

static inline int i2c_clock_disable()
{
    drv_clr_reg32(SCP_CLK_CTRL + OFFSET_CLK_CG_CTRL, 0b11 << 3);
    return 0;
}
#else

#define i2c_SET_BITS(REG, BS)       OUTREG32(REG, INREG32(REG) | (unsigned int)(BS))
#define i2c_CLR_BITS(REG, BC)       OUTREG32(REG, INREG32(REG) & ~((unsigned int)(BC)))

static inline int i2c_clock_enable()
{
    i2c_SET_BITS(0xA0001084, 3 << 11);
    return 0;
}

static inline int i2c_clock_disable()
{
    i2c_SET_BITS(0xA0001080, 3 << 11);
    return 0;
}
#endif

#define I2C_SEM_TIMEOUT 100
static int i2c_get_scp_semaphore(mt_i2c *i2c)
{
    int ret = 0;
    int count = I2C_SEM_TIMEOUT;

    /* Semaphore handling */
    do {
        switch (i2c->id) {
            case I2C0:
                ret = get_scp_semaphore(SEMAPHORE_I2C0);
                break;

            case I2C1:
                ret = get_scp_semaphore(SEMAPHORE_I2C1);
                break;

            default:
                I2CLOG("Invalid I2C number!\n");
                return ret;
        }

        if (ret == 1) {
            /* get semaphore successfully */
            break;
        }
    } while (count--);

    if (count <= 0) {
        I2CERR("Get I2C%d sem timeout!\n\r", i2c->id);
        return -ETIMEDOUT_I2C;
    }

    return 0;
}

static int i2c_release_scp_semaphore(mt_i2c *i2c)
{
    int ret = 0;
    int count = I2C_SEM_TIMEOUT;

    /* Semaphore handling */
    do {
        switch (i2c->id) {
            case I2C0:
                ret = release_scp_semaphore(SEMAPHORE_I2C0);
                break;

            case I2C1:
                ret = release_scp_semaphore(SEMAPHORE_I2C1);
                break;

            default:
                I2CLOG("Invalid I2C number!\n");
                return ret;
        }

        if (ret == 1) {
            /* release semaphore successfully */
            break;
        }
    } while (count--);

    if (count <= 0) {
        I2CERR("Rel I2C%d sem timeout!\n\r", i2c->id);
        return -ETIMEDOUT_I2C;
    }

    return 0;
}

/* *********************************I2C common Param ************************ */
volatile uint32_t I2C_TIMING_REG_BACKUP[7] = { 0 };

/* *********************************i2c debug****************************************************** */
#define I2C_DEBUG_FS
#ifdef I2C_DEBUG_FS
#define PORT_COUNT 7
#define MESSAGE_COUNT 16
#define I2C_T_DMA 1
#define I2C_T_TRANSFERFLOW 2
#define I2C_T_SPEED 3
/*7 ports,16 types of message */
uint8_t i2c_port[PORT_COUNT][MESSAGE_COUNT];

#define I2CINFO(type, format, arg...) do { \
    if (type < MESSAGE_COUNT && type >= 0) { \
        if (i2c_port[i2c->id][0] != 0 && ( i2c_port[i2c->id][type] != 0 || i2c_port[i2c->id][MESSAGE_COUNT - 1] != 0)) { \
            I2CLOG(format, ## arg); \
        } \
    } \
} while (0)

#else
#define I2CINFO(type, format, arg...)
#endif

/* *********************************common API****************************************************** */
/*Set i2c port speed*/
int32_t i2c_set_speed(mt_i2c *i2c)
{
    int32_t ret = 0;
    static int32_t mode;
    static uint32_t khz;
    /* uint32_t base = i2c->base; */
    uint16_t step_cnt_div = 0;
    uint16_t sample_cnt_div = 0;
    uint32_t tmp, sclk, hclk = i2c->clk;
    uint16_t max_step_cnt_div = 0;
    uint32_t diff, min_diff = i2c->clk;
    uint16_t sample_div = MAX_SAMPLE_CNT_DIV;
    uint16_t step_div = 0;
    uint16_t i2c_timing_reg = 0;
    /* I2CFUC(); */
    /* compare the current mode with the latest mode */
    i2c_timing_reg = i2c_readl(i2c, OFFSET_TIMING);
    if ((mode == i2c->mode) && (khz == i2c->speed)
            && (i2c_timing_reg == I2C_TIMING_REG_BACKUP[i2c->id])) {
        I2CINFO(I2C_T_SPEED, " set sclk to %ldkhz\n", i2c->speed);
        /* I2CLOG(" set sclk to %ldkhz\n", i2c->speed); */
        /* return 0; */
    }
    mode = i2c->mode;
    khz = i2c->speed;

    max_step_cnt_div = (mode == HS_MODE) ? MAX_HS_STEP_CNT_DIV : MAX_STEP_CNT_DIV;
    step_div = max_step_cnt_div;

    if ((mode == FS_MODE && khz > MAX_FS_MODE_SPEED)
            || (mode == HS_MODE && khz > MAX_HS_MODE_SPEED)) {
        I2CERR(" the speed is too fast for this mode.\n\r");
        I2C_BUG_ON((mode == FS_MODE && khz > MAX_FS_MODE_SPEED)
                   || (mode == HS_MODE && khz > MAX_HS_MODE_SPEED));
        ret = -EINVAL_I2C;
        goto end;
    }
    /* I2CERR("first:khz=%d,mode=%d sclk=%d,min_diff=%d,max_step_cnt_div=%d\n",khz,mode,sclk,min_diff,max_step_cnt_div); */
    /*Find the best combination */
    for (sample_cnt_div = 1; sample_cnt_div <= MAX_SAMPLE_CNT_DIV; sample_cnt_div++) {
        for (step_cnt_div = 1; step_cnt_div <= max_step_cnt_div; step_cnt_div++) {
            sclk = (hclk >> 1) / (sample_cnt_div * step_cnt_div);
            if (sclk > khz)
                continue;
            diff = khz - sclk;
            if (diff < min_diff) {
                min_diff = diff;
                sample_div = sample_cnt_div;
                step_div = step_cnt_div;
            }
        }
    }
    sample_cnt_div = sample_div;
    step_cnt_div = step_div;
    sclk = hclk / (2 * sample_cnt_div * step_cnt_div);
    /* I2CERR("second:sclk=%d khz=%d,i2c->speed=%d hclk=%d sample_cnt_div=%d,step_cnt_div=%d.\n",sclk,khz,i2c->speed,hclk,sample_cnt_div,step_cnt_div); */
    if (sclk > khz) {
        I2CERR("%s mode: unsupported speed (%ldkhz)\n\r", (mode == HS_MODE) ? "HS" : "ST/FT",
               khz);
        I2C_BUG_ON(sclk > khz);
        ret = -ENOTSUPP_I2C;
        goto end;
    }

    step_cnt_div--;
    sample_cnt_div--;

    /* spin_lock(&i2c->lock); */

    if (mode == HS_MODE) {

        /*Set the hignspeed timing control register */
        tmp = i2c_readl(i2c, OFFSET_TIMING) & ~((0x7 << 8) | (0x3f << 0));
        tmp = (0 & 0x7) << 8 | (16 & 0x3f) << 0 | tmp;
        i2c->timing_reg = tmp;
        /* i2c_writel(i2c, OFFSET_TIMING, tmp); */
        I2C_TIMING_REG_BACKUP[i2c->id] = tmp;

        /*Set the hign speed mode register */
        tmp = i2c_readl(i2c, OFFSET_HS) & ~((0x7 << 12) | (0x7 << 8));
        tmp = (sample_cnt_div & 0x7) << 12 | (step_cnt_div & 0x7) << 8 | tmp;
        /*Enable the hign speed transaction */
        tmp |= 0x0001;
        i2c->high_speed_reg = tmp;
        /* i2c_writel(i2c, OFFSET_HS, tmp); */
    } else {
        /*Set non-highspeed timing */
        tmp = i2c_readl(i2c, OFFSET_TIMING) & ~((0x7 << 8) | (0x3f << 0));
        tmp = (sample_cnt_div & 0x7) << 8 | (step_cnt_div & 0x3f) << 0 | tmp;
        i2c->timing_reg = tmp;
        I2C_TIMING_REG_BACKUP[i2c->id] = tmp;
        /* i2c_writel(i2c, OFFSET_TIMING, tmp); */
        /*Disable the high speed transaction */
        /* I2CERR("NOT HS_MODE============================1\n"); */
        tmp = i2c_readl(i2c, OFFSET_HS) & ~(0x0001);
        /* I2CERR("NOT HS_MODE============================2\n"); */
        i2c->high_speed_reg = tmp;
        /* i2c_writel(i2c, OFFSET_HS, tmp); */
        /* I2CERR("NOT HS_MODE============================3\n"); */
    }
    /* spin_unlock(&i2c->lock); */
    I2CINFO(I2C_T_SPEED, " set sclk to %ldkhz(orig:%ldkhz), sample=%d,step=%d\n\r", sclk, khz,
            sample_cnt_div, step_cnt_div);
end:
    return ret;
}

void _i2c_dump_info(mt_i2c *i2c)
{
    /* I2CFUC(); */
    int freq;
    switch (get_cur_clk()) {
        case CLK_354M:
            freq = 354;
            break;
        case CLK_224M:
            freq = 224;
            break;
        case CLK_112M:
            freq = 112;
            break;
        default:
            freq = -1;
            break;
    }

    I2CERR("SCP --> cpu freq : %d MHz, 0x400a4000 : 0x%x .\n\r", freq, DRV_Reg32(0x400a4000));

    I2CERR("fwq: scl mode %x \n\r", DRV_Reg32(0xA0005360));
    I2CERR("fwq: dir %x  \n\r", DRV_Reg32(0xA0005010));
    I2CERR("fwq: din %x \n\r", DRV_Reg32(0xA0005210));
    I2CERR("fwq: dout %x  \n\r", DRV_Reg32(0xA0005110));
    I2CERR("fwq: sda mode %x  \n\r", DRV_Reg32(0xA0005370));

    I2CERR("I2C structure:\n\r"
           I2CTAG "Clk=%ld,Id=%d,Mode=%x,St_rs=%x,Dma_en=%x,Op=%x,Poll_en=%x,Irq_stat=%x\n\r"
           I2CTAG "Trans_len=%x,Trans_num=%x,Trans_auxlen=%x,Data_size=%x,speed=%ld\n\r",
           /* ,Trans_stop=%u,Trans_comp=%u,Trans_error=%u\n" */
           i2c->clk, i2c->id, i2c->mode, i2c->st_rs, i2c->dma_en, i2c->op, i2c->poll_en,
           i2c->irq_stat, i2c->trans_data.trans_len, i2c->trans_data.trans_num,
           i2c->trans_data.trans_auxlen, i2c->trans_data.data_size, i2c->speed);
    /* atomic_read(&i2c->trans_stop),atomic_read(&i2c->trans_comp),atomic_read(&i2c->trans_err), */

    I2CERR("base address 0x%lx\n\r", i2c->base);
    I2CERR("I2C register:\n\r"
           I2CTAG "SLAVE_ADDR=%lx,INTR_MASK=%lx,INTR_STAT=%lx,CONTROL=%lx,TRANSFER_LEN=%lx\n\r"
           I2CTAG "TRANSAC_LEN=%lx,DELAY_LEN=%lx,TIMING=%lx,START=%lx,FIFO_STAT=%lx\n\r"
           I2CTAG
           "IO_CONFIG=%lx,HS=%lx,DCM_EN=%lx,DEBUGSTAT=%lx,EXT_CONF=%lx,TRANSFER_LEN_AUX=%lx\n\r",
           (i2c_readl(i2c, OFFSET_SLAVE_ADDR)), (i2c_readl(i2c, OFFSET_INTR_MASK)),
           (i2c_readl(i2c, OFFSET_INTR_STAT)), (i2c_readl(i2c, OFFSET_CONTROL)),
           (i2c_readl(i2c, OFFSET_TRANSFER_LEN)), (i2c_readl(i2c, OFFSET_TRANSAC_LEN)),
           (i2c_readl(i2c, OFFSET_DELAY_LEN)), (i2c_readl(i2c, OFFSET_TIMING)),
           (i2c_readl(i2c, OFFSET_START)), (i2c_readl(i2c, OFFSET_FIFO_STAT)),
           (i2c_readl(i2c, OFFSET_IO_CONFIG)), (i2c_readl(i2c, OFFSET_HS)),
           (i2c_readl(i2c, OFFSET_DCM_EN)), (i2c_readl(i2c, OFFSET_DEBUGSTAT)),
           (i2c_readl(i2c, OFFSET_EXT_CONF)), (i2c_readl(i2c, OFFSET_TRANSFER_LEN_AUX)));

    /*
       I2CERR("DMA register:\nINT_FLAG %x\nCON %x\nTX_MEM_ADDR %x\nRX_MEM_ADDR %x\nTX_LEN %x\nRX_LEN %x\nINT_EN %x\nEN %x\n",
       (__raw_readl(i2c->pdmabase+OFFSET_INT_FLAG)),
       (__raw_readl(i2c->pdmabase+OFFSET_CON)),
       (__raw_readl(i2c->pdmabase+OFFSET_TX_MEM_ADDR)),
       (__raw_readl(i2c->pdmabase+OFFSET_RX_MEM_ADDR)),
       (__raw_readl(i2c->pdmabase+OFFSET_TX_LEN)),
       (__raw_readl(i2c->pdmabase+OFFSET_RX_LEN)),
       (__raw_readl(i2c->pdmabase+OFFSET_int32_t_EN)),
       (__raw_readl(i2c->pdmabase+OFFSET_EN)));
     */
    return;
}

static int32_t _i2c_deal_result(mt_i2c *i2c)
{
    int32_t ret = i2c->msg_len;
    uint16_t data_size = 0;
    uint8_t *ptr = i2c->msg_buf;
    bool transfer_error = false;
    long timeout_poll = 0xffff;
#ifdef I2C_EARLY_PORTING
    I2CFUC();
#endif
    if (i2c->poll_en) { /* Poll mode */
        while (timeout_poll > 0) {
            i2c->irq_stat = i2c_readl(i2c, OFFSET_INTR_STAT);
            if (i2c->irq_stat)
                break;
            timeout_poll--;
        }
    } else { /* Interrupt mode, wait for ISR to wake up */
        //I2CFUC();
        timeout_poll = xSemaphoreTake(i2c_irq_semaphore[i2c->id], 3072);
        if (timeout_poll == pdTRUE) {
            i2c->irq_stat = irq_stat[i2c->id];
            irq_stat[i2c->id] = 0; /* clear the irq_stat */
        }
        //I2CFUC();
    }

    /* try to release semaphore */
    if (i2c_release_scp_semaphore(i2c) != 0) {
        return -ETIMEDOUT_I2C;
    }

    if (i2c->irq_stat & (I2C_HS_NACKERR | I2C_ACKERR))
        transfer_error = true;

    /*Check the transfer status */
    if (timeout_poll != 0 && transfer_error == false) {
        /*Transfer success ,we need to get data from fifo,
            only read mode or write_read mode need to get data */
        if ((!i2c->dma_en) && (i2c->op == I2C_MASTER_RD || i2c->op == I2C_MASTER_WRRD)) {
            data_size = (i2c_readl(i2c, OFFSET_FIFO_STAT) >> 4) & 0x000F;
            /* I2CLOG("data_size=%d\n",data_size); */
            while (data_size--) {
                *ptr = i2c_readl(i2c, OFFSET_DATA_PORT);
#ifdef I2C_EARLY_PORTING
                I2CLOG("addr %x read byte = 0x%x\n", i2c->addr, *ptr);
#endif
                ptr++;
            }
        }
    } else {
        /*Timeout or ACKERR */
        if (timeout_poll == 0) {
            I2CERR("id=%d,addr: %x, transfer timeout\n\r", i2c->id, i2c->addr);
            ret = -ETIMEDOUT_I2C;
        } else {
            I2CERR("id=%d,addr: %x, transfer error\n\r", i2c->id, i2c->addr);
            ret = -EREMOTEIO_I2C;
        }
        if (i2c->irq_stat & I2C_HS_NACKERR)
            I2CERR("I2C_HS_NACKERR\n\r");
        if (i2c->irq_stat & I2C_ACKERR)
            I2CERR("I2C_ACKERR\n\r");
        if (i2c->filter_msg == false) { /* TEST */
            _i2c_dump_info(i2c);
        }
        /* spin_lock(&i2c->lock); */
        /*Reset i2c port */
        i2c_writel(i2c, OFFSET_SOFTRESET, 0x0001);
        /*Set slave address */
        i2c_writel(i2c, OFFSET_SLAVE_ADDR, 0x0000);
        /*Clear interrupt status */
        i2c_writel(i2c, OFFSET_INTR_STAT, (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
        /*Clear fifo address */
        i2c_writel(i2c, OFFSET_FIFO_ADDR_CLR, 0x0001);

        /* spin_unlock(&i2c->lock); */
    }
    return ret;
}
#if 0
static int32_t _i2c_deal_result_old(mt_i2c *i2c)
{
    long tmo = 1;
    uint16_t data_size = 0;
    uint8_t *ptr = i2c->msg_buf;
    bool TRANSFER_ERROR = false;
    int32_t ret = i2c->msg_len;
    long tmo_poll = 0xffff;

    I2CFUC();
    /* addr_reg = i2c->read_flag ? ((i2c->addr << 1) | 0x1) : ((i2c->addr << 1) & ~0x1); */

    if (i2c->poll_en) { /*master read && poll mode */
        for (;;) {  /*check the interrupt status register */
            i2c->irq_stat = i2c_readl(i2c, OFFSET_INTR_STAT);
            /* I2CLOG("irq_stat = 0x%x\n", i2c->irq_stat); */
            if (i2c->irq_stat & (I2C_HS_NACKERR | I2C_ACKERR)) {
                /* transfer error */
                /*Clear interrupt status,write 1 clear */
                /* i2c_writel(i2c, OFFSET_INTR_STAT, (I2C_HS_NACKERR | I2C_ACKERR )); */
                TRANSFER_ERROR = true;
                tmo = 1;
                /* spin_unlock(&i2c->lock); */
                break;
            } else if (i2c->irq_stat & I2C_TRANSAC_COMP) {
                /* transfer complete */
                tmo = 1;
                break;
            }
            tmo_poll--;
            if (tmo_poll == 0) {
                tmo = 0;
                break;
            }
        }
    } else {        /*Interrupt mode,wait for interrupt wake up */
        if (xSemaphoreTake(i2c_irq_semaphore[i2c->id], 3072) == pdTRUE) {
            tmo = 1;
            i2c->irq_stat = irq_stat[i2c->id];
            irq_stat[i2c->id] = 0; /* clear the irq_stat */
            if (i2c->irq_stat & (I2C_HS_NACKERR | I2C_ACKERR))
                TRANSFER_ERROR = true;
        } else
            tmo = 0;
    }

    if (i2c_release_scp_semaphore(i2c) != 0) {
        return -ETIMEDOUT_I2C;
    }

    /*Check the transfer status */
    if (tmo != 0 && TRANSFER_ERROR == false) {
        /*Transfer success ,we need to get data from fifo */
        if ((!i2c->dma_en) && (i2c->op == I2C_MASTER_RD
                               || i2c->op == I2C_MASTER_WRRD)) {   /*only read mode or write_read mode and fifo mode need to get data */
            data_size = (i2c_readl(i2c, OFFSET_FIFO_STAT) >> 4) & 0x000F;
            /* I2CLOG("data_size=%d\n",data_size); */
            while (data_size--) {
                *ptr = i2c_readl(i2c, OFFSET_DATA_PORT);
#ifdef I2C_EARLY_PORTING
                I2CLOG("addr %x read byte = 0x%x\n", i2c->addr, *ptr);
#endif
                ptr++;
            }
        }
    } else {
        /*Timeout or ACKERR */
        if (tmo == 0) {
            I2CERR("id=%d,addr: %x, transfer timeout\n", i2c->id, i2c->addr);
            ret = -ETIMEDOUT_I2C;
        } else {
            I2CERR("id=%d,addr: %x, transfer error\n", i2c->id, i2c->addr);
            ret = -EREMOTEIO_I2C;
        }
        if (i2c->irq_stat & I2C_HS_NACKERR)
            I2CERR("I2C_HS_NACKERR\n");
        if (i2c->irq_stat & I2C_ACKERR)
            I2CERR("I2C_ACKERR\n");
        if (i2c->filter_msg == false) { /* TEST */
            _i2c_dump_info(i2c);
        }
        /* spin_lock(&i2c->lock); */
        /*Reset i2c port */
        i2c_writel(i2c, OFFSET_SOFTRESET, 0x0001);
        /*Set slave address */
        i2c_writel(i2c, OFFSET_SLAVE_ADDR, 0x0000);
        /*Clear interrupt status */
        i2c_writel(i2c, OFFSET_INTR_STAT, (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
        /*Clear fifo address */
        i2c_writel(i2c, OFFSET_FIFO_ADDR_CLR, 0x0001);

        /* spin_unlock(&i2c->lock); */
    }
    return ret;
}
#endif

void _i2c_write_reg(mt_i2c *i2c)
{
    uint8_t *ptr = i2c->msg_buf;
    uint32_t data_size = i2c->trans_data.data_size;
    uint32_t addr_reg = 0;
#ifdef I2C_EARLY_PORTING
    I2CFUC();
#endif

    i2c_writel(i2c, OFFSET_CONTROL, i2c->control_reg);

    /*set start condition */
    if (i2c->speed <= 100) {
        i2c_writel(i2c, OFFSET_EXT_CONF, 0x8001);
    }
    /* set timing reg */
    i2c_writel(i2c, OFFSET_TIMING, i2c->timing_reg);
    i2c_writel(i2c, OFFSET_HS, i2c->high_speed_reg);

    if (0 == i2c->delay_len)
        i2c->delay_len = 2;
    if (~i2c->control_reg & I2C_CONTROL_RS) {   /* bit is set to 1, i.e.,use repeated stop */
        i2c_writel(i2c, OFFSET_DELAY_LEN, i2c->delay_len);
    }

    /*Set ioconfig */
    if (i2c->pushpull) {
        i2c_writel(i2c, OFFSET_IO_CONFIG, 0x0000);
    } else {
        i2c_writel(i2c, OFFSET_IO_CONFIG, 0x0003);
    }

    /*Set slave address */

    addr_reg = i2c->read_flag ? ((i2c->addr << 1) | 0x1) : ((i2c->addr << 1) & ~0x1);
    i2c_writel(i2c, OFFSET_SLAVE_ADDR, addr_reg);
    /*Clear interrupt status */
    i2c_writel(i2c, OFFSET_INTR_STAT, (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
    /*Clear fifo address */
    i2c_writel(i2c, OFFSET_FIFO_ADDR_CLR, 0x0001);
    /*Setup the interrupt mask flag */
    if (i2c->poll_en)
        i2c_writel(i2c, OFFSET_INTR_MASK, i2c_readl(i2c,
                   OFFSET_INTR_MASK) & ~(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));    /*Disable interrupt */
    else
        i2c_writel(i2c, OFFSET_INTR_MASK, i2c_readl(i2c,
                   OFFSET_INTR_MASK) | (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP)); /*Enable interrupt */
    /*Set transfer len */
    i2c_writel(i2c, OFFSET_TRANSFER_LEN, i2c->trans_data.trans_len & 0xFFFF);
    i2c_writel(i2c, OFFSET_TRANSFER_LEN_AUX, i2c->trans_data.trans_auxlen & 0xFFFF);
    /*Set transaction len */
    i2c_writel(i2c, OFFSET_TRANSAC_LEN, i2c->trans_data.trans_num & 0xFF);

    /*Prepare buffer data to start transfer */
    if (i2c->dma_en) {
#if 0
#ifdef SCP_I2C_SUPPORT_DMA
        /* Reset I2C DMA status */
        mt_reg_sync_writel(0x0001, i2c->pdmabase + OFFSET_RST);
        if (I2C_MASTER_RD == i2c->op) {
            mt_reg_sync_writel(0x0000, i2c->pdmabase + OFFSET_INT_FLAG);
            mt_reg_sync_writel(0x0001, i2c->pdmabase + OFFSET_CON);
            mt_reg_sync_writel((uint32_t) i2c->msg_buf,
                               i2c->pdmabase + OFFSET_RX_MEM_ADDR);
            mt_reg_sync_writel(i2c->trans_data.data_size,
                               i2c->pdmabase + OFFSET_RX_LEN);
        } else if (I2C_MASTER_WR == i2c->op) {
            mt_reg_sync_writel(0x0000, i2c->pdmabase + OFFSET_INT_FLAG);
            mt_reg_sync_writel(0x0000, i2c->pdmabase + OFFSET_CON);
            mt_reg_sync_writel((uint32_t) i2c->msg_buf,
                               i2c->pdmabase + OFFSET_TX_MEM_ADDR);
            mt_reg_sync_writel(i2c->trans_data.data_size,
                               i2c->pdmabase + OFFSET_TX_LEN);
        } else {
            mt_reg_sync_writel(0x0000, i2c->pdmabase + OFFSET_INT_FLAG);
            mt_reg_sync_writel(0x0000, i2c->pdmabase + OFFSET_CON);
            mt_reg_sync_writel((uint32_t) i2c->msg_buf,
                               i2c->pdmabase + OFFSET_TX_MEM_ADDR);
            mt_reg_sync_writel((uint32_t) i2c->msg_buf,
                               i2c->pdmabase + OFFSET_RX_MEM_ADDR);
            mt_reg_sync_writel(i2c->trans_data.trans_len,
                               i2c->pdmabase + OFFSET_TX_LEN);
            mt_reg_sync_writel(i2c->trans_data.trans_auxlen,
                               i2c->pdmabase + OFFSET_RX_LEN);
        }
        I2C_MB();
        mt_reg_sync_writel(0x0001, i2c->pdmabase + OFFSET_EN);

        I2CINFO(I2C_T_DMA, "addr %.2x dma %.2X byte\n", i2c->addr,
                i2c->trans_data.data_size);
        I2CINFO(I2C_T_DMA, "DMA Register:INT_FLAG:0x%x,CON:0x%x,TX_MEM_ADDR:0x%x, \
         RX_MEM_ADDR:0x%x,TX_LEN:0x%x,RX_LEN:0x%x,EN:0x%x\n", DRV_Reg32(i2c->pdmabase + OFFSET_INT_FLAG),
                DRV_Reg32(i2c->pdmabase + OFFSET_CON), DRV_Reg32(i2c->pdmabase + OFFSET_TX_MEM_ADDR),
                DRV_Reg32(i2c->pdmabase + OFFSET_RX_MEM_ADDR), DRV_Reg32(i2c->pdmabase + OFFSET_TX_LEN),
                DRV_Reg32(i2c->pdmabase + OFFSET_RX_LEN), DRV_Reg32(i2c->pdmabase + OFFSET_EN));
#endif
#endif
    } else {
        /*Set fifo mode data */
        if (I2C_MASTER_RD != i2c->op) { /*both write && write_read mode */
            while (data_size--) {
                i2c_writel(i2c, OFFSET_DATA_PORT, *ptr);
                ptr++;
            }
        }
    }
    /*Set trans_data */
    i2c->trans_data.data_size = data_size;

}

int32_t _i2c_get_transfer_len(mt_i2c *i2c)
{
    int32_t ret = I2C_OK;
    uint16_t trans_num = 0;
    uint16_t data_size = 0;
    uint16_t trans_len = 0;
    uint16_t trans_auxlen = 0;
    /* I2CFUC(); */
    /*Get Transfer len and transaux len */
    if (false == i2c->dma_en) { /*non-DMA mode */
        if (I2C_MASTER_WRRD != i2c->op) {
            trans_len = (i2c->msg_len) & 0xFFFF;
            trans_num = (i2c->msg_len >> 16) & 0xFF;
            if (0 == trans_num)
                trans_num = 1;
            trans_auxlen = 0;
            data_size = trans_len * trans_num;

            if (!trans_len || !trans_num || trans_len * trans_num > I2C_FIFO_SIZE) {
                I2CERR
                (" non-WRRD transfer length is not right. trans_len=%x, tans_num=%x, trans_auxlen=%x\n\r",
                 trans_len, trans_num, trans_auxlen);
                I2C_BUG_ON(!trans_len || !trans_num
                           || trans_len * trans_num > I2C_FIFO_SIZE);
                ret = -EINVAL_I2C;
            }
        } else {
            trans_len = (i2c->msg_len) & 0xFFFF;
            trans_auxlen = (i2c->msg_len >> 16) & 0xFFFF;
            trans_num = 2;
            data_size = trans_len;
            if (!trans_len || !trans_auxlen || trans_len > I2C_FIFO_SIZE
                    || trans_auxlen > I2C_FIFO_SIZE) {
                I2CERR
                (" WRRD transfer length is not right. trans_len=%x, tans_num=%x, trans_auxlen=%x\n\r",
                 trans_len, trans_num, trans_auxlen);
                I2C_BUG_ON(!trans_len || !trans_auxlen || trans_len > I2C_FIFO_SIZE
                           || trans_auxlen > I2C_FIFO_SIZE);
                ret = -EINVAL_I2C;
            }
        }
    } else {        /*DMA mode */
#if 0
#ifdef SCP_I2C_SUPPORT_DMA
        if (I2C_MASTER_WRRD != i2c->op) {
            trans_len = (i2c->msg_len) & 0xFFFF;
            trans_num = (i2c->msg_len >> 16) & 0xFF;
            if (0 == trans_num)
                trans_num = 1;
            trans_auxlen = 0;
            data_size = trans_len * trans_num;

            if (!trans_len || !trans_num || trans_len > MAX_DMA_TRANS_SIZE
                    || trans_num > MAX_DMA_TRANS_NUM) {
                I2CERR
                (" DMA non-WRRD transfer length is not right. trans_len=%x, tans_num=%x, trans_auxlen=%x\n",
                 trans_len, trans_num, trans_auxlen);
                I2C_BUG_ON(!trans_len || !trans_num
                           || trans_len > MAX_DMA_TRANS_SIZE
                           || trans_num > MAX_DMA_TRANS_NUM);
                ret = -EINVAL_I2C;
            }
            I2CINFO(I2C_T_DMA,
                    "DMA non-WRRD mode!trans_len=%x, tans_num=%x, trans_auxlen=%x\n",
                    trans_len, trans_num, trans_auxlen);
        } else {
            trans_len = (i2c->msg_len) & 0xFFFF;
            trans_auxlen = (i2c->msg_len >> 16) & 0xFFFF;
            trans_num = 2;
            data_size = trans_len;
            if (!trans_len || !trans_auxlen || trans_len > MAX_DMA_TRANS_SIZE
                    || trans_auxlen > MAX_DMA_TRANS_NUM) {
                I2CERR
                (" DMA WRRD transfer length is not right. trans_len=%x, tans_num=%x, trans_auxlen=%x\n",
                 trans_len, trans_num, trans_auxlen);
                I2C_BUG_ON(!trans_len || !trans_auxlen
                           || trans_len > MAX_DMA_TRANS_SIZE
                           || trans_auxlen > MAX_DMA_TRANS_NUM);
                ret = -EINVAL_I2C;
            }
            I2CINFO(I2C_T_DMA,
                    "DMA WRRD mode!trans_len=%x, tans_num=%x, trans_auxlen=%x\n",
                    trans_len, trans_num, trans_auxlen);
        }
#endif
#endif
    }

    i2c->trans_data.trans_num = trans_num;
    i2c->trans_data.trans_len = trans_len;
    i2c->trans_data.data_size = data_size;
    i2c->trans_data.trans_auxlen = trans_auxlen;

    return ret;
}

int32_t _i2c_transfer_interface(mt_i2c *i2c)
{
    int32_t return_value = 0;
    int32_t ret = 0;
    uint8_t *ptr = i2c->msg_buf;

    /* I2CFUC(); */
    i2c_clock_enable();

    if (i2c->dma_en) {
        I2CINFO(I2C_T_DMA, "DMA Transfer mode!\n");
        if (i2c->pdmabase == 0) {
            I2CERR(" I2C%d doesnot support DMA mode!\n\r", i2c->id);
            I2C_BUG_ON(i2c->pdmabase == NULL);
            ret = -EINVAL_I2C;
            goto err;
        }
        if ((uint32_t) ptr > DMA_ADDRESS_HIGH) {
            I2CERR(" DMA mode should use physical buffer address!\n\r");
            I2C_BUG_ON((uint32_t) ptr > DMA_ADDRESS_HIGH);
            ret = -EINVAL_I2C;
            goto err;
        }
    }
    i2c->irq_stat = 0;

    return_value = _i2c_get_transfer_len(i2c);
    if (return_value < 0) {
        I2CERR("_i2c_get_transfer_len fail,return_value=%ld\n\r", return_value);
        ret = -EINVAL_I2C;
        goto err;
    }
    /* get clock */
    i2c->clk = I2C_CLK_RATE;

    /*
    return_value = i2c_set_speed(i2c);
    if (return_value < 0) {
        I2CERR("i2c_set_speed fail,return_value=%ld\n\r", return_value);
        ret = -EINVAL_I2C;
        goto err;
    } */
    /* hard code i2c speed to 400KHz */
    if (HS_MODE == i2c->mode) {
        if (get_cur_clk() == CLK_354M) {
            i2c->timing_reg = 0x1017;
            i2c->high_speed_reg = 0x3503;
        } else {
            i2c->timing_reg = 0x100e;
            i2c->high_speed_reg = 0x2403;
        }
    } else {
        if (get_cur_clk() == CLK_354M)
            i2c->timing_reg = 0x1017;
        else
            i2c->timing_reg = 0x100e;
        i2c->high_speed_reg = 0x102;
    }
#if 0
    if (HS_MODE == i2c->mode) {
        if (get_cur_clk() == CLK_354M) {
            i2c->timing_reg = 0x101b;
            i2c->high_speed_reg = 0x3603;
        } else {
            i2c->timing_reg = 0x1011;
            i2c->high_speed_reg = 0x2503;
        }
    } else {
        if (get_cur_clk() == CLK_354M)
            i2c->timing_reg = 0x101b;
        else
            i2c->timing_reg = 0x1011;
        i2c->high_speed_reg = 0x102;
    }
#endif

    /*Set Control Register */
#ifdef CONFIG_MT_I2C_FPGA_ENABLE
    i2c->control_reg = I2C_CONTROL_ACKERR_DET_EN;
#else
    i2c->control_reg = I2C_CONTROL_ACKERR_DET_EN | I2C_CONTROL_CLK_EXT_EN;
#endif
    if (i2c->dma_en) {
        i2c->control_reg |= I2C_CONTROL_DMA_EN;
    }
    if (I2C_MASTER_WRRD == i2c->op)
        i2c->control_reg |= I2C_CONTROL_DIR_CHANGE;

    if (HS_MODE == i2c->mode
            || (i2c->trans_data.trans_num > 1 && I2C_TRANS_REPEATED_START == i2c->st_rs)) {
        i2c->control_reg |= I2C_CONTROL_RS;
    }

    /* Use HW semaphore for inter-chip access protection */
    if (i2c_get_scp_semaphore(i2c) != 0) {
        ret = -EINVAL_I2C;
        goto err;
    }

    _i2c_write_reg(i2c);

    /*All register must be prepared before setting the start bit [SMP] */
    I2C_MB();
    /* I2CLOG(I2C_T_TRANSFERFLOW, "Before start .....\n"); */
    /*Start the transfer */
    i2c_writel(i2c, OFFSET_START, 0x0001);
    /* spin_unlock(&i2c->lock); */
    ret = _i2c_deal_result(i2c);
    /* I2CINFO(I2C_T_TRANSFERFLOW, "After i2c transfer .....\n");*/

err:
    i2c_clock_disable();
    return ret;
}

int32_t _i2c_check_para(mt_i2c *i2c)
{
    int32_t ret = 0;

    /* I2CFUC(); */
    if (i2c->addr == 0) {
        I2CERR(" addr is invalid.\n\r");
        I2C_BUG_ON(i2c->addr == NULL);
        ret = -EINVAL_I2C;
        goto err;
    }

    if (i2c->msg_buf == NULL) {
        I2CERR(" data buffer is NULL.\n\r");
        I2C_BUG_ON(i2c->msg_buf == NULL);
        ret = -EINVAL_I2C;
        goto err;
    }

    if (i2c->id >= I2C_NR) {
        I2CERR("invalid para: i2c->id=%d\n\r", i2c->id);
        ret = -EINVAL_I2C;
    }

err:
    return ret;

}

void _config_mt_i2c(mt_i2c *i2c)
{
    /* I2CFUC(); */
    switch (i2c->id) {
        case 0:
            i2c->base = 0x400A5000;
            break;
        case 1:
            i2c->base = 0x400A6000;
            break;
        default:
            I2CERR("invalid para: i2c->id=%d\n\r", i2c->id);
            break;
    }
    if (i2c->st_rs == I2C_TRANS_REPEATED_START)
        i2c->st_rs = I2C_TRANS_REPEATED_START;
    else
        i2c->st_rs = I2C_TRANS_STOP;
#if 0
    if (i2c->dma_en == true)
        i2c->dma_en = true;
    else
        i2c->dma_en = false;
#else
    i2c->dma_en = false;
#endif
    i2c->poll_en = true;
#if 0
    if (i2c->filter_msg == true)
        i2c->filter_msg = true;
    else
#endif
        i2c->filter_msg = false;

    /* Set device speed,set it before set_control register */
    if (0 == i2c->speed) {
        i2c->mode = ST_MODE;
        i2c->speed = MAX_ST_MODE_SPEED;
    } else {
        if (i2c->mode == HS_MODE)
            i2c->mode = HS_MODE;
        else
            i2c->mode = FS_MODE;
    }

    /*Set ioconfig */
    if (i2c->pushpull == true)
        i2c->pushpull = true;
    else
        i2c->pushpull = false;

}

/*-----------------------------------------------------------------------
 * new read interface: Read bytes
 *   mt_i2c:    I2C chip config, see mt_i2c_t.
 *   buffer:  Where to read/write the data.
 *   len:     How many bytes to read/write
 *   Returns: ERROR_CODE
 */
int32_t i2c_read(mt_i2c *i2c, uint8_t *buffer, uint32_t len)
{
    int32_t ret = I2C_OK;
#ifdef I2C_EARLY_PORTING
    I2CFUC();
#endif
    /* read */
    i2c->read_flag |= I2C_M_RD;
    i2c->op = I2C_MASTER_RD;
    i2c->msg_buf = buffer;
    i2c->msg_len = len;

    ret = _i2c_check_para(i2c);
    if (ret < 0) {
        I2CERR(" _i2c_check_para fail\n\r");
        goto err;
    }

    /* Use mutex for I2C controller protection */
    if (xSemaphoreTake(i2c_mutex[i2c->id], portMAX_DELAY) != pdTRUE) {
        I2CERR("get mutex fail\n\r");
        goto mutexError;
    }

    _config_mt_i2c(i2c);
    /* get the addr */
    ret = _i2c_transfer_interface(i2c);

    if ((int)i2c->msg_len != ret) {
        I2CERR("read %ld bytes fails,ret=%ld.\n\r", i2c->msg_len, ret);
        ret = -1;
    } else {
        ret = I2C_OK;
        /* I2CLOG("read %d bytes pass,ret=%d.\n",i2c->msg_len,ret); */
    }

    xSemaphoreGive(i2c_mutex[i2c->id]);

mutexError:
err:
    return ret;
}

/*-----------------------------------------------------------------------
 * New write interface: Write bytes
 *   i2c:    I2C chip config, see mt_i2c_t.
 *   buffer:  Where to read/write the data.
 *   len:     How many bytes to read/write
 *   Returns: ERROR_CODE
 */
int32_t i2c_write(mt_i2c *i2c, uint8_t *buffer, uint32_t len)
{
    int32_t ret = I2C_OK;
#ifdef I2C_EARLY_PORTING
    I2CFUC();
#endif
    /* write */
    i2c->read_flag = !I2C_M_RD;
    i2c->op = I2C_MASTER_WR;
    i2c->msg_buf = buffer;
    i2c->msg_len = len;

    ret = _i2c_check_para(i2c);
    if (ret < 0) {
        I2CERR(" _i2c_check_para fail\n\r");
        goto err;
    }

    /* Use mutex for I2C controller protection */
    if (xSemaphoreTake(i2c_mutex[i2c->id], portMAX_DELAY) != pdTRUE) {
        I2CERR("get mutex fail\n\r");
        goto mutexError;
    }

    _config_mt_i2c(i2c);
    /* get the addr */
    ret = _i2c_transfer_interface(i2c);

    if ((int)i2c->msg_len != ret) {
        I2CERR("Write %ld bytes fails,ret=%ld.\n\r", i2c->msg_len, ret);
        ret = -1;
    } else {
        ret = I2C_OK;
        /* I2CLOG("Write %d bytes pass,ret=%d.\n",i2c->msg_len,ret); */
    }

    xSemaphoreGive(i2c_mutex[i2c->id]);

mutexError:
err:
    return ret;
}

/*-----------------------------------------------------------------------
 * New write then read back interface: Write bytes then read bytes
 *   i2c:    I2C chip config, see mt_i2c_t.
 *   buffer:  Where to read/write the data.
 *   write_len:     How many bytes to write
 *   read_len:     How many bytes to read
 *   Returns: ERROR_CODE
 */
int32_t i2c_write_read(mt_i2c *i2c, uint8_t *buffer, uint32_t write_len, uint32_t read_len)
{
    int32_t ret = I2C_OK;
    /* I2CFUC(); */
    /* write and read */
    i2c->op = I2C_MASTER_WRRD;
    i2c->read_flag = !I2C_M_RD;
    i2c->msg_buf = buffer;
    i2c->msg_len = ((read_len & 0xFFFF) << 16) | (write_len & 0xFFFF);

    ret = _i2c_check_para(i2c);
    if (ret < 0) {
        I2CERR(" _i2c_check_para fail\n\r");
        goto err;
    }

    /* Use mutex for I2C controller protection */
    if (xSemaphoreTake(i2c_mutex[i2c->id], portMAX_DELAY) != pdTRUE) {
        I2CERR("get mutex fail\n\r");
        goto mutexError;
    }

    _config_mt_i2c(i2c);
    /* get the addr */
    ret = _i2c_transfer_interface(i2c);

    if ((int)i2c->msg_len != ret) {
        I2CERR("write_read 0x%lx bytes fails,ret=%ld.\n\r", i2c->msg_len, ret);
        ret = -1;
    } else {
        ret = I2C_OK;
        /* I2CLOG("write_read 0x%x bytes pass,ret=%d.\n",i2c->msg_len,ret); */
    }

    xSemaphoreGive(i2c_mutex[i2c->id]);

mutexError:
err:
    return ret;
}

static void i2c0_irq_handler(void)
{
    /* I2CLOG("I2C0, interrupt is coming!!\n"); */
    struct mt_i2c_t mi2c;
    portBASE_TYPE taskWoken = pdFALSE;
    struct mt_i2c_t *i2c = &mi2c;
    i2c->id = 0;
    _config_mt_i2c(i2c);
    i2c_writel(i2c, OFFSET_INTR_MASK, i2c_readl(i2c, OFFSET_INTR_MASK) & ~(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
    irq_stat[0] = i2c_readl(i2c, OFFSET_INTR_STAT);
    i2c_writel(i2c, OFFSET_INTR_STAT, (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
    xSemaphoreGiveFromISR(i2c_irq_semaphore[i2c->id], &taskWoken);
}

static void i2c1_irq_handler(void)
{
    /* I2CLOG("I2C1, interrupt is coming!!\n"); */
    struct mt_i2c_t mi2c;
    portBASE_TYPE taskWoken = pdFALSE;
    struct mt_i2c_t *i2c = &mi2c;
    i2c->id = 1;
    _config_mt_i2c(i2c);
    i2c_writel(i2c, OFFSET_INTR_MASK, i2c_readl(i2c, OFFSET_INTR_MASK) & ~(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
    irq_stat[1] = i2c_readl(i2c, OFFSET_INTR_STAT);
    i2c_writel(i2c, OFFSET_INTR_STAT, (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
    xSemaphoreGiveFromISR(i2c_irq_semaphore[i2c->id], &taskWoken);
}

int32_t i2c_hw_init(void)
{
    int i = 0;

    for (i = 0; i < I2C_NR; i++) {
        i2c_mutex[i] = xSemaphoreCreateMutex();
        i2c_irq_semaphore[i] = xSemaphoreCreateCounting(10, 0);
    }

    request_irq(I2C0_IRQn, i2c0_irq_handler, "I2C0");
    request_irq(I2C1_IRQn, i2c1_irq_handler, "I2C1");

    I2CLOG("fwq i2c init\n");

    return 0;
}

/* Test I2C */
#ifdef I2C_EARLY_PORTING
int mt_i2c_test_eeprom(int id)
{
    uint32_t ret = 0;
    uint32_t len = 0;
    unsigned char write_byte[2], read_byte[2];
    struct mt_i2c_t i2c;

    i2c.id = id;
    i2c.addr = 0x50;
    i2c.mode = FS_MODE;
    i2c.speed = 100;
    /* ================================================== */
    I2CLOG("test i2c write=======================================>liujiang\n");
    write_byte[0] = 0x20;
    write_byte[1] = 0x55;
    len = 2;
    ret = i2c_write(&i2c, write_byte, len);
    if (I2C_OK != ret) {
        I2CERR("Write 2 bytes fails(%lx).\n\r", ret);
        ret = -1;
        return ret;
    } else {
        I2CLOG("Write 2 bytes pass,these bytes are %x, %x.\n\r", write_byte[0],
               write_byte[1]);
    }

    //mdelay(100);
    /* ================================================== */
    I2CLOG("test i2c read\n\r");
    /* 1st:write addree 00,1byte(0x0A) */
    write_byte[0] = 0x20;
    write_byte[1] = 0x0A;
    len = 1;
    ret = i2c_write(&i2c, write_byte, len);

    //mdelay(100);

    read_byte[0] = 0x20;
    len = 1;
    ret = i2c_read(&i2c, read_byte, len);
    if ((I2C_OK != ret) && read_byte[0] == write_byte[1]) {
        I2CERR("read 2 bytes fails(%lx).\n\r", ret);
        ret = -1;
        return ret;
    } else {
        I2CLOG("read 2 bytes pass,read_byte=%x,write_byte= %x.\n", read_byte[0],
               write_byte[1]);
    }

    //mdelay(100);
    /* ================================================== */
    I2CLOG("test i2c write_read\n");
    write_byte[0] = 0x20;
    /* write_byte[1] = 0x34; */
    len = ((1 & 0xFFFF) << 16) | (1 & 0xFFFF);
    ret = i2c_write_read(&i2c, write_byte, 1, 1);
    if (I2C_OK != ret) {
        I2CERR("Write 1 byte fails(ret=%lx).\n\r", ret);
        ret = -1;
        return ret;
    } else {
        I2CLOG("Read 1 byte pass ,this byte is %x.\n\r", write_byte[0]);
        ret = 0;
    }
    return ret;
}

uint32_t mt_i2c_test_eeprom1(int id)
{
    uint32_t ret = 0;
    uint32_t len = 0;
    uint32_t delay_count = 0xff;
    uint8_t write_byte[2], read_byte[2];
    struct mt_i2c_t i2c;

    I2CLOG("i2c %d test start++++++++++++++++++++\n", id);


    i2c.id = id;
    i2c.addr = 0x50;
    i2c.mode = ST_MODE;
    i2c.speed = 100;
    /* ================================================== */
    I2CLOG("\ntest i2c write\n\n");
    write_byte[0] = 0x20;
    write_byte[1] = 0x55;
    len = 2;
    ret = i2c_write(&i2c, write_byte, len);
    if (I2C_OK != ret) {
        I2CERR("Write 2 bytes fails(%lx).\n\r", ret);
        ret = -1;
        return ret;
    } else {
        I2CLOG("Write 2 bytes pass,these bytes are %x, %x.\n", write_byte[0],
               write_byte[1]);
    }
    /* mdelay(1000); */
    for (delay_count = 0xff; delay_count > 0; delay_count--) ;
    /* ================================================== */
    I2CLOG("\ntest i2c read\n\n");
    //1st:write addree 00,1byte(0x0A)
    write_byte[0] = 0x20;
    len = 1;
    ret = i2c_write(&i2c, write_byte, len);
    if (I2C_OK != ret) {
        I2CERR("Write 1 bytes fails(%lx).\n\r", ret);
        ret = -1;
        return ret;
    } else {
        I2CLOG("Write 1 bytes pass,these bytes are %x.\n", write_byte[0]);
    }

    /* mdelay(1000); */
    for (delay_count = 0xff; delay_count > 0; delay_count--) ;
    /* 2rd:read back 1byte(0x0A) */
    read_byte[0] = 0x33;
    len = 1;
    ret = i2c_read(&i2c, read_byte, len);
    if ((I2C_OK != ret) || read_byte[0] != write_byte[1]) {
        I2CERR("read 1 bytes fails(%lx).\n\r", ret);
        I2CLOG("liujiang ----> read 1 bytes ,read_byte=%x\n\r", read_byte[0]);
        ret = -1;
        return ret;
    } else {
        I2CLOG("read 1 bytes pass,read_byte=%x\n\r", read_byte[0]);
    }

    /* mdelay(1000); */
    for (delay_count = 0xff; delay_count > 0; delay_count--) ;
    /* ================================================== */
    I2CLOG("\ntest i2c write_read\n\r");
    read_byte[0] = 0x20;
    /* write_byte[1] = 0x34; */
    len = (1 & 0xFF) << 8 | (1 & 0xFF);
    ret = i2c_write_read(&i2c, read_byte, 1, 1);
    if (I2C_OK != ret || read_byte[0] != write_byte[1]) {
        I2CERR("write_read 1 byte fails(ret=%lx).\n\r", ret);
        I2CLOG("write_read 1 byte fails, read_byte=%x\n\r", read_byte[0]);
        ret = -1;
        return ret;
    } else {
        I2CLOG("Write_Read 1 byte pass ,this byte is %x.\n", read_byte[0]);
        ret = 0;
    }
    I2CLOG("i2c %d test done-------------------\n", id);
    return ret;
}

uint32_t mt_i2c_test_alsps(int id, int addr, int offset, int value)
{
    uint32_t ret = 0;
    uint32_t len = 0;
    uint8_t write_byte[2], read_byte[2];
    uint32_t delay_count = 0xff;
    struct mt_i2c_t i2c;
    //int i = 0;

    I2CLOG("i2c %d test start++++++++++++++++++++\n", id);


    i2c.id = id;
    i2c.addr = addr;
    i2c.mode = FS_MODE;
    i2c.speed = 200;
    /* i2c.mode = FS_MODE;
    i2c.speed = 200;*/
    /* ================================================== */

    I2CLOG("\ntest i2c write\n\n");
    write_byte[0] = offset;
    write_byte[1] = value;
    len = 2;
    ret = i2c_write(&i2c, write_byte, len);
    if (I2C_OK != ret) {
        I2CERR("Write 2 bytes fails(%lx).\n\r", ret);
        ret = -1;
        return ret;
    } else {
        I2CLOG("Write 2 bytes pass,these bytes are %x, %x.\n\r", write_byte[0],
               write_byte[1]);
    }

    /* mdelay(1000); */

    for (delay_count = 0xff; delay_count > 0; delay_count--) ;
    /* ================================================== */
    I2CLOG("\ntest i2c read\n\n");
    //1st:write addree 00,1byte(0x0A)
    //write_byte[0] = 0x0e;
    write_byte[0] = offset;

    len = 1;
    ret = i2c_write(&i2c, write_byte, len);
    if (I2C_OK != ret) {
        I2CERR("Write 1 bytes fails(%lx).\n\r", ret);
        ret = -1;
        return ret;
        //continue;
    } else {
        I2CLOG("Write 1 bytes pass,these bytes are %x.\n\r", write_byte[0]);
    }
    /* mdelay(1000); */
    for (delay_count = 0xff; delay_count > 0; delay_count--) ;
    /* 2rd:read back 1byte(0x0A) */
    read_byte[0] = 0x55;
    len = 1;
    ret = i2c_read(&i2c, read_byte, len);
    if ((I2C_OK != ret) || read_byte[0] != value) {
        I2CERR("read 1 bytes fails(%lx).\n\r", ret);
        I2CLOG("liujiang ----> read 1 bytes ,read_byte=%x\n\r", read_byte[0]);
        ret = -1;
        return ret;
        //continue;
    } else {
        I2CLOG("read 1 bytes pass,read_byte=%x\n\r", read_byte[0]);
    }

    /* mdelay(1000); */

    for (delay_count = 0xff; delay_count > 0; delay_count--) ;
    /* ================================================== */
    I2CLOG("\ntest i2c write_read\n\r");
    read_byte[0] = offset;
    /* write_byte[1] = 0x34; */
    len = (1 & 0xFF) << 8 | (1 & 0xFF);
    ret = i2c_write_read(&i2c, read_byte, 1, 1);
    if (I2C_OK != ret || read_byte[0] != value) {
        I2CERR("write_read 1 byte fails(ret=%lx).\n\r", ret);
        I2CLOG("write_read 1 byte fails, read_byte=%x\n\r", read_byte[0]);
        ret = -1;
        return ret;
    } else {
        I2CLOG("Write_Read 1 byte pass ,this byte is %x.\n\r", read_byte[0]);
        ret = 0;
    }

    I2CLOG("i2c %d test done-------------------\n\r", id);
    return ret;
}

int32_t mt_i2c_test(void)
{
    int ret;
    int i = 0;

    for (i = 0; i < 1; i++) {
        //ret = mt_i2c_test_eeprom(i);
        ret = mt_i2c_test_alsps(i, 0x0c, 0x31, 0x03);
        if (0 == ret) {
            I2CLOG("I2C%d,EEPROM test PASS!!\n\r", i);
        } else {
            I2CLOG("I2C%d,EEPROM test FAIL!!(%d)\n\r", i, ret);
        }
    }
    return 0;
}
#endif
