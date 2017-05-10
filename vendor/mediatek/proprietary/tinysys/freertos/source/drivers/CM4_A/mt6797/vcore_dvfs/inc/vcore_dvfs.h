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

#ifndef _VCOREDVFS_
#define _VCOREDVFS_

#include <mt_reg_base.h>
#include <dma.h>


/***********************************************************************************
** Definitions
************************************************************************************/

//#define DVFS_DVT_TEST

#ifdef DVFS_DVT_TEST
//#define SCP_WAKEUP_BY_TIMER_TEST
//#define SCP_WAKEUP_SPM_TST
//#define COUNT_SLEEP_TIMES
//#define COREMARK_TEST
#endif

#define SPM_BASE        0x400ac000
#ifdef BUILD_SLT
#define SCP_WAKEUP_BY_TIMER_TEST
#endif

#define CLEAR_ALL_IRQ_EXCEPT_ONE    0

/* Clock Control Modes */
#define MODE_1_CLK_HIGH_ONLY        1
#define MODE_2_CLK_HIGH_AND_SYS     2
#define MODE_3_SPM_CONTROL_MODE     3

// #define DVFS_TIMER_PROFILING

#ifdef BUILD_CTP
#define CLK_CTRL_MODE MODE_1_CLK_HIGH_ONLY
#else

//#define CLK_CTRL_MODE MODE_2_CLK_HIGH_AND_SYS
#if (configLOWPOWER_DISABLE == 1)
#define CLK_CTRL_MODE MODE_3_SPM_CONTROL_MODE
#else
#define CLK_CTRL_MODE MODE_1_CLK_HIGH_ONLY
#endif
#endif

/* CLK_CTRL Registers */
#define CLK_SW_SEL          (CLK_CTRL_BASE + 0x00)  /* Clock Source Select */
#define CLK_ENABLE          (CLK_CTRL_BASE + 0x04)  /* Clock Source Enable */
#define CLK_SAFE_ACK        (CLK_CTRL_BASE + 0x08)  /* Clock Safe Ack */
#define CLK_ACK             (CLK_CTRL_BASE + 0x0C)  /* Clock Ack */
#define CLK_IRQ_ACK         (CLK_CTRL_BASE + 0x10)  /* Clock Interrupt Acknowledge */
#define CLK_SYS_VAL         (CLK_CTRL_BASE + 0x14)  /* System Clock Counter Value */
#define CLK_HIGH_VAL        (CLK_CTRL_BASE + 0x18)  /* ULPOSC Clock Counter Value */
#define SLEEP_CTRL          (CLK_CTRL_BASE + 0x20)  /* Sleep mode control */
#define CLK_DIV_SEL         (CLK_CTRL_BASE + 0x24)  /* Clock Divider Select */
#define SLEEP_DEBUG         (CLK_CTRL_BASE + 0x28)  /* Sleep mode debug signals */
#define SRAM_PDN            (CLK_CTRL_BASE + 0x2C)  /* Sram Power Down */
#define PMICW_CTRL          (CLK_CTRL_BASE + 0x34)  /* PMIC Wrapper Control */
#define SLEEP_WAKE_DEBUG    (CLK_CTRL_BASE + 0x38)  /* PMIC Wrapper Control */
#define DCM_EN              (CLK_CTRL_BASE + 0x3C)  /* PMIC Wrapper Control */

/* INTC Registers */
#define IRQ_STATUS          (INTC_BASE + 0x00)  /* SCP Interrupt Status */
#define IRQ_ENABLE          (INTC_BASE + 0x04)  /* SCP IRQ Enable */
#define IRQ_OUT             (INTC_BASE + 0x08)  /* Final Interrupt output (IRQ_STATUS & IRQ_EN) */
#define IRQ_SLEEP_EN        (INTC_BASE + 0x0C)  /* IRQ WAKE UP  Enable */

/* CFG REG control reg base */
#define BUS_RESOURCE        (CFGREG_BASE + 0x08)  /* BUS resource request */
#define SCP_TO_SPM_INT      (CFGREG_BASE + 0x20)  /* sshub to spm interrupt */

#define R_GPR3              (CFGREG_BASE + 0x5C)  /* General purpose register 4, used for reserve current opp */
#define R_GPR4              (CFGREG_BASE + 0x60)  /* General purpose register 4, used for reserve current opp */
#define CM4_CONTROL           (CFGREG_BASE + 0xa0)
#define SCP_BUS_ACK              8
#define SCP_BUS_REQ              0
#define AP_DBG_REQ               3

/* SPM reg base */
#define SPM_SW_RSV_3                                    (SPM_BASE + 0x614)
#define PCM_CON1                                             (0xA000601C)
#define SPM_WAKEUP_EVENT_MASK          (SPM_BASE + 0x0C4)
#define SPM_SRC_RDY_STA                             (SPM_BASE + 0x1d0)
#define SPM_SW_FLAG                                      (SPM_BASE + 0x118)
#define PCM_WRITE_ENABLE                           (0xb16 << 16)
#define SCP_APB_INTERNAL_EN                     14
#define SCP_WAKE_UP_MASK_EN                  10
#define CKSW_SEL_O          16
#define CKSW_DIV_SEL_O      16
#define CLK_CTRL_IRQ_EN     13
#define SPM_IRQ_EN          4
/* Clock Gating ID */
typedef enum {
    TMR_MCLK_CG = 0,
    TMR_BCLK_CG,
    MAD_MCLK_CG,
    I2C_MCLK_CG,
    I2C_BCLK_CG,
    GPIO_MCLK_CG,
    AP2P_MCLK_CG,
    UART0_MCLK_CG,
    UART0_BCLK_CG,
    UART1_MCLK_CG = 11,
    UART1_BCLK_CG,

    NR_CG
} clk_cg_id;

/* Wakeup Source ID */
typedef enum  {
    IPC0_WAKE_CLK_CTRL = 0,
    IPC1_WAKE_CLK_CTRL,
    IPC2_WAKE_CLK_CTRL,
    IPC3_WAKE_CLK_CTRL,
    SPM_WAKE_CLK_CTRL,
    CIRQ_WAKE_CLK_CTRL,
    EINT_WAKE_CLK_CTRL,
    PMIC_WAKE_CLK_CTRL,
    UART_WAKE_CLK_CTRL,
    UART1_WAKE_CLK_CTRL,
    I2C0_WAKE_CLK_CTRL,
    I2C1_WAKE_CLK_CTRL,
    I2C2_WAKE_CLK_CTRL,
    CLK_CTRL_WAKE_CLK_CTRL,
    MAD_FIFO_WAKE_CLK_CTRL,
    TIMER0_WAKE_CLK_CTRL,
    TIMER1_WAKE_CLK_CTRL,
    TIMER2_WAKE_CLK_CTRL,
    TIMER3_WAKE_CLK_CTRL,
    TIMER4_WAKE_CLK_CTRL,
    TIMER5_WAKE_CLK_CTRL,
    UART_RX_WAKE_CLK_CTRL,
    UART1_RX_WAKE_CLK_CTRL,
    DMA_WAKE_CLK_CTRL,
    AUDIO_WAKE_CLK_CTRL,
    MD_WAKE_CLK_CTRL,
    C2K_WAKE_CLK_CTRL,
    SPI0_WAKE_CLK_CTRL,
    SPI4_WAKE_CLK_CTRL,
    SPI5_WAKE_CLK_CTRL,

    NR_WAKE_CLK_CTRL
} wakeup_src_id;

enum {
    CLK_SYS_EN_BIT = 0,
    CLK_HIGH_EN_BIT = 1,
    CLK_HIGH_CG_BIT = 2,
    CLK_SYS_IRQ_EN_BIT = 16,
    CLK_HIGH_IRQ_EN_BIT = 17,
};

typedef enum {
    CLK_SEL_SYS     = 0x0,
    CLK_SEL_32K     = 0x1,
    CLK_SEL_HIGH    = 0x2
} clk_sel_val;

enum clk_ack {
    CLK_SYS_ACK = 0,
    CLK_HIGH_ACK = 1,
};

enum clk_safe_ack {
    CLK_SYS_SAFE_ACK = 0,
    CLK_HIGH_SAFE_ACK = 1,
};


enum clk_irq_ack {
    CLK_SYS_IRQ_ACK = 1,
    CLK_HIGH_IRQ_ACK = 2,
    SLP_IRQ_ACK = 8
};

enum PMICW_CTRL_BITS {
    PMICW_SLEEP_REQ = 0,
    PMICW_SLEEP_ACK = 4,
    PMICW_CLK_MUX = 8,
    PMICW_DCM = 9
};

enum SLEEP_CTRL_BITS {
    SLP_CTRL_EN = 0,
    VREQ_COUNT = 4,
    SPM_SLP_MODE = 8,
    SPM_SLP_MODE_CLK_AO = 9,
};

typedef enum {
    CLK_DIV_1 = 0,
    CLK_DIV_2 = 1,
    CLK_DIV_4  = 2,
    CLK_DIV_8  = 3,
    CLK_DIV_UNKNOWN,
} clk_div_enum;

enum {
    IN_DEBUG_IDLE = 0,
    ENTERING_SLEEP,
    IN_SLEEP,
    ENTERING_ACTIVE,
    IN_ACTIVE,
    DEBUG_REQ,
    DEBUG_ACK,
};

typedef enum  {
    CLK_UNKNOWN = 0,
    CLK_112M,
    CLK_224M,
    CLK_354M,
} clk_enum;

enum clk_dir_enum {
    CLK_DIR_UNKNOWN = 0,
    CLK_DIR_UP,
    CLK_DIR_DOWN,
};

typedef enum  {
    SPM_VOLTAGE_800_D = 0,
    SPM_VOLTAGE_800,
    SPM_VOLTAGE_900,
    SPM_VOLTAGE_1000,
    SPM_VOLTAGE_TYPE_NUM,
} voltage_enum;

enum {
    SPM_VOLT = 0,
    SPM_CLR_WAKE_UP_EVT = 2,
    SPM_SET_DONE = 3,
};

enum {
    CLK_DONE = 0,
    CLK_REQ = 1,
    VOLT_REQ = 3,
};

#ifdef DVFS_TIMER_PROFILING
/*
 * global variables
 */
extern volatile uint32_t *ITM_CONTROL;
extern volatile uint32_t *DWT_CONTROL;
extern volatile uint32_t *DWT_CYCCNT;
extern volatile uint32_t *DEMCR;

#define CPU_RESET_CYCLECOUNTER() \
do { \
*DEMCR = *DEMCR | 0x01000000; \
*DWT_CYCCNT = 0; \
*DWT_CONTROL = *DWT_CONTROL | 1 ; \
} while(0)
#endif
/***********************************************************************************
** Externs
************************************************************************************/
extern clk_enum get_cur_clk(void);

/*****************************************
 * CLK_HIGH and CLK_SYS Control APIs
 ****************************************/
extern void clk_sw_select(clk_sel_val val);
extern void clk_div_select(uint32_t val);

extern uint32_t get_clk_sw_select(void);
extern uint32_t get_clk_div_select(void);

/*****************************************
 * Sleep Control
 ****************************************/
extern void dvfs_enable_DRAM_resource(scp_reserve_mem_id_t dma_id);
extern void dvfs_disable_DRAM_resource(scp_reserve_mem_id_t dma_id);
extern void dvfs_enable_DRAM_resource_from_isr(scp_reserve_mem_id_t dma_id);
extern void dvfs_disable_DRAM_resource_from_isr(scp_reserve_mem_id_t dma_id);
extern void scp_wakeup_src_setup(wakeup_src_id irq, uint32_t enable);
extern int32_t check_infra_state(void);
extern void enable_clk_bus(scp_reserve_mem_id_t dma_id);
extern void disable_clk_bus(scp_reserve_mem_id_t dma_id);
extern void enable_clk_bus_from_isr(scp_reserve_mem_id_t dma_id);
extern void disable_clk_bus_from_isr(scp_reserve_mem_id_t dma_id);

extern void use_SPM_ctrl_sleep_mode(void);
extern void use_SCP_ctrl_sleep_mode(void);
extern int32_t request_dfs_opp_up(uint32_t req_opp);
extern int32_t request_dfs_opp_down(uint32_t req_opp);
extern void enable_SCP_sleep_mode(void);
extern void disable_SCP_sleep_mode(void);
extern int32_t request_enter_sleep_mode(void);
extern uint32_t GetSystemCoreClock(void);
extern void dvfs_init(void);


#endif //_CLKMGR_
