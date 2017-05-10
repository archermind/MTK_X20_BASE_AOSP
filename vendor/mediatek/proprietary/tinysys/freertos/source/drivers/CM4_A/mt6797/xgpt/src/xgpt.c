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

#include <driver_api.h>
#include <mt_reg_base.h>
#include <interrupt.h>
#include <mt_gpt.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <FreeRTOSConfig.h>
#include <main.h>
#include <task.h>
#include <platform.h>
#include <vcore_dvfs.h>
#include <tinysys_config.h>

//CLOCK_TIMER used to update timekeeper every 1 sec
#define AP_CLOCK_TIMER              0x6 /*AP's GPT6*/

#define CLOCK_TIMER             TMR0
#define CLOCK_TIMER_IRQ     TIMER0_IRQn

//TICK_TIMER used for system tick & softtimer
#define TICK_TIMER              TMR1
#define TICK_TIMER_IRQ      TIMER1_IRQn

//DELAY_TIMER used for mdelay()/udelay()
#define DELAY_TIMER             TMR2
#define DELAY_TIMER_RSTVAL          0xffffffff
#define MAX_RG_BIT         0xFFFFFFFF

#define SENSOR1_TIMER             TMR3
#define SENSOR1_TIMER_RSTVAL          0xffffffff
#define MAX_RG_BIT         0xFFFFFFFF

#define SENSOR2_TIMER             TMR4
#define SENSOR2_TIMER_RSTVAL          0xffffffff
#define MAX_RG_BIT         0xFFFFFFFF

#define GPT_BIT_MASK_L 0x00000000FFFFFFFF
#define GPT_BIT_MASK_H 0xFFFFFFFF00000000

SemaphoreHandle_t xgpt_mutex;

struct timer_device {
    unsigned int id;
    unsigned int base_addr;
    unsigned int irq_id;
};

#ifdef SCP_TIMESYSTEM
static struct timekeeper scp_timekeeper;
//static long sleep_time;
#endif

static struct timer_device scp_timer[NR_TMRS];
//static mutex_t tmr_mutex;


//static mt_time_t system_time = 0;
static volatile mt_time_t ticks = 0;
mt_time_t tick_interval;
mt_time_t sensor1_tick_interval;
mt_time_t sensor2_tick_interval;
static unsigned long long scp_system_time = 0;
static platform_timer_callback time_callback;
static platform_timer_callback time_sensor1_callback;
static platform_timer_callback time_sensor2_callback;

static void *callback_arg;
static void *sensor1_callback_arg;
static void *sensor2_callback_arg;


#ifdef DYNAMIC_TICK_RESOLUTION
volatile mt_time_t tick_owner[TICK_OWNER_END] = {MT_TIMER_TICK, MT_TIMER_TICK};
#endif

// 200MHz setting
#define MCLK_RATE  200*1024*1024        //200M
#define MCLK_1TICK_NS                   5                       //  1/(200M) = 5nsec
#define CLK32_1TICK_NS              31250               //  1/(32K) = 31250nsec
#define CLK32_1TICK_MS                              //  1/(32K) = 31250nsec

#define SEC_TO_NSEC             1024*1024*1024
#define TIMER_TICK_RATE     32768

static void __timer_disable(struct timer_device *dev);

static unsigned int __read_cpuxgpt(unsigned int reg_index)
{
    unsigned int value = 0;
    DRV_WriteReg32(INDEX_BASE, reg_index);
    value = DRV_Reg32(CTL_BASE);
    return value;
}

static struct timer_device *id_to_dev(unsigned int id)
{
    return id < NR_TMRS ? scp_timer + id : 0;
}

static void __timer_enable_irq(struct timer_device *dev)
{
    DRV_SetReg32(dev->base_addr + TIMER_IRQ_CTRL_REG, 0x1);
}

static void __timer_disable_irq(struct timer_device *dev)
{
    DRV_ClrReg32(dev->base_addr + TIMER_IRQ_CTRL_REG, 0x1);
}

static void __timer_ack_irq(struct timer_device *dev)
{
    DRV_SetReg32(dev->base_addr + TIMER_IRQ_CTRL_REG, TIMER_IRQ_CLEAR);
}

void timer_ack_irq(unsigned int gpt_num)
{
    struct timer_device *dev = id_to_dev(gpt_num);
    /*clear IRQ*/
    __timer_disable(dev);
    __timer_disable_irq(dev);
    __timer_ack_irq(dev);
}

#ifdef SCP_TIMESYSTEM
static int __timer_get_irqsta(struct timer_device *dev)
{
    unsigned int reg;
    reg = DRV_Reg32(dev->base_addr + TIMER_IRQ_CTRL_REG);
    if (reg & TIMER_IRQ_STA)
        return 1;
    return 0;
}
#endif
static void __timer_enable(struct timer_device *dev)
{
    DRV_SetReg32(dev->base_addr + TIMER_EN, 0x1);
}

static void __timer_disable(struct timer_device *dev)
{
    DRV_ClrReg32(dev->base_addr + TIMER_EN, 0x1);
}
#if 0
static void __timer_set_clk(struct timer_device *dev, unsigned int clksrc)
{
    DRV_ClrReg32(TIMER_CLK_SEL_REG, (0x03 << (4 * dev->id)));
    DRV_SetReg32(TIMER_CLK_SEL_REG, (clksrc << (4 * dev->id)));
}
#endif
static void __timer_set_rstval(struct timer_device *dev, unsigned int val)
{
    DRV_WriteReg32(dev->base_addr + TIMER_RST_VAL, val);
}

static void __timer_get_curval(struct timer_device *dev, unsigned long *ptr)
{
    *ptr = DRV_Reg32(dev->base_addr + TIMER_CUR_VAL_REG);
}

/* used CPUxGPT for time sync with AP side, para number is not use*/
static void __timer_get_AP_counter(unsigned int number, unsigned long *ptr)
{
    *ptr = __read_cpuxgpt(L_CNT64);
    if (number == AP_CLOCK_TIMER) {
        *(++ptr) = __read_cpuxgpt(H_CNT64);
    }
}
static void __timer_reset(struct timer_device *dev)
{
    __timer_disable(dev);
    __timer_disable_irq(dev);
    __timer_ack_irq(dev);
    __timer_set_rstval(dev, 0);
    /* __timer_set_clk(dev, TIMER_CLK_SRC_CLK_32K); */
}

unsigned long long mt_gpt_read(void)
{
    unsigned long long cycles;
    unsigned long cnt[2] = {0, 0};
    __timer_get_AP_counter(AP_CLOCK_TIMER, cnt);

    if (AP_CLOCK_TIMER != 0x6) {
        /*
           * force do mask for high 32-bit to avoid unpredicted alignment
           */
        cycles = (GPT_BIT_MASK_L & (unsigned long long)(cnt[0]));
    } else {
        cycles = (GPT_BIT_MASK_H & (((unsigned long long)(cnt[1])) << 32)) | (GPT_BIT_MASK_L & ((unsigned long long)(cnt[0])));
    }

    return cycles;
}

unsigned long xgpt_tick = 0;
static unsigned long long read_xgpt_stamp_cnt(int id)
{
    unsigned long current_counter = 0;

    __timer_get_curval(id_to_dev(id), &current_counter);
    return ((xgpt_tick * MAX_RG_BIT) + (MAX_RG_BIT - current_counter));
}

unsigned long long mt_gpt_cycles_p = 0;

unsigned long long read_xgpt_stamp_ns(void)
{
    unsigned long long temp = ((read_xgpt_stamp_cnt(CLOCK_TIMER) * 1000000000ULL + mt_gpt_cycles_p) / TIMER_TICK_RATE);

    mt_gpt_cycles_p = (read_xgpt_stamp_cnt(CLOCK_TIMER) * 1000000000ULL + mt_gpt_cycles_p) % TIMER_TICK_RATE;

    return temp;
}

void update_xgpt_tick(void)
{
    struct timer_device *dev = id_to_dev(CLOCK_TIMER);

    xgpt_tick = xgpt_tick + 1;

    __timer_disable(dev);
    __timer_disable_irq(dev);
    __timer_ack_irq(dev);
    __timer_set_rstval(dev, MAX_RG_BIT);
    __timer_enable_irq(dev);
    __timer_enable(dev);
}

/* sync time with AP side*/
unsigned long long sched_clock(void)
{
    unsigned long long cycles;

    cycles = mt_gpt_read();

    return (cycles * AP_NS_PER_CNT);
}

/*GPT's clock need changed to 32K*/
void platform_timer_suspend(void)
{
    unsigned long current_count;
    unsigned long long map_count;
    struct timer_device *dev = id_to_dev(TICK_TIMER);
    kal_taskENTER_CRITICAL();
    /*disable GPT */
    __timer_disable(dev);
    /*clear IRQ*/
    __timer_ack_irq(dev);
    /*get current counter*/
    __timer_get_curval(dev, &current_count);
    /*map counter from 200MHz to 32K*/
    map_count = (TIMER_TICK_RATE * current_count) / MCLK_RATE;
    if (map_count == 0)
        map_count = 1;
    PRINTF_D("platform_timer_suspend map_count=0x%lu\n", map_count);
    /*set GPT load value*/
    __timer_set_rstval(dev, map_count);
    /*enable GPT*/
    __timer_enable(dev);
    kal_taskEXIT_CRITICAL();
}

/*GPT's clock need changed to 200MHz*/
void platform_timer_resume(void)
{
    unsigned long current_count;
    unsigned long long map_count;
    struct timer_device *dev = id_to_dev(TICK_TIMER);
    kal_taskENTER_CRITICAL();
    /*disable GPT */
    __timer_disable(dev);
    /*clear IRQ*/
    __timer_ack_irq(dev);
    /*get current counter*/
    __timer_get_curval(dev, &current_count);
    /*map current counter from 32K to 200MHz*/
    map_count = current_count * (MCLK_RATE / TIMER_TICK_RATE);
    if (map_count > MAX_RG_BIT)
        map_count = MAX_RG_BIT;
    PRINTF_D("platform_timer_resume map_count=0x%lu\n", map_count);
    /*set GPT load value*/
    __timer_set_rstval(dev, map_count);
    /*enable GPT*/
    __timer_enable(dev);
    kal_taskEXIT_CRITICAL();
}

void mt_platform_timer_init(void)
{
    int i;
    struct timer_device *dev;
    /* enable clock */
    DRV_SetReg32(TIMER_IN_CLK, 0x7);

    for (i = 0; i < NR_TMRS; i++) {
        scp_timer[i].id = i;
        scp_timer[i].base_addr = SCP_IMER_BASE + 0x10 * i;
        scp_timer[i].irq_id = TIMER0_IRQn + i;
    }

    // reset timer
    for (i = 0; i < NR_TMRS; i++) {
        __timer_reset(&scp_timer[i]);
    }

    scp_wakeup_src_setup(TIMER0_WAKE_CLK_CTRL, 1);
    scp_wakeup_src_setup(TIMER1_WAKE_CLK_CTRL, 1);
    scp_wakeup_src_setup(TIMER2_WAKE_CLK_CTRL, 1);
#ifdef CFG_SENSORHUB_SUPPORT
    scp_wakeup_src_setup(TIMER3_WAKE_CLK_CTRL, 1);
    scp_wakeup_src_setup(TIMER4_WAKE_CLK_CTRL, 1);
#endif

//    /*
    dev = id_to_dev(CLOCK_TIMER);
    request_irq(dev->irq_id, update_xgpt_tick, "Tick_Timer");
    __timer_set_rstval(dev, MAX_RG_BIT);
    __timer_enable_irq(dev);
    __timer_enable(dev);
//    */
    /* enable delay GPT */
    dev = id_to_dev(DELAY_TIMER);
    __timer_set_rstval(dev, DELAY_TIMER_RSTVAL);
    __timer_enable(dev);

    xgpt_mutex = xSemaphoreCreateMutex();
}

static void timer_irq(void *arg)
{
    /*diff with before*/
    //ticks += tick_interval;
    //scp_system_time++;
    if (time_callback != NULL)
        return time_callback(callback_arg);
}

void timer_irq_handle(void)
{
    //unsigned int i;
    //mt_time_t tick_tmp;
    //static int ret;
    struct timer_device *dev = id_to_dev(TICK_TIMER);
    /*
        #if CLK_CTRL_MODE == MODE_1_CLK_HIGH_ONLY || CLK_CTRL_MODE == MODE_2_CLK_HIGH_AND_SYS
        #ifdef SCP_WAKEUP_BY_TIMER_TEST
        DRV_WriteReg32(CFGREG_BASE+0x20, 0x1);
        #endif
        #endif
    */
    //kal_taskENTER_CRITICAL();

    __timer_disable(dev);
    __timer_disable_irq(dev);
    __timer_ack_irq(dev);
    timer_irq(0);

#ifdef DYNAMIC_TICK_RESOLUTION
    if (tick_interval != MT_TIMER_TICK) {
        tick_tmp = tick_owner[0];
        for (i = 1; i < TICK_OWNER_END; i++) {
            if (tick_tmp > tick_owner[i])
                tick_tmp = tick_owner[i];
        }
        if (tick_interval != tick_tmp) {
            tick_interval = tick_tmp;
            __timer_set_rstval(dev, TIMER_TICK_RATE * tick_interval / 1000);
        }
    }
#endif
    //__timer_set_rstval(dev, TIMER_TICK_RATE*tick_interval/1000); /*diff with before*/
    //__timer_enable(dev);

    //getnstimeofday(&timestamp);
    //printf("lk_scheduler tv_sec= 0x%x, tv_nsec= 0x%x \n", timestamp.tv_sec, timestamp.tv_nsec);
    //printf("lk_scheduler tick_interval=%d, ticks=%d, tick_owner[0]=%d, tick_owner[1]=%d \n",
    //tick_interval, ticks, tick_owner[0], tick_owner[1]);

    //mtk_wdt_restart();

    //kal_taskEXIT_CRITICAL();
    return ;
}

int platform_set_periodic_timer(platform_timer_callback callback, void *arg, mt_time_t interval)
{
    //unsigned int reg_tmp;
    time_callback = callback;
    tick_interval = interval;
    callback_arg  = arg;
    struct timer_device *dev;

    //mt_platform_timer_init();

    dev = id_to_dev(TICK_TIMER);
    __timer_disable(dev);

    if (interval >= 1)
        __timer_set_rstval(dev, TIMER_TICK_RATE * interval / 1000 - 32); //0.3ms(sw)+0.7ms(hw wake)
    else
        __timer_set_rstval(dev, 1);
    __timer_enable_irq(dev);

    //unmask_irq(TICK_TIMER_IRQ);
    request_irq(dev->irq_id, timer_irq_handle, "Timer");

#ifdef MTK_SENSOR_HUB_SUPPORT
    /* scp_wakeup_src_setup(TICK_TIMER_IRQ, 1); */
#endif
    __timer_enable(dev);

#if 0   //diff with before
    dev = id_to_dev(DELAY_TIMER);
    __timer_set_clk(dev, TIMER_CLK_SRC_MCLK);
    __timer_set_rstval(dev, DELAY_TIMER_RSTVAL);
    __timer_enable(dev);
#endif
    return 0;
}

void timer_sensor1_change_period(mt_time_t interval)
{
    struct timer_device *dev = id_to_dev(SENSOR1_TIMER);

    __timer_disable(dev);

    __timer_set_rstval(dev, TIMER_TICK_RATE * interval / 1000);
    __timer_enable_irq(dev);

    __timer_enable(dev);

}
void timer_sensor1_stop(void)
{
    struct timer_device *dev = id_to_dev(SENSOR1_TIMER);

    __timer_disable(dev);
}

static void timer_sensor1_irq(void *arg)
{
    /*diff with before*/
    //ticks += tick_interval;
    //scp_system_time++;
    if (time_sensor1_callback != NULL)
        return time_sensor1_callback(sensor1_callback_arg);
}

void timer_irq_sensor1_handle(void)
{
    struct timer_device *dev = id_to_dev(SENSOR1_TIMER);
    //PRINTF_D("timer_irq_sensor1_handle++++++++\n");
    __timer_disable(dev);
    __timer_disable_irq(dev);
    __timer_ack_irq(dev);
    timer_sensor1_irq(0);

    return ;
}

int platform_set_periodic_timer_sensor1(platform_timer_callback callback, void *arg, mt_time_t interval)
{
    //unsigned int reg_tmp;
    time_sensor1_callback = callback;
    sensor1_tick_interval = interval;
    sensor1_callback_arg  = arg;
    struct timer_device *dev;

    //mt_platform_timer_init();

    dev = id_to_dev(SENSOR1_TIMER);
    if (interval >= 1)
        __timer_set_rstval(dev, TIMER_TICK_RATE * interval / 1000); //0.3ms(sw)+0.7ms(hw wake)
    else
        __timer_set_rstval(dev, 1);
    __timer_enable_irq(dev);



    //unmask_irq(TICK_TIMER_IRQ);
    request_irq(dev->irq_id, timer_irq_sensor1_handle, "Sensor-Timer1");

    __timer_enable(dev);

#if 0   //diff with before
    dev = id_to_dev(DELAY_TIMER);
    __timer_set_clk(dev, TIMER_CLK_SRC_MCLK);
    __timer_set_rstval(dev, DELAY_TIMER_RSTVAL);
    __timer_enable(dev);
#endif
    return 0;
}

void timer_sensor2_change_period(mt_time_t interval)
{
    struct timer_device *dev = id_to_dev(SENSOR2_TIMER);

    __timer_disable(dev);

    __timer_set_rstval(dev, TIMER_TICK_RATE * interval / 1000);
    __timer_enable_irq(dev);

    __timer_enable(dev);

}
void timer_sensor2_stop(void)
{
    struct timer_device *dev = id_to_dev(SENSOR2_TIMER);

    __timer_disable(dev);
}

static void timer_sensor2_irq(void *arg)
{
    /*diff with before*/
    //ticks += tick_interval;
    //scp_system_time++;
    if (time_sensor2_callback != NULL)
        return time_sensor2_callback(sensor2_callback_arg);
}

void timer_irq_sensor2_handle(void)
{
    struct timer_device *dev = id_to_dev(SENSOR2_TIMER);
    //PRINTF_D("timer_irq_sensor1_handle++++++++\n");
    __timer_disable(dev);
    __timer_disable_irq(dev);
    __timer_ack_irq(dev);
    timer_sensor2_irq(0);

    return ;
}

int platform_set_periodic_timer_sensor2(platform_timer_callback callback, void *arg, mt_time_t interval)
{
    //unsigned int reg_tmp;
    time_sensor2_callback = callback;
    sensor2_tick_interval = interval;
    sensor2_callback_arg  = arg;
    struct timer_device *dev;

    //mt_platform_timer_init();

    dev = id_to_dev(SENSOR2_TIMER);
    if (interval >= 1)
        __timer_set_rstval(dev, TIMER_TICK_RATE * interval / 1000); //0.3ms(sw)+0.7ms(hw wake)
    else
        __timer_set_rstval(dev, 1);
    __timer_enable_irq(dev);



    //unmask_irq(TICK_TIMER_IRQ);
    request_irq(dev->irq_id, timer_irq_sensor2_handle, "Sensor-Timer2");

    __timer_enable(dev);

#if 0   //diff with before
    dev = id_to_dev(DELAY_TIMER);
    __timer_set_clk(dev, TIMER_CLK_SRC_MCLK);
    __timer_set_rstval(dev, DELAY_TIMER_RSTVAL);
    __timer_enable(dev);
#endif
    return 0;
}

/*
* this API is requeired by Power
*/
void xGPT_wakeup(mt_time_t idle_time)
{
    platform_set_periodic_timer(NULL, NULL, idle_time);
}

mt_time_t current_time(void)
{
    return ticks;
}

unsigned long long scp_system_tick(void)
{
    return (scp_system_time);
}

unsigned long timer_get_curval_tick(int id)
{
    unsigned long current_count;
    struct timer_device *dev = id_to_dev(id);

    __timer_get_curval(dev, &current_count);

    return COUNT_TO_TICK(current_count); //return TICK
}
//===========================================================================
// SCP time system
//===========================================================================
#ifdef SCP_TIMESYSTEM
int time_update_handler(void)
{
    struct timer_device *dev = id_to_dev(CLOCK_TIMER);

    if (xSemaphoreTake(xgpt_mutex, portMAX_DELAY) == pdTRUE) {
        __timer_ack_irq(dev);
        scp_timekeeper.xtime_sec += 1;
        xSemaphoreGive(xgpt_mutex);
    } else {
        PRINTF_D("time_update_handler cannot get mutex\n");
    }

    return 0;
}

void time_sync_handler(int id, void *data, unsigned int len)
{
    //SCP_SYSTIME_SYNC sync_data;
    struct timer_device *dev = id_to_dev(CLOCK_TIMER);
    if (data == 0)
        return;

    if (xSemaphoreTake(xgpt_mutex, portMAX_DELAY) == pdTRUE) {
        switch (((SCP_SYSTIME_SYNC *)data)->cmd) {
            case AP_WRITE_TIME:
                __timer_disable(dev);
                __timer_ack_irq(dev);

                scp_timekeeper.xtime_sec = ((SCP_SYSTIME_SYNC *)data)->tv_sec;
                scp_timekeeper.xtime_nsec = ((SCP_SYSTIME_SYNC *)data)->tv_nsec;

                __timer_enable(dev);
                break;

            case AP_READ_TIME:
                //sync_data.tv_sec = scp_timekeeper.xtime_sec;
                //sync_data.tv_nsec = scp_timekeeper.xtime_nsec;
                //sync_data.cmd = AP_READ_TIME;

                //md32_ipi_send(IPI_XXX, &sync_data, sizeof(sync_data), 0);
                break;

            default:
                break;
        }
        xSemaphoreGive(xgpt_mutex);
    } else {
        PRINTF_D("time_sync_handler cannot get mutex\n");
    }
}

void scp_systime_init(void)
{
    struct timer_device *dev = id_to_dev(CLOCK_TIMER);
    //mutex_init(&tmr_mutex);
    //md32_ipi_registration(IPI_XXX, time_sync_handler, "SYSTEM TIME IPI");

    __timer_disable(dev);
    __timer_set_clk(dev, TIMER_CLK_SRC_MCLK);
    __timer_set_rstval(dev, MCLK_RATE);
    __timer_enable_irq(dev);
    //unmask_irq(CLOCK_TIMER_IRQ);
    //request_irq(CLOCK_TIMER_IRQ, time_update_handler, "CLOCK_TIME");

#ifdef SCP_TIMESYSTEM_TEMP
    __timer_enable(dev);
#endif

}

void scp_systime_suspend(void)
{
    unsigned long current_count, duration;
    struct timer_device *dev;

    //printf("scp_systime_suspend\n");
    //mtk_wdt_disable();
    //mtk_wdt_restart();
    if (xSemaphoreTake(xgpt_mutex, portMAX_DELAY) == pdTRUE) {

        dev = id_to_dev(CLOCK_TIMER);
        __timer_disable(dev);

        //enter_critical_section();
        if (__timer_get_irqsta(dev)) {
            __timer_ack_irq(dev);
            scp_timekeeper.xtime_sec += 1;
        } else {
            __timer_get_curval(dev, &current_count);
            duration = (MCLK_RATE - current_count) * MCLK_1TICK_NS;
            if ((SEC_TO_NSEC - scp_timekeeper.xtime_nsec) < duration) {
                scp_timekeeper.xtime_sec += 1;
                scp_timekeeper.xtime_nsec = duration - (SEC_TO_NSEC - scp_timekeeper.xtime_nsec);
            } else {
                scp_timekeeper.xtime_nsec += duration;
            }
        }
        xSemaphoreGive(xgpt_mutex);
    } else {
        PRINTF_D("scp_systime_suspend cannot get mutex\n");
    }
}

void scp_systime_resume(void)
{
    //unsigned long current_count, duration_s, duration_ns;
    struct timer_device *dev;

    //printf("scp_systime_resume\n");
    //mtk_wdt_init();
    //mtk_wdt_restart();
    if (xSemaphoreTake(xgpt_mutex, portMAX_DELAY) == pdTRUE) {

        //enable CLOCK_TIMER by MCLK
        dev = id_to_dev(CLOCK_TIMER);
        __timer_enable(dev);

        xSemaphoreGive(xgpt_mutex);
    } else {
        PRINTF_D("scp_systime_resume cannot get mutex\n");
    }
}

void getnstimeofday(struct timespec *ts)
{
    unsigned long current_count, duration;
    struct timer_device *dev = id_to_dev(CLOCK_TIMER);

    __timer_get_curval(dev, &current_count);
    duration = (MCLK_RATE - current_count) * MCLK_1TICK_NS;
    if ((SEC_TO_NSEC - scp_timekeeper.xtime_nsec) < duration) {
        ts->tv_sec = scp_timekeeper.xtime_sec + 1;
        ts->tv_nsec = duration - (SEC_TO_NSEC - scp_timekeeper.xtime_nsec);
    } else {
        ts->tv_sec = scp_timekeeper.xtime_sec;
        ts->tv_nsec = scp_timekeeper.xtime_nsec + duration;
    }
}
#endif



//===========================================================================
// delay function
//===========================================================================
#if 1
unsigned int delay_get_current_tick(void)
{
    unsigned long current_count;
    struct timer_device *dev = id_to_dev(DELAY_TIMER);

    __timer_get_curval(dev, &current_count);
    return current_count;
}

int check_timeout_tick(unsigned int start_tick, unsigned int timeout_tick)
{
    //register unsigned int cur_tick;
    //register unsigned int elapse_tick;
    unsigned int cur_tick;
    unsigned int elapse_tick;
    // get current tick
    cur_tick = delay_get_current_tick();

    // check elapse time, down counter
    if (start_tick >= cur_tick) {
        elapse_tick = start_tick - cur_tick;
    } else {
        elapse_tick = (DELAY_TIMER_RSTVAL - cur_tick) + start_tick;
    }
    // check if timeout
    if (timeout_tick <= elapse_tick) {
        // timeout
        return 1;
    }

    return 0;
}

unsigned int time2tick_us(unsigned int time_us)
{
    return TIME_TO_TICK_US(time_us);
}

static unsigned int time2tick_ms(unsigned int time_ms)
{
    return TIME_TO_TICK_MS(time_ms);
}

//===========================================================================
// busy wait
//===========================================================================
void busy_wait_us(unsigned int timeout_us)
{
#if 1
    unsigned int start_tick, timeout_tick;

    // get timeout tick
    timeout_tick = time2tick_us(timeout_us);
    start_tick = delay_get_current_tick();

    // wait for timeout
    while (!check_timeout_tick(start_tick, timeout_tick));
#else
    unsigned long i;
    unsigned long count = timeout_us * 0x100;

    for (i = 0; i < count; i++)
        ;
#endif
}

void busy_wait_ms(unsigned int timeout_ms)
{
#if 1
    unsigned int start_tick, timeout_tick;

    // get timeout tick
    timeout_tick = time2tick_ms(timeout_ms);
    start_tick = delay_get_current_tick();

    // wait for timeout
    while (!check_timeout_tick(start_tick, timeout_tick));
#else
    unsigned long count = timeout_ms * 0x10000;
    unsigned long i;

    for (i = 0; i < count; i++)
        ;
#endif
}

/* delay msec mseconds */
void mdelay(unsigned long msec)
{
    busy_wait_ms(msec);
}

/* delay usec useconds */
void udelay(unsigned long usec)
{
    unsigned long usec_t;
    if (usec < US_LIMIT) {
        //PRINTF_D("usec < 31us, error parameter\n");
        busy_wait_us(1);
    } else {
        usec_t = usec / 31 + 1;
        busy_wait_us(usec_t);
    }
}
#endif

//======================================================================
