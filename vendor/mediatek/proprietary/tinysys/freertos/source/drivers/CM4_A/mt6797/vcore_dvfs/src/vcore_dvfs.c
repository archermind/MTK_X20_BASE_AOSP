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

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <driver_api.h>
#include <interrupt.h>
#include <mt_reg_base.h>
#include <scp_sem.h>
#include <platform.h>

#include <scp_ipi.h>
#include <semphr.h>
#include <vcore_dvfs.h>
#include <utils.h>
#include <sys/types.h>
#include <mt_gpt.h>
#include <feature_manager.h>
#include <tinysys_config.h>
#include <mt_uart.h>
#include "pmic_wrap.h"

#define bool int
#define true 1
#define false 0
#define DVFS_DEBUG 1

#if DVFS_DEBUG
#define DVFSTAG                "[DVFS-SCP] "
#define DVFSLOG(fmt, arg...)        \
    do {            \
        if (g_scp_dvfs_debug)   \
            PRINTF_D(DVFSTAG"-INFO " fmt, ##arg);   \
    } while (0)
#define DVFSDBG(fmt, arg...)        \
    do {            \
        if (g_scp_dvfs_debug)          \
            PRINTF_W(DVFSTAG"-DEBUG " fmt, ##arg);   \
    } while (0)
#define DVFSERR(fmt, arg...)        \
    do {            \
        if (g_scp_dvfs_debug)   \
            PRINTF_E(DVFSTAG"-ERROR " "%d: "fmt, __LINE__, ##arg);  \
    } while (0)
#define DVFSFUC(...)
#else

#define DVFSLOG(...)
#define DVFSDBG(...)
#define DVFSERR(...)
#define DVFSFUC(...)

#endif

#define SWITCH_PMICW_CLK_BY_SPM
//#define ENABLE_PMICW_TEST

#ifdef SWITCH_PMICW_CLK_BY_SPM
#define PMICW_SLEEP_REQ_BIT     0
#define PMICW_SLEEP_ACK_BIT     4
#endif

#ifdef ENABLE_PMICW_TEST
#define PMIC_ID_ADDR            0x200
#define PMIC_ID_VAL             0x5120
#define MAX_PMICW_TEST_ROUND    1
#endif

#undef DRV_Reg32
#undef DRV_SetReg32
#undef DRV_ClrReg32
#define DRV_Reg32(addr)             (*(volatile unsigned int *)(addr))
#define DRV_WriteReg32(addr,data)   ((*(volatile unsigned int *)(addr)) = (unsigned int)(data))
#define DRV_SetReg32(addr, data)    ((*(volatile unsigned int *)(addr)) |= (unsigned int)(data))
#define DRV_ClrReg32(addr, data)    ((*(volatile unsigned int *)(addr)) &= ~((unsigned int)(data)))

#ifdef DVFS_TIMER_PROFILING
/*
 * global variables
 */
volatile uint32_t *ITM_CONTROL = (uint32_t *)0xE0000E80;
volatile uint32_t *DWT_CONTROL = (uint32_t *)0xE0001000;
volatile uint32_t *DWT_CYCCNT = (uint32_t *)0xE0001004;
volatile uint32_t *DEMCR = (uint32_t *)0xE000EDFC;

#endif

//#define TIMER_CLK_SEL 0x400a3060
//#define CHK_ROSC_CLOCK

//0x1f: 170MHz, 0x2f: 208MHz
#define RG_OSC_CALI 0x2f

#define PLL_PWR_ON  (0x1 << 0)
#define PLL_ISO_EN  (0x1 << 1)

#define DVFS_STATUS_OK                               0
#define DVFS_STATUS_BUSY                         -1
#define DVFS_REQUEST_SAME_CLOCK      -2
#define DVFS_STATUS_ERR                            -3
#define DVFS_STATUS_TIMEOUT                 -4
#define DVFS_CLK_ERROR                              -5
#define DVFS_STATUS_CMD_FIX                  -6
#define DVFS_STATUS_CMD_LIMITED        -7
#define DVFS_STATUS_CMD_DISABLE        -8

#if !defined  (HSULP2_VALUE)
#define HSULP2_VALUE    ((uint32_t)122000000) /*!< Default value of the External oscillator in Hz */
#endif /* HSE_VALUE */
#if !defined  (HSULP_VALUE)
#define HSULP_VALUE    ((uint32_t)224000000) /*!< Default value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSPLL_VALUE)
#define HSPLL_VALUE    ((uint32_t)354000000) /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

static bool g_scp_dvfs_debug = false;
static uint32_t g_clk_sys_req_cnt = 0;
static uint32_t g_clk_high_req_cnt = 0;
/* Clock Gating variables */
static uint32_t g_dfs_cur_opp = CLK_UNKNOWN;
static uint32_t g_irq_enable_state = 0;
static bool g_fix_clk_sw_flag = false;
static clk_sel_val g_fix_clk_sw_val = CLK_UNKNOWN;
static bool g_fix_clk_div_flag = false;
static clk_div_enum g_fix_clk_div_val = CLK_DIV_UNKNOWN;
static clk_sel_val g_limited_clk_sw_val = CLK_UNKNOWN;
static bool g_limited_clk_sw_flag = false;
static bool g_disable_dvfs_flag = false;
static bool g_scp_sleep_flag = false;
SemaphoreHandle_t dvfs_mutex;/* For semaphore handling */
SemaphoreHandle_t dvfs_irq_semaphore;/* For interrupt handling */
static uint32_t ap_resource_req_cnt = 0;
static uint32_t dma_user_list = 0;
static uint32_t infra_req_cnt = 0;
static uint32_t infra_user_list = 0;
bool spm_start = false;
static bool isr_leave_flag = false;
static bool dvfs_first_init = false;
static uint32_t enter_sleep_mode;
int g_sleep_flag = 0;
static uint32_t g_sleep_cnt = 0;
//static uint32_t pre_sleep_cnt = 0;
#ifdef DVFS_TIMER_PROFILING
TickType_t time_start_1[30], time_end_1[30], time_start_2[30], time_end_2[30];
uint32_t time_cnt = 0;
#endif
static bool force_clk_sys_on = false;
static uint32_t wakeup_src_mask = 0x000fe05f;
uint32_t g_wakeup_src = 0;
static uint32_t turn_on_high_flag = 0;
static uint32_t turn_on_sys_flag = 0;

clk_enum get_cur_clk(void);
void clkctrl_polling(enum clk_irq_ack irq_ack);
void clkctrl_isr(void);

/*****************************************
 * ADB CMD Control APIs
 ****************************************/
voltage_enum get_cur_volt(void)
{
    DVFSFUC();

    uint32_t cur_voltage;

    cur_voltage = DRV_Reg32(SPM_SW_RSV_3) & 0x3;
    return cur_voltage;
}

void dvfs_debug_set(int id, void* data, unsigned int len)
{
    uint8_t *msg;

    msg = (uint8_t *)data;

    DVFSLOG("SCP Debug IPIMSG Handler:%x %x\n", *msg, len);

    if (*msg) {
        g_scp_dvfs_debug = true;
        DVFSLOG("debug on\n");
    } else {
        g_scp_dvfs_debug = false;
        DVFSLOG("debug off\n");
    }
}

#if 1
uint32_t get_fix_clk_sw_select(void)
{
    uint32_t ret_val;
    DVFSFUC();
    ret_val = g_fix_clk_sw_val;

    return ret_val;
}

void set_fix_clk_sw_select(int id, void* data, unsigned int len)
{
    uint8_t *msg;

    msg = (uint8_t *)data;

    DVFSLOG("SCP fix clock IPIMSG Handler:%x %x\n", *msg, len);
    g_fix_clk_sw_val = (clk_div_enum) * msg;
}

uint32_t get_fix_clk_enable(void)
{
    uint32_t ret_val;
    DVFSFUC();
    ret_val = g_fix_clk_sw_flag;
    return ret_val;
}

void set_fix_clk_enable(int id, void *data, unsigned int len)
{
    uint8_t *msg;

    DVFSFUC();
    msg = (uint8_t *)data;

    if (*msg) {
        g_fix_clk_sw_flag = true;
    } else {
        g_fix_clk_sw_flag = false;
    }
}

uint32_t get_limited_clk_sw_select(void)
{
    uint32_t ret_val;
    DVFSFUC();
    ret_val = g_limited_clk_sw_val;

    return ret_val;
}

void set_limited_clk_sw_select(int id, void *data, unsigned int len)
{
    uint8_t *msg;

    DVFSFUC();
    msg = (uint8_t *)data;

    g_limited_clk_sw_val = (clk_div_enum) * msg;
}

uint32_t get_limited_clk_enable(void)
{
    uint32_t ret_val;
    DVFSFUC();
    ret_val = g_limited_clk_sw_flag;

    return ret_val;
}

void set_limited_clk_enable(int id, void *data, unsigned int len)
{
    uint8_t *msg;

    DVFSFUC();
    msg = (uint8_t *)data;

    if (*msg) {
        g_limited_clk_sw_flag = true;
    } else {
        g_fix_clk_sw_flag = false;
    }
}

void get_dvfs_info_dump(void)
{
#if 0
    uint32_t info_buf[7];

    info_buf[0] = get_cur_clk();
    info_buf[1] = get_fix_clk_sw_select();
    info_buf[2] = get_fix_clk_enable();
    info_buf[3] = get_limited_clk_sw_select();
    info_buf[4] = get_limited_clk_enable();
    info_buf[5] = get_cur_volt();
    info_buf[6] = g_disable_dvfs_flag;
    //scp_ipi_send(IPI_DVFS_INFO_DUMP, (void *)&info_buf, sizeof(info_buf), 0, IPI_SCP2AP);
#endif
}

void get_scp_state(int id, void *data, unsigned int len)
{
    uint32_t ret_val;
    ret_val = DRV_Reg32(SLEEP_DEBUG);

    DVFSLOG("sleep_debug = 0x%x\n", ret_val);
}

#endif
/*****************************************
 * Clock Control APIs
 ****************************************/

/* select clock source */
void clk_sw_select(clk_sel_val val)
{
    DVFSLOG("clk_sw_select(0x%x)\n", val);

    DRV_WriteReg32(CLK_SW_SEL, val);
}

uint32_t get_clk_sw_select(void)
{
    int32_t i, clk_val = 0;

    clk_val = DRV_Reg32(CLK_SW_SEL) >> CKSW_SEL_O;

    for (i = 0; i < 3; i++) {
        if (clk_val == 1 << i) {
            break;
        }
    }

    return i;
}

void clk_div_select(uint32_t val)
{
    DVFSLOG("clk_div_select(0x%x)\n", val);
    if (g_fix_clk_div_flag) {
        DRV_WriteReg32(CLK_DIV_SEL, g_fix_clk_div_val);
    } else {
        DRV_WriteReg32(CLK_DIV_SEL, val);
    }
}

void fix_clk_div_select(uint32_t val)
{
    DVFSLOG("fix_clk_div_select(0x%x)\n", val);
    kal_taskENTER_CRITICAL();
    g_fix_clk_div_flag = true;
    g_fix_clk_div_val = (clk_div_enum) val;
    kal_taskEXIT_CRITICAL();
}

void release_clk_div_select(void)
{
    DVFSFUC();
    kal_taskENTER_CRITICAL();
    g_fix_clk_div_flag = false;
    g_fix_clk_div_val = (clk_div_enum) CLK_DIV_UNKNOWN;
    kal_taskEXIT_CRITICAL();
}

uint32_t get_clk_div_select(void)
{
    int32_t i, div_val = 0;
    DVFSFUC();

    div_val = DRV_Reg32(CLK_DIV_SEL) >> CKSW_DIV_SEL_O;

    for (i = 0; i < CLK_DIV_UNKNOWN; i++) {
        if (div_val == 1 << i) {
            break;
        }
    }

    return i;
}


clk_enum get_cur_clk(void)
{
    DVFSFUC();

    uint32_t cur_clk, cur_div, clk_enable;

    cur_clk = get_clk_sw_select();
    cur_div = get_clk_div_select();
    clk_enable = DRV_Reg32(CLK_ENABLE);
    //DVFSLOG("cur_clk = %d, cur_div = %d, clk_enable = 0x%x\n", cur_clk, cur_div, clk_enable);
    if (cur_clk == CLK_SEL_SYS && cur_div == CLK_DIV_1 && (clk_enable & (1 << CLK_SYS_EN_BIT)) != 0) {
        return CLK_354M;
    } else if (cur_clk == CLK_SEL_HIGH && cur_div == CLK_DIV_1 && (clk_enable & (1 << CLK_HIGH_EN_BIT)) != 0) {
        return CLK_224M;
    } else if (cur_clk == CLK_SEL_HIGH && cur_div == CLK_DIV_2 && (clk_enable & (1 << CLK_HIGH_EN_BIT)) != 0) {
        return CLK_112M;
    } else {
        DVFSERR("clk setting error (%d, %d)\n", cur_clk, cur_div);
        return CLK_UNKNOWN;
    }
}

void turn_off_clk_high(void)
{
    DVFSFUC();

    //kal_taskENTER_CRITICAL();

    /* Turn off clk high output */
    DRV_ClrReg32(CLK_ENABLE, (1 << CLK_HIGH_CG_BIT));
    /* Turn off sys Clock Source */
    DRV_ClrReg32(CLK_ENABLE, (1 << CLK_HIGH_EN_BIT));
    DVFSDBG("high CLK_ENABLE = 0x%x\n", DRV_Reg32(CLK_ENABLE));
    // kal_taskEXIT_CRITICAL();
}

void switch_to_clk_high(void)
{
    DVFSFUC();

    /* switch SCP clock to Ring oscillator */
    clk_sw_select(CLK_SEL_HIGH);
    DRV_SetReg32(GENERAL_REG5, 1 << CLK_DONE);
}

void turn_off_clk_sys(void)
{
    DVFSFUC();

    if (!force_clk_sys_on || g_scp_sleep_flag) {
        /* Turn off sys Clock Source */
        DRV_ClrReg32(CLK_ENABLE, 1 << CLK_SYS_EN_BIT);
        DVFSDBG("sys CLK_ENABLE = 0x%x\n", DRV_Reg32(CLK_ENABLE));
    }
}

void switch_to_clk_sys(void)
{
    DVFSFUC();

    /* switch SCP clock to Ring oscillator */
    clk_sw_select(CLK_SEL_SYS);
    DRV_SetReg32(GENERAL_REG5, 1 << CLK_DONE);
}

void switch_to_clk_32k(void)
{
    DVFSFUC();

    /* switch SCP clock to Ring oscillator */
    clk_sw_select(CLK_SEL_32K);
    DRV_SetReg32(GENERAL_REG5, 1 << CLK_DONE);
}

uint32_t get_clk_high_ack(void)
{
    uint32_t clk_ack;
    clk_ack = DRV_Reg32(CLK_ACK);
    DVFSLOG("DRV_Reg32(CLK_ACK) = 0x%x\n", clk_ack);
    return ((clk_ack & (1 << CLK_HIGH_ACK)) ? 1 : 0);
}

uint32_t get_clk_high_safe_ack(void)
{
    return ((DRV_Reg32(CLK_SAFE_ACK) & (1 << CLK_HIGH_SAFE_ACK)) ? 1 : 0);
}

void turn_on_clk_high(void)
{
    int32_t ret = 0, is_already_on = 0;
    volatile uint32_t  count = 0;
    uint32_t irq_status, irq_ack;

    DVFSFUC();

    //kal_taskENTER_CRITICAL();

    //g_clk_high_req_cnt++;
    //DVFSLOG("in %s, get count %d\n", __func__, g_clk_high_req_cnt);
    //if (g_clk_high_req_cnt == 1) {
    if (get_clk_high_ack() == 1) { /* check if 26M is already ON */
        ret = 1;
        if (get_clk_high_safe_ack() == 1) {
            is_already_on = 1;
            switch_to_clk_high();
            turn_off_clk_sys();
        }
    }
    turn_on_high_flag = 1;
    //DRV_ClrReg32(CLK_ENABLE, 1 << CLK_SYS_IRQ_EN_BIT);
    DRV_SetReg32(CLK_ENABLE, 1 << CLK_HIGH_IRQ_EN_BIT);
    /* enable high clock */
    DRV_SetReg32(CLK_ENABLE, (1 << CLK_HIGH_EN_BIT));
    /* Turn on clk high output */
    DRV_SetReg32(CLK_ENABLE, (1 << CLK_HIGH_CG_BIT));
    //}

    DVFSDBG("in %s, leave CRITICAL is_already_on = %d\n", __func__, is_already_on);

    if (ret == 1) {
        do {
            if (get_clk_high_safe_ack() == 1)
                break;
            count++;
        } while (count < 100);
    }

    count = 0;
    do {
        irq_status = DRV_Reg32(IRQ_STATUS);
        irq_ack = DRV_Reg32(CLK_IRQ_ACK);
        if (((irq_status & (1 << CLK_CTRL_IRQn)) != 0) || ((irq_ack & CLK_HIGH_IRQ_ACK) != 0)) {
            DVFSLOG("IRQ_STATUS = 0x%x, CLK_IRQ_ACK = 0x%x\n", irq_status, irq_ack);
            clkctrl_polling(irq_ack);
            break;
        }
        count++;
    } while (count <= 100000);
}

uint32_t get_clk_sys_ack(void)
{
    //DVFSFUC();
    return ((DRV_Reg32(CLK_ACK) & (1 << CLK_SYS_ACK)) ? 1 : 0);
}

uint32_t get_clk_sys_safe_ack(void)
{
    //DVFSFUC();
    return ((DRV_Reg32(CLK_SAFE_ACK) & (1 << CLK_SYS_SAFE_ACK)) ? 1 : 0);
}

void turn_on_clk_sys(void)
{
    int32_t ret = 0, is_already_on = 0;
    volatile uint32_t count = 0;
    uint32_t irq_status, irq_ack;

    DVFSFUC();

    //kal_taskENTER_CRITICAL();

    //g_clk_sys_req_cnt++;
    //DVFSDBG("in %s, get count %d\n", __func__, g_clk_sys_req_cnt);

    // if (g_clk_sys_req_cnt == 1) {
    if (get_clk_sys_ack() == 1) { /* check if 26M is already ON */
        ret = 1;
        if (get_clk_sys_safe_ack() == 1) {
            is_already_on = 1;
            switch_to_clk_sys();
            turn_off_clk_high();
        }
    }
    /* enable sys clock */
    turn_on_sys_flag = 1;
    DRV_ClrReg32(CLK_ENABLE, 1 << CLK_HIGH_IRQ_EN_BIT);
    DRV_SetReg32(CLK_ENABLE, 1 << CLK_SYS_IRQ_EN_BIT);
    DRV_SetReg32(CLK_ENABLE, 1 << CLK_SYS_EN_BIT);
    //}
    DVFSDBG("in %s, leave CRITICAL is_already_on = %d\n", __func__, is_already_on);

    if (ret == 1) {
        do {
            if (get_clk_sys_safe_ack() == 1)
                break;
            count++;
        } while (count < 100);
    }

    count = 0;
    do {
        irq_status = DRV_Reg32(IRQ_STATUS);
        irq_ack = DRV_Reg32(CLK_IRQ_ACK);
        if (((irq_status & (1 << CLK_CTRL_IRQn)) != 0) || ((irq_ack & CLK_SYS_IRQ_ACK) != 0)) {
            DVFSLOG("IRQ_STATUS = 0x%x, CLK_IRQ_ACK = 0x%x\n", irq_status, irq_ack);
            clkctrl_polling(irq_ack);
            break;
        }
        count++;
    } while (count <= 100000);
}

/*****************************************
 * AP Resource APIs
 ****************************************/

int32_t get_resource_ack(uint32_t reg_name)
{
    DVFSFUC();
    return ((DRV_Reg32(reg_name) & 0x100) ? 1 : 0);
}

int32_t wait_for_ack(uint32_t reg_name)
{
    uint32_t ret, dma_count = 0;

    do {
        ret = get_resource_ack(reg_name);
        if (ret == 1)
            break;
        dma_count++;
    } while (dma_count < 1000000);
    if (ret == 0 && dma_count >= 1000000) {
        DVFSDBG("request dma time out (0x%x)\n", dma_user_list);
        configASSERT(0);
        return 0;
    } else if (ret == 1) {
        return 1;
    }

    return 0;
}

void enable_AP_resource(scp_reserve_mem_id_t dma_id)
{
    uint32_t ap_ack;

    DVFSFUC();
    if ((dma_user_list & (1 << dma_id)) == 0) {
        dma_user_list |= 1 << dma_id;
        ap_resource_req_cnt++;
    }
    if (ap_resource_req_cnt == 1) {
        DRV_SetReg32(AP_RESOURCE, 0x1);
        /* wait for 200us
         * ensure SPM already complete the previous AP resource disable request*/
        udelay(200);
        ap_ack = wait_for_ack(AP_RESOURCE);

        if (ap_ack == 1) {
            DVFSDBG("access dram resource success.\n");
        } else {
            ap_resource_req_cnt --;
            dma_user_list &= ~(1 << dma_id);
            DVFSDBG("access dram resource fail.(%d)\n", dma_id);
        }
    } else {
        DVFSDBG("adram already requested.(0x%x)\n", dma_user_list);
    }
}

void disable_AP_resource(scp_reserve_mem_id_t dma_id)
{
    DVFSFUC();
    if ((dma_user_list & (1 << dma_id)) != 0) {
        ap_resource_req_cnt--;
        dma_user_list &= ~(1 << dma_id);
    }
    if (ap_resource_req_cnt == 0) {
        DRV_ClrReg32(AP_RESOURCE, 0x1);
    } else {
        DVFSDBG("adram already released.(0x%x)\n", dma_user_list);
    }
}

void enable_infra(scp_reserve_mem_id_t dma_id)
{
    uint32_t bus_ack;
    DVFSFUC();
    if ((infra_user_list & (1 << dma_id)) == 0) {
        infra_user_list |= 1 << dma_id;
        infra_req_cnt++;
    }

    if (infra_req_cnt == 1) {
        DRV_SetReg32(CLK_ENABLE, (1 << CLK_SYS_EN_BIT));
        DRV_SetReg32(BUS_RESOURCE, 0x1);
        bus_ack = wait_for_ack(BUS_RESOURCE);

        if (bus_ack == 1) {
            DVFSDBG("open clk_bus success.\n");
        } else {
            infra_req_cnt --;
            infra_user_list &= ~(1 << dma_id);
            DVFSDBG("open clk_bus fail.(%d)\n", dma_id);
        }
    } else {
        DVFSDBG("clk_bus already requested.(0x%x)\n", infra_user_list);
    }
}

void disable_infra(scp_reserve_mem_id_t dma_id)
{
    DVFSFUC();
    if ((infra_user_list & (1 << dma_id)) != 0) {
        infra_req_cnt--;
        infra_user_list &= ~(1 << dma_id);
    }

    if (infra_req_cnt == 0) {
        DRV_ClrReg32(BUS_RESOURCE, 0x1);
        turn_off_clk_sys();
    } else {
        DVFSDBG("clk_bus already released.(0x%x)\n", dma_user_list);
    }
}

void enable_clk_bus(scp_reserve_mem_id_t dma_id)
{
    DVFSFUC();
    kal_taskENTER_CRITICAL();
    enable_infra(dma_id);
    kal_taskEXIT_CRITICAL();
}

void disable_clk_bus(scp_reserve_mem_id_t dma_id)
{
    DVFSFUC();
    kal_taskENTER_CRITICAL();
    disable_infra(dma_id);
    kal_taskEXIT_CRITICAL();
}

void enable_clk_bus_from_isr(scp_reserve_mem_id_t dma_id)
{
    DVFSFUC();
    enable_infra(dma_id);
}

void disable_clk_bus_from_isr(scp_reserve_mem_id_t dma_id)
{
    DVFSFUC();
    disable_infra(dma_id);
}

void dvfs_enable_DRAM_resource(scp_reserve_mem_id_t dma_id)
{
    DVFSFUC();
    kal_taskENTER_CRITICAL();
    enable_infra(dma_id);
    enable_AP_resource(dma_id);
    kal_taskEXIT_CRITICAL();
}

void dvfs_disable_DRAM_resource(scp_reserve_mem_id_t dma_id)
{
    DVFSFUC();
    kal_taskENTER_CRITICAL();
    disable_AP_resource(dma_id);
    disable_infra(dma_id);
    kal_taskEXIT_CRITICAL();
}

void dvfs_enable_DRAM_resource_from_isr(scp_reserve_mem_id_t dma_id)
{
    DVFSFUC();
    enable_infra(dma_id);
    enable_AP_resource(dma_id);
}

void dvfs_disable_DRAM_resource_from_isr(scp_reserve_mem_id_t dma_id)
{
    DVFSFUC();
    disable_AP_resource(dma_id);
    disable_infra(dma_id);
}

void scp_wakeup_src_setup(wakeup_src_id irq, uint32_t enable)
{
    kal_taskENTER_CRITICAL();

    DVFSLOG("%s(%d,%d)\n", "s_w_s_s", irq, enable);

    if (irq >= NR_WAKE_CLK_CTRL) {
        DVFSERR("fail to set wake up SRC\n");
        return ;
    }

    if (enable) {
        wakeup_src_mask |= (1 <<  irq);
    } else {
        wakeup_src_mask &= ~(1 <<  irq);
    }

    kal_taskEXIT_CRITICAL();
    return;
}
/*****************************************
 * Sleep Control
 ****************************************/

/*
 * Only used if SPM Sleep Mode is enabled.
 * Set to allow SCP to enter halt state, but keep clock on always
 */
void set_CM4_CLK_AO(void)
{
    DVFSFUC();
    DRV_SetReg32(SLEEP_CTRL, 1 << SPM_SLP_MODE_CLK_AO);
}

//Allow SPM clock request to be inactive when SCP is halted
void disable_CM4_CLK_AO(void)
{
    DVFSFUC();
    DRV_ClrReg32(SLEEP_CTRL, 1 << SPM_SLP_MODE_CLK_AO);
}

/*
 * Enable SPM controlled Sleep mode.
 * In this mode, SPM will handle SCP sleep mode, wakeup and SRAM control.
 */
void use_SPM_ctrl_sleep_mode(void)
{
    DVFSFUC();
    DRV_SetReg32(SLEEP_CTRL, 1 << SPM_SLP_MODE);
}

// Disable SPM controlled Sleep mode.
void use_SCP_ctrl_sleep_mode(void)
{
    DVFSFUC();
    DRV_ClrReg32(SLEEP_CTRL, 1 << SPM_SLP_MODE);
}

int32_t polling_request_state(void)
{
    int32_t status;
    DVFSFUC();

    status = DVFS_STATUS_OK;

    return status;
}
extern void mtk_wdt_disable(void);
extern void mtk_wdt_enable(void);

static int32_t request_spm_dvs(voltage_enum req_voltage)
{
#ifdef VCORE_DVFS_TEST_MODE
    return DVFS_STATUS_OK;
#else
    uint32_t status;
    uint32_t volt_status, irq_status = 0;
    uint32_t scp2spm_status = 0;
    uint32_t cur_volt = SPM_VOLTAGE_800_D;
    int32_t ret_val = DVFS_STATUS_OK;
    uint64_t spm_count = 0;

    DVFSFUC();
    if (dvfs_first_init)
        DRV_SetReg32(PCM_CON1, PCM_WRITE_ENABLE | (1 << SCP_APB_INTERNAL_EN));

#ifdef SPM_USE_INT_TO_NOTIFY_SCP
    DRV_SetReg32(IRQ_ENABLE, 1 << SPM_IRQ_EN);
#endif
    if (!spm_start)  {
        status = (DRV_Reg32(SPM_SW_FLAG) & 0x400) >> 10;
        DVFSLOG("SPM status= 0x%x\n", status);
        if (status == 1)
            spm_start = true;
        else
            return DVFS_STATUS_OK;
    }
    if (!spm_start)
        return DVFS_STATUS_OK;

    if (req_voltage == SPM_VOLTAGE_800) {
        DVFSLOG("skip voltage 0.8v\n");
        return DVFS_STATUS_OK;
    }

    do {
        /* there is two SPM tasks: help scp wake up AP, adjust scp request voltage
         * because there is a wakeup ipi sent from scp to AP
         * wait for AP to clear the mailbox first
         * */
        if ((DRV_Reg32(SCP_TO_HOST_INT) & 0x1) == 0x0
                || (DRV_Reg32(SPM_SW_RSV_3) & 0xBABE0000) != 0xBABE0000) {

            DVFSDBG("scp to host not processing\n");
            break;
        }
        spm_count++;
        if (spm_count > 1000000)
            return DVFS_STATUS_BUSY;
    } while (1);
    spm_count = 0;

    if ((DRV_Reg32(SPM_SW_RSV_3) & (0x1 << SPM_CLR_WAKE_UP_EVT)) == (0x1 << SPM_CLR_WAKE_UP_EVT)) {
        DVFSDBG("spm rsv3 0x%x\n", DRV_Reg32(SPM_SW_RSV_3));
        return DVFS_STATUS_OK;
    }
    cur_volt = DRV_Reg32(SPM_SW_RSV_3) & 0x3;
    DRV_WriteReg32(SPM_SW_RSV_3, (DRV_Reg32(SPM_SW_RSV_3) & ~(0x3)) | req_voltage << SPM_VOLT);

    if (req_voltage == cur_volt) {
        DVFSLOG("SPM request same voltage\n");
        return DVFS_STATUS_OK;
    }

    DRV_SetReg32(SCP_TO_SPM_INT, 1);

    do {
        spm_count++;
        volt_status = (DRV_Reg32(SPM_SW_RSV_3) >> SPM_SET_DONE) & 0x1;
        scp2spm_status = DRV_Reg32(SCP_TO_SPM_INT);

        if (spm_count % 30000 == 0) {
            PRINTF_W("irq_status = 0x%x, ack = 0x%x, SPM_SW_RSV_3 = 0x%x, r6 = 0x%x, r15 = 0x%x\n",
                     irq_status, DRV_Reg32(0x400ac098), DRV_Reg32(SPM_SW_RSV_3), DRV_Reg32(0x400ac118), DRV_Reg32(0x400ac13c));
        }
        if (volt_status == 1 && scp2spm_status == 0) {
            DRV_ClrReg32(SPM_SW_RSV_3, 1 << SPM_SET_DONE);
            DRV_ClrReg32(SPM_SW_RSV_3, 1 << SPM_CLR_WAKE_UP_EVT);
            DVFSDBG(" request spm status 0x%x\n", DRV_Reg32(SPM_SW_RSV_3));
            ret_val = DVFS_STATUS_OK;
            break;
        }

        spm_count++;
        if (spm_count > 50000) {
            DVFSDBG("pc =0x%x, pcm cnt1 = 0x%x, ack = 0x%x\n", DRV_Reg32(0x400ac13c),  DRV_Reg32(0x400ac400),
                    DRV_Reg32(0x400ac098));
            break;
        }
    } while (1);

    if (spm_count > 50000) {
        DVFSDBG(" request spm timeout, count = %d\n", spm_count);
        ret_val =  DVFS_STATUS_TIMEOUT;
        //configASSERT(0);
    }

    return ret_val;
#endif
}

int32_t request_dfs_opp_down(uint32_t req_opp)
{
    int32_t ret_val = DVFS_STATUS_OK, clk_status;
    DVFSFUC();

    if (get_cur_clk() == CLK_224M) {
        if (req_opp == CLK_112M) {
            if (polling_request_state() != DVFS_STATUS_OK) {
                DVFSERR("cound not get idle state\n");
                ret_val = DVFS_STATUS_BUSY;
                goto dvfs_err;
            }
            clk_div_select(CLK_DIV_2);
            request_spm_dvs(SPM_VOLTAGE_800);
        }
    } else if (get_cur_clk() == CLK_354M) {
        if (polling_request_state() != DVFS_STATUS_OK) {
            DVFSERR("cound not get idle state\n");
            ret_val = DVFS_STATUS_BUSY;
            goto dvfs_err;
        }
        turn_on_clk_high();
        clk_status = (DRV_Reg32(GENERAL_REG5) >> CLK_DONE) & 0x1;
        DRV_ClrReg32(GENERAL_REG5, 1 << CLK_DONE);
        if (clk_status) {
            if (req_opp == CLK_112M) {
                clk_div_select(CLK_DIV_2);
                request_spm_dvs(SPM_VOLTAGE_800);
            } else if (req_opp == CLK_224M) {
                clk_div_select(CLK_DIV_1);
                request_spm_dvs(SPM_VOLTAGE_900);
            }
        } else {
            DVFSDBG("clk setting on timeout");
            ret_val = DVFS_STATUS_TIMEOUT;
        }
    } else {
        DVFSDBG("division setting error %d\n", get_clk_div_select());
        ret_val = DVFS_STATUS_ERR;
    }
dvfs_err:
    return ret_val;
}

int32_t request_dfs_opp_up(uint32_t req_opp)
{
    int32_t ret_val = DVFS_STATUS_OK, clk_status;
    clk_enum cur_clk;
    DVFSFUC();
    if (req_opp == CLK_354M) {
        ret_val = request_spm_dvs(SPM_VOLTAGE_1000);
        if ((polling_request_state() != DVFS_STATUS_OK
                || ret_val != DVFS_STATUS_OK) && !dvfs_first_init) {
            DVFSERR("cound not get idle state\n");
            ret_val = DVFS_STATUS_BUSY;
            goto dvfs_err;
        }
        turn_on_clk_sys();
        clk_status = (DRV_Reg32(GENERAL_REG5) >> CLK_DONE) & 0x1;
        DRV_ClrReg32(GENERAL_REG5, 1 << CLK_DONE);
        if (clk_status)
            clk_div_select(CLK_DIV_1);
    } else if (req_opp == CLK_224M) {
        cur_clk = get_cur_clk();
        if (cur_clk <= CLK_112M) {
            ret_val = request_spm_dvs(SPM_VOLTAGE_900);
            if ((polling_request_state() != DVFS_STATUS_OK
                    || ret_val != DVFS_STATUS_OK) && !dvfs_first_init) {
                ret_val = DVFS_STATUS_BUSY;
                goto dvfs_err;
            }
            if (cur_clk == CLK_UNKNOWN) {
                turn_on_clk_high();
                clk_status = (DRV_Reg32(GENERAL_REG5) >> CLK_DONE) & 0x1;
                DRV_ClrReg32(GENERAL_REG5, 1 << CLK_DONE);
                if (clk_status)
                    clk_div_select(CLK_DIV_1);
            }
            clk_div_select(CLK_DIV_1);
        } else {
            DVFSDBG("clk setting wrong\n", get_cur_clk());
            ret_val = DVFS_STATUS_ERR;
        }
    } else if (req_opp == CLK_112M) {
        cur_clk = get_cur_clk();
        if (cur_clk < CLK_112M) {
            ret_val = request_spm_dvs(SPM_VOLTAGE_800);
            if ((polling_request_state() != DVFS_STATUS_OK
                    || ret_val != DVFS_STATUS_OK) && !dvfs_first_init) {
                DVFSERR("cound not get idle state\n");
                ret_val = DVFS_STATUS_BUSY;
                goto dvfs_err;
            }
            turn_on_clk_high();
            clk_status = (DRV_Reg32(GENERAL_REG5) >> CLK_DONE) & 0x1;
            DRV_ClrReg32(GENERAL_REG5, 1 << CLK_DONE);
            if (clk_status)
                clk_div_select(CLK_DIV_2);
        } else {
            DVFSDBG("clk setting wrong\n", get_cur_clk());
            ret_val = DVFS_STATUS_ERR;
        }
    } else {
        DVFSDBG("no need to drop CLK rate, %d\n", req_opp);
    }
dvfs_err:
    return ret_val;
}

/*
 * Use only when using SCP internal Sleep Controller.
 * Please disable when using SPM to control SCP Sleep Mode.
 */
void enable_SCP_sleep_mode(void)
{
    DVFSFUC();
    DRV_ClrReg32(CLK_ENABLE, (1 << CLK_SYS_EN_BIT));
    DRV_SetReg32(SLEEP_CTRL, 1 << SLP_CTRL_EN);
}

void disable_SCP_sleep_mode(void)
{
    DVFSFUC();
    DRV_ClrReg32(SLEEP_CTRL, 1 << SLP_CTRL_EN);
}

uint32_t request_dvfs_opp(uint32_t req_opp)
{
    int32_t ret_val = DVFS_STATUS_OK;
#if 1
    if ((DRV_Reg32(SLEEP_DEBUG) & ((1 << ENTERING_SLEEP) | (1 << IN_SLEEP))) == 0) {
        if (req_opp == CLK_354M)
            force_clk_sys_on = true;
        else
            force_clk_sys_on = false;
        g_dfs_cur_opp = get_cur_clk();

        if (g_fix_clk_sw_flag == true) {
            if (req_opp != g_fix_clk_sw_val) {
                req_opp = g_fix_clk_sw_val;
            }
        }

        if (g_limited_clk_sw_flag == true) {
            if (req_opp > g_limited_clk_sw_val) {
                req_opp = g_limited_clk_sw_val;
            }
        }
        DVFSLOG("req_opp = %d, g_dfs_cur_opp = %d\n", req_opp, g_dfs_cur_opp);
        if (g_dfs_cur_opp == req_opp) {
#if 1
            if (req_opp == CLK_112M)
                ret_val = request_spm_dvs(SPM_VOLTAGE_800);
            else if (req_opp == CLK_224M)
                ret_val = request_spm_dvs(SPM_VOLTAGE_900);
            else if (req_opp == CLK_354M)
                ret_val = request_spm_dvs(SPM_VOLTAGE_1000);
#endif
        } else {
            if (g_dfs_cur_opp > req_opp) {
                ret_val = request_dfs_opp_down(req_opp);
            } else if (g_dfs_cur_opp < req_opp) {
                ret_val = request_dfs_opp_up(req_opp);
            } else {
                DVFSERR("should not be reach this line! (%d, %d)\n", g_dfs_cur_opp, req_opp);
            }
        }
        // xSemaphoreGive(dvfs_mutex);
        return ret_val;
    } else {
        DVFSDBG("SCP is in sleep status! (0x%x\n", DRV_Reg32(SLEEP_DEBUG));
    }

#endif
    return ret_val;

}

int32_t unmask_all_int(void)
{
    DRV_WriteReg32(IRQ_ENABLE, g_irq_enable_state);
    DVFSLOG("g_irq_enable_state = 0x%x\n", g_irq_enable_state);
    DRV_ClrReg32(IRQ_SLEEP_EN, 0x3fffffff);
    return DVFS_STATUS_OK;
}

int32_t mask_all_int(void)
{
    g_irq_enable_state = DRV_Reg32(IRQ_ENABLE);
    DRV_ClrReg32(IRQ_ENABLE, 0x3fffffff);

    if (!g_disable_dvfs_flag) {
        /* enable wakeup source(clk & IPC for test)*/
        DRV_SetReg32(IRQ_SLEEP_EN,
                     wakeup_src_mask);//INTC 1 enable. 0 disable  0x0000200F IPC enable,  0x01002000  audio & FIFO enable,  0x001FA000 timer enable
    } else {
        DRV_SetReg32(IRQ_SLEEP_EN, 0x00000010);
    }
    DVFSLOG("IRQ_SLEEP_EN = 0x%x\n", DRV_Reg32(IRQ_SLEEP_EN));
    DRV_SetReg32(IRQ_ENABLE, (1 << CLK_CTRL_IRQ_EN));
    return DVFS_STATUS_OK;
}

void set_vreq_count(void)
{
    DVFSFUC();
    /* VREQ_COUNT */
    DRV_SetReg32(SLEEP_CTRL, 0x1 << VREQ_COUNT);//CLKCTL 0x0020 SLEEP_CTRL  VREQ_COUNT
}

void set_clk_sys_val(void)
{
    DVFSFUC();
    /* CLK_SYS_VAL */
    DRV_SetReg32(CLK_SYS_VAL, 0x1F);//CLKCTL 0x0014 CLK_SYS_VAL
}

void set_clk_high_val(void)
{
    DVFSFUC();
    /* CLK_HIGH_VAL */
    DRV_SetReg32(CLK_HIGH_VAL, 0x3);//CLKCTL 0x0014 CLK_HIGH_VAL
}

void set_cm4_scr(void)
{
    DVFSFUC();
    /* sleep conctrol */
    DRV_SetReg32(0xE000ED10, 0x4);
}

void clr_clock_request(void)
{
    g_clk_high_req_cnt = 0;
    g_clk_sys_req_cnt = 0;
}

#if 0
#define mainCHECK_DELAY                     ( ( portTickType ) 2000 / portTICK_RATE_MS )
static void vDvfsMainTask(void *pvParameters)
{
    portTickType xLastExecutionTime, xDelayTime;
    xLastExecutionTime = xTaskGetTickCount();
    xDelayTime = mainCHECK_DELAY;

    while (1) {
        PRINTF_W("dummy task  0x%x\n", DRV_Reg32(SLEEP_DEBUG));
        taskYIELD();
        vTaskDelayUntil(&xLastExecutionTime, xDelayTime);
    }
}
#endif

#if 1
void dvfs_timer_callback(void *arg)
{
    //DVFSLOG("timer wake up scp\n");
    //DRV_ClrReg32(0x400a301c, 0x1 << 5);
}
#endif

int32_t check_infra_state(void)
{
#ifdef SWITCH_PMICW_CLK_BY_SPM
    if ((DRV_Reg32(BUS_RESOURCE) & (1 << SCP_BUS_ACK)) == 0)
        return DVFS_STATUS_OK;
    else
        return DVFS_STATUS_BUSY;
#else /* SWITCH_PMICW_CLK_BY_SPM */
#if 1
    uint32_t ap_ack;
    uint32_t bus_ack;
    uint32_t count = 0;

    DVFSFUC();
    DRV_ClrReg32(AP_RESOURCE, 1 << SCP_BUS_REQ);
    DRV_ClrReg32(BUS_RESOURCE, 1 << SCP_BUS_REQ);

    do {
        ap_ack = DRV_Reg32(AP_RESOURCE) >> SCP_BUS_ACK;
        if (ap_ack == 0)
            break;
        count++;
    } while (count < 10000);

    do {
        bus_ack = DRV_Reg32(BUS_RESOURCE) >> SCP_BUS_ACK;
        if (bus_ack == 0)
            break;
        count++;
    } while (count < 10000);
    if (ap_ack == 1 && bus_ack == 1) {
#else
    if (((DRV_Reg32(BUS_RESOURCE) >> SCP_BUS_ACK) & 0x1) == 1) {
#endif
        return DVFS_STATUS_BUSY;
    }

    return DVFS_STATUS_OK;
#endif /* SWITCH_PMICW_CLK_BY_SPM */
}

#ifdef SWITCH_PMICW_CLK_BY_SPM
int check_ap_suspend_status(int debug)
{
    int infra = 1, spm_irq = 1, in_dpidle = 1, pmicw_clk;

    if (check_infra_state() != DVFS_STATUS_BUSY)
        infra = 0;

    if (((DRV_Reg32(IRQ_STATUS) >> SPM_IRQ_EN) & 0x1) == 0)
        spm_irq = 0;

    if ((DRV_Reg32(SPM_SW_FLAG) & (1 << 25)) == 0)
        in_dpidle = 0;

    pmicw_clk = (DRV_Reg32(0x400a4034) >> 8) & 1;

    if (debug) {
        DVFSDBG("infra=%d, spm_irq=%d, in_idle=%d, pmicw_clk=%s\n",
                infra, spm_irq, in_dpidle, pmicw_clk ? "ROSC" : "clk_bus");
    }

    if (infra == 0 && spm_irq == 0 && in_dpidle == 0) {
        return 1;
    } else {
        return 0;
    }
}

void pmicw_init(void)
{
    U32 ext_data;
    static int first_set = 1;

    if (first_set == 1) {
        first_set = 0;

        //set 0x02F0[0]=1
        if (pwrap_scp_read((uint32_t)0x02F0, &ext_data) == 0) {
            if (pwrap_scp_write((uint32_t)0x2F0, ext_data | 0x1) != 0) {
                DVFSDBG("pwrap_scp_write(0x02F0)[0]=1 fail!\n");
            }

            if (pwrap_scp_read((uint32_t)0x02F0, &ext_data) != 0) {
                DVFSDBG("after write: pwrap_scp_read(0x02F0) fail\n");
            } else {
                if ((ext_data & 0x1) == 0) {
                    DVFSDBG("after write: pwrap_scp_read(0x02F0)=0x%x\n", ext_data);
                } else
                    DVFSDBG("read after write success: pwrap_scp_read(0x02F0)=0x%x\n", ext_data);
            }
        } else {
            DVFSDBG("pwrap_scp_read(0x02F0) fail!\n");
        }
    }
}
#endif /* SWITCH_PMICW_CLK_BY_SPM */

#ifdef ENABLE_PMICW_TEST
void pmicw_test(void)
{
    U32 ext_data;
    int i, success_cnt = 0, api_fail_cnt = 0, read_data_fail_cnt = 0;

    //set 0x400a4030[6]=1 used to enable SCP access PMICW bus
    //(need to disable it once access PMICW is done)
    DRV_SetReg32(0x400a4030, (0x1 << 6));

    for (i = 0; i < MAX_PMICW_TEST_ROUND; i++) {
        if (pwrap_scp_read((uint32_t)PMIC_ID_ADDR, &ext_data) != 0) {
            api_fail_cnt++;
            DVFSDBG("pwrap_scp_read() return fail!, PMIC_WRAP_WACS_P2P_RDATA=0x%x\n", DRV_Reg32(PMIC_WRAP_WACS_P2P_RDATA));
        }

        if (ext_data != PMIC_ID_VAL) {
            read_data_fail_cnt++;
            DVFSDBG("wrong PMICW read data 0x%x\n", ext_data);
        } else {
            success_cnt++;
        }
    }

    if (api_fail_cnt == 0 && read_data_fail_cnt == 0)
        DVFSDBG("PMICW read success cnt = %d\n", success_cnt);
    else {
        DVFSDBG("ERROR: PMICW read fail api_fail_cnt = %d, read_data_fail_cnt = %d\n", api_fail_cnt, read_data_fail_cnt);
        while (1);
    }

    //set 0x400a4030[6]=1 used to enable SCP access PMICW bus
    //(need to disable it once access PMICW is done)
    DRV_ClrReg32(0x400a4030, (0x1 << 6));
}
#endif /* ENABLE_PMICW_TEST */

int32_t request_enter_sleep_mode(void)
{
    DVFSFUC();

    //NVIC_ClearPendingIRQ(13);

#ifdef SWITCH_PMICW_CLK_BY_SPM
    if ((check_ap_suspend_status(0) == 1 || g_scp_sleep_flag) && !force_clk_sys_on) {
#else
    if ((check_infra_state() != DVFS_STATUS_BUSY || g_scp_sleep_flag) && !force_clk_sys_on) {
#endif
        return DVFS_STATUS_OK;
    }

    DVFSDBG("AP not in suspend or force_clk_sys_on, SCP cannot sleep\n");
    return DVFS_STATUS_BUSY;
}

void disable_scp_dvfs(int id, void* data, unsigned int len)
{
    uint8_t *msg;

    msg = (uint8_t *)data;
    DVFSLOG("in %s, IPIMSG Handler:%x\n", __func__, *msg);
#if 1
    if (*msg) {
        g_disable_dvfs_flag = true;
        DVFSLOG("scp disable flag on\n");
    } else {
        g_disable_dvfs_flag = false;
        DVFSLOG("scp disable flag off\n");
    }
#endif
}

void scp_sleep_enable(int id, void* data, unsigned int len)
{
    uint8_t *msg;

    msg = (uint8_t *)data;
    DVFSLOG("in %s, IPIMSG Handler:%x\n", __func__, *msg);

    g_scp_sleep_flag = true;
    DVFSLOG("scp sleep flag on\n");

}

void scp_sleep_disable(int id, void* data, unsigned int len)
{
    uint8_t *msg;

    msg = (uint8_t *)data;
    DVFSLOG("in %s, IPIMSG Handler:%x\n", __func__, *msg);

    g_scp_sleep_flag = false;
    DVFSLOG("scp sleep flag off\n");
}

void pre_sleep_process(uint32_t modify_time)
{
    if (modify_time < 2)
        return;
#if (configLOWPOWER_DISABLE == 1)
    mtk_wdt_disable();
    return ;
#endif

#ifdef DVFS_TIMER_PROFILING
    time_start_2[time_cnt] = *DWT_CYCCNT;
#endif
    DVFSFUC();
    mtk_wdt_disable();
    DRV_SetReg32(IRQ_ENABLE, 1 << SPM_IRQ_EN);
    if (!force_clk_sys_on || g_scp_sleep_flag) {
        request_dvfs_opp(CLK_112M);
    }

    if (request_enter_sleep_mode() == DVFS_STATUS_OK) {

        if (!g_disable_dvfs_flag) {
            DRV_SetReg32(CLK_CG_CTRL, 0x7);
            g_sleep_flag = 1;
            platform_set_periodic_timer(dvfs_timer_callback, NULL, modify_time);
        }

#ifdef ENABLE_PMICW_TEST
        DVFSDBG("\nprepare enter sleep\n");
        pmicw_test();
#endif

        if (((DRV_Reg32(IRQ_STATUS) >> SPM_IRQ_EN) & 0x1) != 0) {
            DVFSDBG("not sleep cause AP resume!\n");
            goto enter_idle;
        } else {
#ifdef SWITCH_PMICW_CLK_BY_SPM
            DRV_SetReg32(0x400A4034, (0x1 << PMICW_SLEEP_REQ_BIT));
            while ((DRV_Reg32(0x400A4034) & (0x1 << PMICW_SLEEP_ACK_BIT)) == 0) {
                if (((DRV_Reg32(IRQ_STATUS) >> SPM_IRQ_EN) & 0x1) != 0) {
                    //DRV_ClrReg32(0x400A4034, (0x1<<PMICW_SLEEP_REQ_BIT));
                    DVFSDBG("not sleep cause AP resume!!\n");
                    goto enter_idle;
                }
            }
            //DVFSDBG("pmicw enter idle ack\n");
#endif
        }

        if (((DRV_Reg32(IRQ_STATUS) >> SPM_IRQ_EN) & 0x1) != 0) {
#ifdef SWITCH_PMICW_CLK_BY_SPM
            DVFSDBG("dobule re-check AP not in suspend\n");

            //set 0x100A4034[0]=0 (request pmic_wrap to exit idle)
            DRV_ClrReg32(0x400A4034, (0x1 << PMICW_SLEEP_REQ_BIT));
            DVFSDBG("pmicw exit idle req\n");

            //Spm wait until 0x100A4034[4]=0x1 (pmic_wrap retrun idle ack => pmic_wrap is idle)
            while ((DRV_Reg32(0x400A4034) & (0x1 << PMICW_SLEEP_ACK_BIT)) != 0);
            DVFSDBG("pmicw exit idle ack\n");
            goto enter_idle;
#endif
        } else {
            set_cm4_scr();
            clr_clock_request();
            DRV_ClrReg32(CM4_CONTROL, 1 << AP_DBG_REQ);
            mask_all_int();
            enable_SCP_sleep_mode();
            enter_sleep_mode = 1;
            g_sleep_cnt++;
            DVFSDBG("SCP sleep ...\n\n");
        }
    } else {
#ifdef ENABLE_PMICW_TEST
        //pmicw_test();
#endif
        goto enter_idle;
    }
#ifdef DVFS_TIMER_PROFILING
    time_end_2[time_cnt] = *DWT_CYCCNT;
    time_cnt++;
    if (time_cnt % 30 == 0)
        time_cnt = 0;
#endif
    return;
enter_idle:
    /* turn on timer CG to perform wake up flow */
    g_sleep_flag = 0;

    DRV_SetReg32(CLK_CG_CTRL, 0x7);
    platform_set_periodic_timer(dvfs_timer_callback, NULL, modify_time);
#ifdef DVFS_TIMER_PROFILING
    time_end_2[time_cnt] = *DWT_CYCCNT;
    // if (g_sleep_cnt != pre_sleep_cnt) {
    uint32_t i;
    pre_sleep_cnt = g_sleep_cnt;
    for (i = 0; i < 30; i++) {
        PRINTF_W("[%d]post time_start = %d, time_end %d, duration = %d\n", i, time_start_1[i], time_end_1[i],
                 time_end_1[i] - time_start_1[i]);
        PRINTF_W("[%d]task time_start = %d, time_end %d, duration = %d\n", i, time_end_1[i], time_start_2[i],
                 time_start_2[i] - time_end_1[i]);
        PRINTF_W("[%d]idle time_start = %d, time_end %d, duration = %d\n", i, time_start_2[i], time_end_2[i],
                 time_end_2[i] - time_start_2[i]);
        PRINTF_W("sleep cnt = %d, time_cnt = %d\n", g_sleep_cnt, time_cnt);
    }
    //  }
#endif
    return;
}

void post_sleep_process(uint32_t expect_time)
{
    g_wakeup_src = DRV_Reg32(IRQ_STATUS);
    wakeup_source_count(g_wakeup_src);

    if (expect_time < 2)
        return;

#if (configLOWPOWER_DISABLE == 1)
    //mtk_wdt_enable();
    return;
#endif
    uint32_t irq_status, irq_ack;
    uint32_t clk_opp;

#ifdef DVFS_TIMER_PROFILING
    CPU_RESET_CYCLECOUNTER();
    time_start_1[time_cnt] = *DWT_CYCCNT;
#endif
    //mtk_wdt_enable();


    if (enter_sleep_mode == 1) {
#ifdef SWITCH_PMICW_CLK_BY_SPM
        DVFSDBG("\nSCP wakeup (irq_status=0x%x)\n", DRV_Reg32(IRQ_STATUS));

        //set 0x100A4034[0]=0 (request pmic_wrap to exit idle)
        DRV_ClrReg32(0x400A4034, (0x1 << PMICW_SLEEP_REQ_BIT));
        DVFSDBG("pmicw exit idle req\n");

        //Spm wait until 0x100A4034[4]=0x1 (pmic_wrap retrun idle ack => pmic_wrap is idle)
        while ((DRV_Reg32(0x400A4034) & (0x1 << PMICW_SLEEP_ACK_BIT)) != 0);
        DVFSDBG("pmicw exit idle ack\n");
#endif

#ifdef ENABLE_PMICW_TEST
        pmicw_test();
        PRINTF_W("\n");
#endif

        irq_status = DRV_Reg32(IRQ_STATUS);
        if ((irq_status & (1 << CLK_CTRL_IRQn)) != 0) {
            irq_ack = DRV_Reg32(CLK_IRQ_ACK);
            clkctrl_polling(irq_ack);
        }
        disable_SCP_sleep_mode();
        unmask_all_int();
        enter_sleep_mode = 0;
    } else {
        DRV_ClrReg32(IRQ_ENABLE, 1 << SPM_IRQ_EN);
    }

    clk_opp = get_freq_setting();

    if (clk_opp == 112) {
        request_dvfs_opp(CLK_112M);
    } else if (clk_opp == 224) {
        request_dvfs_opp(CLK_224M);
    } else if (clk_opp == 354) {
        request_dvfs_opp(CLK_354M);
    }
    DRV_WriteReg32(CURRENT_FREQ_REG, clk_opp);
#ifdef DVFS_TIMER_PROFILING
    time_end_1[time_cnt] = *DWT_CYCCNT;
#endif
}

void clkctrl_polling(enum clk_irq_ack irq_ack)
{
    DVFSFUC();
    isr_leave_flag = false;
    if ((irq_ack & CLK_SYS_IRQ_ACK) != 0 && turn_on_sys_flag == 1) {
        if (get_clk_sys_ack() == 1
                && get_clk_sys_safe_ack() == 1) {
            turn_on_sys_flag = 0;
            switch_to_clk_sys();
            turn_off_clk_high();
        } else {
            DVFSERR("clk setting timeout\n");
        }
    }
    if ((irq_ack & CLK_HIGH_IRQ_ACK) != 0 && turn_on_high_flag == 1) {
        if (get_clk_high_ack() == 1
                && get_clk_high_safe_ack() == 1) {
            turn_on_high_flag = 0;
            switch_to_clk_high();
            turn_off_clk_sys();
        } else {
            DVFSERR("clk setting timeout\n");
        }
        //DRV_ClrReg32(CLK_ENABLE, CLK_SYS_IRQ_EN_BIT);
        DRV_ClrReg32(CLK_ENABLE, CLK_HIGH_IRQ_EN_BIT);
    }
    if ((irq_ack & (SLP_IRQ_ACK | CLK_SYS_IRQ_ACK | CLK_HIGH_IRQ_ACK)) == 0) {
        DVFSERR("clock ctrol irq type not found! (0x%x)\n", irq_ack);
    }
    DRV_Reg32(CLK_IRQ_ACK);
    NVIC_ClearPendingIRQ(CLK_CTRL_IRQn);
    isr_leave_flag = true;
}

void clkctrl_isr(void)
{
    uint32_t clk_irq_type;
    //BaseType_t xYieldRequired;

    DVFSFUC();
    isr_leave_flag = false;
    /* Read SLP_IRQ_ACK register to acknowledge CLK_CTRL interrupt */
    clk_irq_type = DRV_Reg32(CLK_IRQ_ACK);
    if ((clk_irq_type & CLK_SYS_IRQ_ACK) != 0 && turn_on_sys_flag == 1) {
        if (get_clk_sys_ack() == 1
                && get_clk_sys_safe_ack() == 1) {
            turn_on_sys_flag = 0;
            switch_to_clk_sys();
            turn_off_clk_high();
        } else {
            DVFSDBG("clk setting timeout\n");
        }
    }
    if ((clk_irq_type & CLK_HIGH_IRQ_ACK) != 0 && turn_on_high_flag == 1) {
        if (get_clk_high_ack() == 1
                && get_clk_high_safe_ack() == 1) {
            turn_on_high_flag = 0;
            switch_to_clk_high();
            turn_off_clk_sys();
        } else {
            DVFSDBG("clk setting timeout\n");
        }
        DRV_ClrReg32(CLK_ENABLE, CLK_HIGH_IRQ_EN_BIT);
    }
    if ((clk_irq_type & (SLP_IRQ_ACK | CLK_SYS_IRQ_ACK | CLK_HIGH_IRQ_ACK)) == 0) {
        DVFSDBG("clock ctrol irq type not found! (0x%x)\n", clk_irq_type);
    }
    DRV_Reg32(CLK_IRQ_ACK);
    NVIC_ClearPendingIRQ(CLK_CTRL_IRQn);
    isr_leave_flag = true;

}

void spm_isr(void)
{
    DRV_ClrReg32(IRQ_ENABLE, 1 << SPM_IRQ_EN);
    NVIC_ClearPendingIRQ(SPM_IRQn);
}
/* Init clock control setting */
void dvfs_init(void)
{
    DVFSFUC();

    dvfs_mutex = xSemaphoreCreateMutex();
    dvfs_irq_semaphore = xSemaphoreCreateMutex();
    //need to correct these settle time after chip back
    set_vreq_count();
    set_clk_sys_val(); //31.25us * 160 = 5000us = 5ms
    set_clk_high_val();  //31.25us * 3 = 90us

    request_irq(CLK_CTRL_IRQn, clkctrl_isr, "clkctrl_isr");
    request_irq(SPM_IRQn, spm_isr, "spm_isr");
#if CLK_CTRL_MODE == MODE_1_CLK_HIGH_ONLY || CLK_CTRL_MODE == MODE_2_CLK_HIGH_AND_SYS
    use_SCP_ctrl_sleep_mode();
    disable_SCP_sleep_mode();
#elif CLK_CTRL_MODE == MODE_3_SPM_CONTROL_MODE
    use_SPM_ctrl_sleep_mode();
#endif

    dvfs_first_init = true;
#if defined(CFG_MTK_AURISYS_PHONE_CALL_SUPPORT) || defined(CFG_MTK_AUDIO_TUNNELING_SUPPORT)
    request_dvfs_opp(CLK_354M);
#else
    request_dvfs_opp(CLK_112M);
#endif
    dvfs_first_init = false;

#if 1
    scp_ipi_registration(IPI_DVFS_DEBUG, (ipi_handler_t)dvfs_debug_set, "IPI_DVFS_DEBUG");
    scp_ipi_registration(IPI_DVFS_FIX_OPP_SET, (ipi_handler_t)set_fix_clk_sw_select, "IPI_DVFS_FIX_OPP_SET");
    scp_ipi_registration(IPI_DVFS_FIX_OPP_EN, (ipi_handler_t)set_fix_clk_enable, "IPI_DVFS_FIX_OPP_EN");
    scp_ipi_registration(IPI_DVFS_LIMIT_OPP_SET, (ipi_handler_t)set_limited_clk_sw_select, "IPI_DVFS_LIMIT_OPP_SET");
    scp_ipi_registration(IPI_DVFS_LIMIT_OPP_EN, (ipi_handler_t)set_limited_clk_enable, "IPI_DVFS_LIMIT_OPP_EN");
    scp_ipi_registration(IPI_DVFS_DISABLE, (ipi_handler_t)disable_scp_dvfs, "IPI_DVFS_DISABLE");
    scp_ipi_registration(IPI_DVFS_SLEEP, (ipi_handler_t)scp_sleep_enable, "IPI_DVFS_SLEEP");
    scp_ipi_registration(IPI_DVFS_WAKE, (ipi_handler_t)scp_sleep_disable, "IPI_DVFS_WAKE");

#endif

    //kal_xTaskCreate( vDvfsMainTask, "dvfs", 384, NULL, 1, NULL);

#ifdef SWITCH_PMICW_CLK_BY_SPM
    //set div of PMICW clock (0x10001098[8]= 1'b0:ulposc/8, 1'b1:ulposc/16)
    DRV_SetReg32(0xa0001098, (0x1 << 8));
    DVFSDBG("[0xa0001098] = 0x%x\n", DRV_Reg32(0xa0001098));

    //gating AP site's ROSC clock
    DRV_WriteReg32(0xa0006458, 0);
    DVFSDBG("[0xa0006458] = 0x%x\n", DRV_Reg32(0xa0006458));

    pmicw_init();
#endif

    DVFSDBG("in %s , init done\n", __func__);
}

uint32_t GetSystemCoreClock(void)
{
    clk_enum cur_clk = CLK_UNKNOWN;

    cur_clk = get_cur_clk();
    switch (cur_clk) {
        case CLK_112M:  /* HSI used as system clock source */
            SystemCoreClock = HSULP2_VALUE;
            break;
        case CLK_224M:  /* HSE used as system clock source */
            SystemCoreClock = HSULP_VALUE;
            break;
        case CLK_354M:  /* PLL used as system clock source */
            SystemCoreClock = HSPLL_VALUE;
            break;
        default:
            DVFSDBG("clock is now currently unclear yet (%d)\n", cur_clk);
            break;
    }

    return SystemCoreClock;
}


