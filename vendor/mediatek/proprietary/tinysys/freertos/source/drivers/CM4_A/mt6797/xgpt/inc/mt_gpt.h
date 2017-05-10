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

#ifndef __MT_GPT_H__
#define __MT_GPT_H__

//#define APMCU_GPTIMER_BASE 0xA0008000
/* CPUxGPT */
#define APMCU_GPTIMER_BASE 0xA0220000
#define INDEX_BASE      (APMCU_GPTIMER_BASE+0x0674)
#define CTL_BASE        (APMCU_GPTIMER_BASE+0x0670)
#define TIMER_IN_CLK    0x400A4030
#define L_CNT64         0x084
#define H_CNT64         0x088

#define SCP_IMER_BASE TIMER_BASE

#define TMR0            0x0
#define TMR1            0x1
#define TMR2            0x2
#define TMR3            0x3
#define TMR4            0x4
#define TMR5            0x5
#define NR_TMRS         0x6
/* AP side system counter frequence is 13MHz*/
#define AP_NS_PER_CNT   (1000000000UL)/(13000000UL)

#define TIMER_EN             (0x00)
#define TIMER_RST_VAL        (0x04)
#define TIMER_CUR_VAL_REG    (0x08)
#define TIMER_IRQ_CTRL_REG   (0x0C)

#define TIMER_CLK_SEL_REG           (SCP_IMER_BASE+0x40)

#define portNVIC_MTK_XGPT_REG               ( * ( ( volatile uint32_t * ) 0x400A3018 ) )


//
//#define TIMER0_EN_REG                 ((volatile unsigned int*)(SCP_IMER_BASE))
//#define TIMER0_RST_VAL_REG        ((volatile unsigned int*)(SCP_IMER_BASE+0x04))
//#define TIMER0_CUR_VAL_REG        ((volatile unsigned int*)(SCP_IMER_BASE+0x08))
//#define TIMER0_IRQ_CTRL_REG       ((volatile unsigned int*)(SCP_IMER_BASE+0x0C))
//
//#define TIMER1_EN_REG                 ((volatile unsigned int*)(SCP_IMER_BASE+0x10))
//#define TIMER1_RST_VAL_REG        ((volatile unsigned int*)(SCP_IMER_BASE+0x14))
//#define TIMER1_CUR_VAL_REG        ((volatile unsigned int*)(SCP_IMER_BASE+0x18))
//#define TIMER1_IRQ_CTRL_REG       ((volatile unsigned int*)(SCP_IMER_BASE+0x1C))
//
//#define TIMER2_EN_REG                 ((volatile unsigned int*)(SCP_IMER_BASE+0x20))
//#define TIMER2_RST_VAL_REG        ((volatile unsigned int*)(SCP_IMER_BASE+0x24))
//#define TIMER2_CUR_VAL_REG        ((volatile unsigned int*)(SCP_IMER_BASE+0x28))
//#define TIMER2_IRQ_CTRL_REG       ((volatile unsigned int*)(SCP_IMER_BASE+0x2C))
//
//#define TIMER3_EN_REG                 ((volatile unsigned int*)(SCP_IMER_BASE+0x30))
//#define TIMER3_RST_VAL_REG        ((volatile unsigned int*)(SCP_IMER_BASE+0x34))
//#define TIMER3_CUR_VAL_REG        ((volatile unsigned int*)(SCP_IMER_BASE+0x38))
//#define TIMER3_IRQ_CTRL_REG       ((volatile unsigned int*)(SCP_IMER_BASE+0x3C))
//
//#define TIMER_CLK_SEL_REG         ((volatile unsigned int*)(SCP_IMER_BASE+0x40))

#define TIMER_ENABLE            1
#define TIMER_DISABLE           0

#define TIMER_IRQ_ENABLE            1
#define TIMER_IRQ_DISABLE           0

#define TIMER_IRQ_STA                   (0x1 << 4)
#define TIMER_IRQ_CLEAR                 (0x1 << 5)

#define TIMER_CLK_SRC_MCLK          (0x00)
#define TIMER_CLK_SRC_CLK_32K       (0x01)
#define TIMER_CLK_SRC_BCLK          (0x02)

#define TIMER_CLK_SEL_MASK      0x3
#define TIMER0_CLK_SEL_SHIFT    0
#define TIMER1_CLK_SEL_SHIFT    4
#define TIMER2_CLK_SEL_SHIFT    8
#define TIMER3_CLK_SEL_SHIFT    12

#define DELAY_TIMER_1US_TICK       ((unsigned int)1)           //     (32KHz)
#define DELAY_TIMER_1MS_TICK       ((unsigned int)33) //(32KHz)
// 32KHz: 31us = 1 counter
#define TIME_TO_TICK_US(us) ((us)*DELAY_TIMER_1US_TICK)
// 32KHz: 1ms = 33 counter
#define TIME_TO_TICK_MS(ms) ((ms)*DELAY_TIMER_1MS_TICK)

#define COUNT_TO_TICK(x) ((x)/32) //32KHz, 1 tick is 32 counters

#define US_LIMIT 31 /* udelay's parameter limit*/

#define GPT_CLEAR       2

#define GPT_ENABLE      1
#define GPT_DISABLE     0

#define GPT_CLK_SYS     (0x0 << 4)
#define GPT_CLK_RTC     (0x1 << 4)

typedef unsigned long mt_time_t;
//typedef void (*platform_timer_callback)(void *arg, mt_time_t now);
typedef void (*platform_timer_callback)(void *arg);

#ifdef DYNAMIC_TICK_RESOLUTION
#include <sys/types.h>
typedef enum TICK_OWNER_ID {
    SENSOR_FRAME = 0,
    SENSOR_AP,
    TICK_OWNER_END
} TICK_OWNER;

#define get_tick_interval()     tick_interval

extern mt_time_t tick_interval;
#endif      //DYNAMIC_TICK_RESOLUTION


//#define   SCP_TIMESYSTEM
#ifdef SCP_TIMESYSTEM

struct timespec {
    long        tv_sec;   /* seconds */
    long        tv_nsec;  /* nanoseconds */
};

struct timekeeper {
    /* Current CLOCK_REALTIME time in seconds */
    long        xtime_sec;
    /* Current CLOCK_REALTIME time nano seconds */
    long        xtime_nsec;
};

typedef enum {
    AP_WRITE_TIME = 0,
    AP_READ_TIME = 1,
} CMD_E;

typedef struct {
    long        tv_sec;         /* seconds */
    long        tv_nsec;        /* nanoseconds */
    long        cali_nsec;  /* compensate for IPI latency if needed */
    unsigned char   cmd;
} SCP_SYSTIME_SYNC;

#endif


/*************************For MD32 DVT*****************************************/
//#define GPT_DVT

/*************************End*****************************************/

extern void scp_systime_init(void);
extern void scp_systime_suspend(void);
extern void scp_systime_resume(void);
#ifdef SCP_TIMESYSTEM
extern void getnstimeofday(struct timespec *ts);
#endif
extern void gpt_busy_wait_us(unsigned int timeout_us);
extern void gpt_busy_wait_ms(unsigned int timeout_ms);

//extern unsigned long get_timer(unsigned long base);
extern void mdelay(unsigned long msec);
extern void udelay(unsigned long usec);

extern unsigned int delay_get_current_tick(void);
extern unsigned long long scp_system_tick(void);

extern int platform_set_periodic_timer(platform_timer_callback callback, void *arg, mt_time_t interval);

unsigned long long sched_clock(void);
void timer_ack_irq(unsigned int gpt_num);
void mt_platform_timer_init(void);
unsigned long timer_get_curval_tick(int id);
unsigned long long read_xgpt_stamp_ns(void);
int platform_set_periodic_timer_sensor1(platform_timer_callback callback, void *arg, mt_time_t interval);
void timer_sensor1_change_period(mt_time_t interval);
void timer_sensor1_stop(void);
void timer_sensor2_change_period(mt_time_t interval);
void timer_sensor2_stop(void);
int platform_set_periodic_timer_sensor2(platform_timer_callback callback, void *arg, mt_time_t interval);

//extern void mtk_timer_init(void);

//void gpt_one_shot_irq(unsigned int ms);
//int gpt_irq_init(void);
//void gpt_irq_ack();
//extern unsigned int gpt4_tick2time_us (unsigned int tick);
//extern unsigned int gpt4_get_current_tick (void);
//extern bool gpt4_timeout_tick (U32 start_tick, U32 timeout_tick);
//extern U32 gpt4_time2tick_us (U32 time_us);

#endif  /* !__MT_GPT_H__ */
