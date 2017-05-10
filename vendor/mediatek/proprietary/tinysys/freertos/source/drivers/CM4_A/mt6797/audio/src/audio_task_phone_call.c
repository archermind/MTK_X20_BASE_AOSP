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

#include "audio_task_phone_call.h"

#include <stdarg.h> /* va_list, va_start, va_arg, va_end */

#include <interrupt.h>
#include <dma.h>
#include <feature_manager.h>
#include <vcore_dvfs.h>
#include <scp_ipi.h>

#include "audio_messenger_ipi.h"
#include "audio_speech_msg_id.h"

#include "arsi_api.h"
#include "dsp_speech_intf.h"


/*==============================================================================
 *                     Time releated // TODO: move to audio_timer.h
 *============================================================================*/

#define TINYSYS_TICKS_PER_US   (354) // TODO: get from system call
#define TINYSYS_TICKS_PER_MS   (TINYSYS_TICKS_PER_US * 1000)

volatile uint32_t *ITM_CONTROL = (uint32_t *)0xE0000E80;
volatile uint32_t *DWT_CONTROL = (uint32_t *)0xE0001000;
volatile uint32_t *DWT_CYCCNT = (uint32_t *)0xE0001004;
volatile uint32_t *DEMCR = (uint32_t *)0xE000EDFC;

#define CPU_RESET_CYCLECOUNTER() \
    do { \
        *DEMCR = *DEMCR | 0x01000000; \
        *DWT_CYCCNT = 0; \
        *DWT_CONTROL = *DWT_CONTROL | 1 ; \
    } while(0)

#define GET_CURRENT_TICK() (*DWT_CYCCNT)
#define GET_TICK_INTERVAL(stop_tick, start_tick) \
    (((stop_tick) > (start_tick)) \
     ? ((stop_tick) - (start_tick)) \
     : ((0xFFFFFFFF - (start_tick)) + (stop_tick) + 1))

#define TICK_TO_US(tick) ((tick) / (TINYSYS_TICKS_PER_US))
#define TICK_TO_MS(tick) ((tick) / (TINYSYS_TICKS_PER_MS))

/*==============================================================================
 *                     Types
 *============================================================================*/

enum tty_mode_t {
    AUD_TTY_OFF  = 0,
    AUD_TTY_FULL = 1,
    AUD_TTY_VCO  = 2,
    AUD_TTY_HCO  = 4,
    AUD_TTY_ERR  = -1
};


/*==============================================================================
 *                     MACRO
 *============================================================================*/

#define LOCAL_TASK_STACK_SIZE (1024)
#define LOCAL_TASK_NAME "call"
#define LOCAL_TASK_PRIORITY (3)

#define MAX_MSG_QUEUE_SIZE (8)

/* TODO: calculate it */
#define FRAME_BUF_SIZE   (640)
#define UL_IN_BUF_SIZE   (FRAME_BUF_SIZE * 2)
#define UL_OUT_BUF_SIZE  (FRAME_BUF_SIZE)
#define AEC_BUF_SIZE     (FRAME_BUF_SIZE)
#define DL_IN_BUF_SIZE   (FRAME_BUF_SIZE)
#define DL_OUT_BUF_SIZE  (FRAME_BUF_SIZE)

#define MAX_PARAM_SIZE   (4096)

#define MAX_UL_PROCESS_TIME_MS (10)
#define MAX_DL_PROCESS_TIME_MS (3)

#define MAX_UL_IRQ_TIME_MS (11)
#define MAX_DL_IRQ_TIME_MS (3)

#define MAX_UL_PROCESS_CYCLE_COUNT (MAX_UL_PROCESS_TIME_MS * TINYSYS_TICKS_PER_MS)
#define MAX_DL_PROCESS_CYCLE_COUNT (MAX_DL_PROCESS_TIME_MS * TINYSYS_TICKS_PER_MS)
#define MAX_UL_IRQ_CYCLE_COUNT (MAX_UL_IRQ_TIME_MS * TINYSYS_TICKS_PER_MS)
#define MAX_DL_IRQ_CYCLE_COUNT (MAX_DL_IRQ_TIME_MS * TINYSYS_TICKS_PER_MS)


/* DEBUG FLAGS */
//#define SWAP_DL_UL_LOOPBACK
//#define RESPONSE_IRQ_TO_MODEM_IMMEDIATELY


/*==============================================================================
 *                     private global members
 *============================================================================*/

static uint32_t max_dl_process_tick;
static uint32_t max_ul_process_tick;
static uint32_t max_dl_irq_tick;
static uint32_t max_ul_irq_tick;

static uint32_t irq_start_tick;
static uint32_t irq_stop_tick;


static arsi_task_config_t arsi_task_config;
static data_buf_t         param_buf;
static uint8_t            flag_lib_update_state;
static uint8_t            dump_pcm_flag;
static uint8_t            lib_log_flag;


typedef struct pcm_dump_ul_t {
    char ul_in_ch1[FRAME_BUF_SIZE];
    char ul_in_ch2[FRAME_BUF_SIZE];
    char aec_in[FRAME_BUF_SIZE];
    char ul_out[FRAME_BUF_SIZE];
} pcm_dump_ul_t;

typedef struct pcm_dump_dl_t {
    char dl_in[FRAME_BUF_SIZE];
    char dl_out[FRAME_BUF_SIZE];
} pcm_dump_dl_t;


#define RSRV_DRAM_SZ (0x200000)
static void *rsrv_dram_phy_base;
static void *rsrv_dram_phy_end;
static void *rsrv_dram_phy_cur;

static extra_call_arg_t extra_call_arg;

static void *arsi_handler;

static data_buf_t working_buf;

static audio_buf_t ul_buf_in;
static audio_buf_t ul_buf_out;
static audio_buf_t dl_buf_in;
static audio_buf_t dl_buf_out;
static audio_buf_t aec_buf_in;

#ifndef SHARESEC
#define SHARESEC __attribute__ ((section (".share")))
#endif

static uint16_t shared_tcm_buf_ul[UL_OUT_BUF_SIZE / sizeof(uint16_t)] SHARESEC;
static uint16_t shared_tcm_buf_dl[DL_OUT_BUF_SIZE / sizeof(uint16_t)] SHARESEC;

#ifdef SWAP_DL_UL_LOOPBACK
static uint16_t *ul_tmp_buf;
static uint16_t *dl_tmp_buf;
#endif

static uint8_t irq_flag;
static uint32_t irq_idx;
static uint32_t irq_cnt_ul;
static uint32_t irq_cnt_dl;
static uint32_t irq_cnt_bypass_handshake_error;
static uint32_t irq_cnt_bypass_level_trigger;
static uint32_t irq_cnt_bypass_speech_off;


static uint16_t old_modem_data_handshake;


static uint8_t enh_on_ul;
static uint8_t enh_on_dl;

static uint8_t enh_on_tty_ul;
static uint8_t enh_on_tty_dl;


/*==============================================================================
 *                     derived functions - declaration
 *============================================================================*/

static void           task_phone_call_constructor(struct AudioTask *this);
static void           task_phone_call_destructor(struct AudioTask *this);

static void           task_phone_call_create_task_loop(struct AudioTask *this);

static audio_status_t task_phone_call_recv_message(
    struct AudioTask *this,
    struct ipi_msg_t *ipi_msg);

static void           task_phone_call_irq_hanlder(
    struct AudioTask *this,
    uint32_t irq_type);


/*==============================================================================
 *                     private functions - declaration
 *============================================================================*/

static void           task_phone_call_parsing_message(
    struct AudioTask *this,
    ipi_msg_t *ipi_msg);

static void           task_phone_call_task_loop(void *pvParameters);

static audio_status_t task_phone_call_init(struct AudioTask *this);
static audio_status_t task_phone_call_working(
    struct AudioTask *this,
    ipi_msg_t *ipi_msg);
static audio_status_t task_phone_call_deinit(struct AudioTask *this);

typedef enum {
    MODEM_UL_DATA = 0x1,
    MODEM_DL_DATA = 0x2,
    BYPASS_DATA   = 0xFF,
} modem_data_handshake_t;

static uint16_t       get_modem_data_handshake();
static void           notify_modem_data_arrival(
    struct AudioTask *this,
    uint16_t *buf,
    uint16_t modem_data_handshake);
static void           check_irq_timeout(uint16_t line,
                                        uint16_t modem_data_handshake);

static void           myprint(const char *message, ...);


/*==============================================================================
 *                     class new/construct/destruct functions
 *============================================================================*/

#if 0
static void vTestTaskPlatform(void *pvParameters)
{
    AudioTask *this = (AudioTask *)pvParameters;

    uint32_t dm_addr = 0;
    uint32_t i = 0;

    uint32_t len = 0x3c0;
    uint32_t base = 0xCD250000;

    uint32_t len_sherif = 0x4FE;
    uint32_t base_sherif = 0xCD207300;


    uint16_t buf[len / sizeof(uint16_t)];
    uint16_t DM_data_2bytes = 0;
    PRINTF_D("buf = %p\n", buf);

    memset(buf, 0xAB, len);

    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        if (this->state != AUDIO_TASK_WORKING) {
            continue;
        }


        dma_transaction_16bit((uint32_t)buf, (uint32_t)base, len);

        for (i = 0; i < len / sizeof(uint16_t); i++) {
            dm_addr = base + 2 * i;
            //drv_write_reg16(dm_addr, i); // sherif
            DM_data_2bytes = drv_reg16(dm_addr);
            if (i != buf[i] || i != DM_data_2bytes) {
                PRINTF_D("DMA buf(0x%x) = 0x%x, single read 2 bytes(0x%x) = 0x%x\n", i, buf[i],
                         dm_addr, DM_data_2bytes);
                break;
            }
        }
        if (i == len / sizeof(uint16_t)) {
            PRINTF_D("DM PASS\n");
        }
        else {
            PRINTF_D("DM FAIL\n");
        }

        for (i = 0; i < len_sherif / sizeof(uint16_t); i++) {
            dm_addr = base_sherif + 2 * i;
            drv_write_reg16(dm_addr, i); // sherif
            DM_data_2bytes = drv_reg16(dm_addr);
            if (i != DM_data_2bytes) {
                PRINTF_D("i = %d, sherif(0x%x) = 0x%x\n", i, dm_addr, DM_data_2bytes);
                break;
            }
        }
        if (i == len_sherif / sizeof(uint16_t)) {
            PRINTF_D("sherif PASS\n");
        }
        else {
            PRINTF_D("sherif FAIL\n");
        }
        PRINTF_D("\n\n\n");

    }
}
#endif

AudioTask *task_phone_call_new()
{
    /* alloc object here */
    AudioTask *task = (AudioTask *)kal_pvPortMalloc(sizeof(AudioTask));
    if (task == NULL) {
        AUD_LOG_E("%s(), kal_pvPortMalloc fail!!\n", __func__);
        return NULL;
    }

    /* only assign methods, but not class members here */
    task->constructor       = task_phone_call_constructor;
    task->destructor        = task_phone_call_destructor;

    task->create_task_loop  = task_phone_call_create_task_loop;

    task->recv_message      = task_phone_call_recv_message;

    task->irq_hanlder       = task_phone_call_irq_hanlder;


    return task;
}


static void task_phone_call_constructor(struct AudioTask *this)
{
    AUD_ASSERT(this != NULL);

    /* assign initial value for class members & alloc private memory here */
    this->scene = TASK_SCENE_PHONE_CALL;
    this->state = AUDIO_TASK_IDLE;

#if 1 // TODO: put init somewhere
    /* merge_en register = 0. 0x10003204. PERIAXI_BUS_CTL2[2]*/
    drv_write_reg32(0xA0003204, drv_reg32(0xA0003204) & (~0x00000004));

    /* enable all interrupt from MD to CM4 */
    //PRINTF_E("[SCP]enable MD to SCP interrupt\n\r");
    drv_write_reg32(0x400A202C, drv_reg32(0x400A202C) | 0x00004000);

    // enable SPM clk: SCP_APB_INTERN AL_EN
    drv_write_reg32(0xA000601C, drv_reg32(0xA000601C | (0x0b16 << 16) | 0x4000));

    // enable p2p cg
    drv_write_reg32(0x400A4030, drv_reg32(0x400A4030) | 0x00000040);

#if 0
    AUD_LOG_D("drv_reg32(0xA0003204) = 0x%x\n\r", drv_reg32(0xA0003204));
    AUD_LOG_D("drv_reg32(0x400A202C) = 0x%x\n\r", drv_reg32(0x400A202C));
    AUD_LOG_D("drv_reg32(0xA000601C) = 0x%x\n\r", drv_reg32(0xA000601C));
    AUD_LOG_D("drv_reg32(0x400A4030) = 0x%x\n\r", drv_reg32(0x400A4030));
    AUD_LOG_D("drv_reg16(0x400AC43c) = 0x%x\n\r", (uint16_t)drv_reg16(0x400AC43c));
#endif
#endif


    /* queue */
    this->queue_idx = 0;

    this->msg_array = (ipi_msg_t *)kal_pvPortMalloc(
                          MAX_MSG_QUEUE_SIZE * sizeof(ipi_msg_t));
    AUD_ASSERT(this->msg_array != NULL);

    this->msg_idx_queue = xQueueCreate(MAX_MSG_QUEUE_SIZE, sizeof(uint8_t));
    AUD_ASSERT(this->msg_idx_queue != NULL);


    /* speech enhancement parameters */
    param_buf.memory_size = MAX_PARAM_SIZE;
    param_buf.data_size = 0;
    param_buf.p_buffer = kal_pvPortMalloc(param_buf.memory_size);
    AUD_LOG_V("param_buf.memory_size = %u\n", param_buf.memory_size);
    AUD_LOG_V("param_buf.p_buffer = %p\n", param_buf.p_buffer);


    modem_index = 0xFF;

    max_dl_process_tick = 0;
    max_ul_process_tick = 0;
    max_dl_irq_tick = 0;
    max_ul_irq_tick = 0;

    irq_start_tick     = 0;
    irq_stop_tick      = 0;

    arsi_handler = NULL;
    rsrv_dram_phy_base = NULL;
    rsrv_dram_phy_end = NULL;
    rsrv_dram_phy_cur = NULL;

    flag_lib_update_state = 0;
    dump_pcm_flag = 0;
    lib_log_flag = 0;

    enh_on_ul = 1;
    enh_on_dl = 1;
    enh_on_tty_ul = 1;
    enh_on_tty_dl = 1;
}


static void task_phone_call_destructor(struct AudioTask *this)
{
    AUD_LOG_D("%s(), task_scene = %d", __func__, this->scene);

    /* dealloc private memory & dealloc object here */
    AUD_ASSERT(this != NULL);

    kal_vPortFree(param_buf.p_buffer);

    kal_vPortFree(this->msg_array);

    kal_vPortFree(this);
}


static void task_phone_call_create_task_loop(struct AudioTask *this)
{
    /* Note: you can also bypass this function,
             and do kal_xTaskCreate until you really need it.
             Ex: create task after you do get the enable message. */

    BaseType_t xReturn = pdFAIL;

    xReturn = kal_xTaskCreate(
                  task_phone_call_task_loop,
                  LOCAL_TASK_NAME,
                  LOCAL_TASK_STACK_SIZE,
                  (void *)this,
                  LOCAL_TASK_PRIORITY,
                  NULL);

    //kal_xTaskCreate(vTestTaskPlatform, "Test", 2048, (void *)this, 1, NULL);

    AUD_ASSERT(xReturn == pdPASS);
}


static audio_status_t task_phone_call_recv_message(
    struct AudioTask *this,
    struct ipi_msg_t *ipi_msg)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    AUD_LOG_V("%s()\n", __func__);

    AUD_ASSERT(this->msg_array[this->queue_idx].magic == 0); /* item is clean */
    memcpy(&this->msg_array[this->queue_idx], ipi_msg, sizeof(ipi_msg_t));

    xQueueSendToBackFromISR(
        this->msg_idx_queue,
        &this->queue_idx,
        &xHigherPriorityTaskWoken);

    /* update queue index */
    this->queue_idx++;
    if (this->queue_idx == MAX_MSG_QUEUE_SIZE) { this->queue_idx = 0; }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return NO_ERROR;
}


static void task_phone_call_irq_hanlder(
    struct AudioTask *this,
    uint32_t irq_type)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    ipi_msg_t *ipi_msg = NULL;
    uint16_t modem_data_handshake = 0;

    if (irq_type == MD_IRQn) {
        AUD_LOG_V("+%x\n", drv_reg16(DSP_OD_MASK));

        /* record interrupt count */
        irq_idx++;

        /* prevent duplicated irq due to level trigger */
        //mask_irq(MD_IRQn);
        if (irq_flag != 0) {
            AUD_LOG_V("irq #%u, bypass duplicated irq\n", irq_idx);
            AUD_LOG_V("-%x\n", drv_reg16(DSP_OD_MASK));
            irq_cnt_bypass_level_trigger++;
            goto IRQ_EXIT;
        }
        irq_flag = 1;


        /* get modem_data_handshake */ // TODO: modem side buf, need to fix it
        modem_data_handshake = get_modem_data_handshake();
        if (modem_data_handshake != MODEM_UL_DATA &&
            modem_data_handshake != MODEM_DL_DATA) {
            //AUD_LOG_W("handshake(0x%x) error!! return\n", modem_data_handshake);
            memset(shared_tcm_buf_dl, 0, DL_OUT_BUF_SIZE);
            notify_modem_data_arrival(this, shared_tcm_buf_dl, BYPASS_DATA);
            irq_cnt_bypass_handshake_error++;
            goto IRQ_EXIT;
        }


#ifdef RESPONSE_IRQ_TO_MODEM_IMMEDIATELY
        /* Do nothing, just return mute buffer */
        memset(shared_tcm_buf_dl, 0, DL_OUT_BUF_SIZE);
        notify_modem_data_arrival(this, shared_tcm_buf_dl, BYPASS_DATA);
        goto IRQ_EXIT;
#endif


        /* AP side speech off or lib updating state, just return mute buffer */
        if (this->state != AUDIO_TASK_WORKING/*|| flag_lib_update_state == 1*/) {
            memset(shared_tcm_buf_dl, 0, DL_OUT_BUF_SIZE);
            notify_modem_data_arrival(this, shared_tcm_buf_dl, BYPASS_DATA);
            irq_cnt_bypass_speech_off++;
            goto IRQ_EXIT;
        }


        /* check duplicated irq due to handover */
        if (modem_data_handshake == old_modem_data_handshake) {
            /* Do nothing... still need to process this frame */
            AUD_LOG_V("DSP_OD_MASK(0x%x) = 0x%x\n",
                      DSP_OD_MASK, modem_data_handshake);
        }
        old_modem_data_handshake = modem_data_handshake;


        /* put DL/UL data into queue to process */
        ipi_msg = &this->msg_array[this->queue_idx];

        AUD_ASSERT(ipi_msg->magic == 0); /* item is clean */
        ipi_msg->magic      = IPI_MSG_MAGIC_NUMBER;
        ipi_msg->task_scene = TASK_SCENE_PHONE_CALL;
        ipi_msg->msg_layer  = AUDIO_IPI_LAYER_MODEM;
        ipi_msg->data_type  = AUDIO_IPI_MSG_ONLY;
        ipi_msg->ack_type   = AUDIO_IPI_MSG_BYPASS_ACK;
        ipi_msg->msg_id     = IPI_MSG_M2D_CALL_DATA_READY;
        ipi_msg->param1     = modem_data_handshake;
        ipi_msg->param2     = 0;

        xQueueSendToBackFromISR(
            this->msg_idx_queue,
            &this->queue_idx,
            &xHigherPriorityTaskWoken);

        check_irq_timeout(__LINE__, modem_data_handshake);

        this->queue_idx++;
        if (this->queue_idx == MAX_MSG_QUEUE_SIZE) { this->queue_idx = 0; }
    }

IRQ_EXIT:
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void show_md_infra_path_regs()
{
    drv_write_reg32(0xB6070608, 0x5566ABBA);
    AUD_LOG_D("*%p = 0x%x\n", 0xB03B0000, drv_reg32(0xB03B0000));
    AUD_LOG_D("*%p = 0x%x\n", 0xB00D0A24, drv_reg32(0xB00D0A24));
    AUD_LOG_D("*%p = 0x%x\n", 0xB6070608, drv_reg32(0xB6070608));
}


static void write_sherif_reg(volatile unsigned short *addr,
                             unsigned short value)
{
    uint32_t sherif_start_tick = 0;
    uint32_t sherif_stop_tick  = 0;
    uint32_t sherif_cycle_count = 0;
    int in_isr = 0;

    if (is_in_isr()) {
        in_isr = 1;
    }
    else {
        kal_taskENTER_CRITICAL();
        in_isr = 0;
    }

    sherif_start_tick = GET_CURRENT_TICK();
    drv_write_reg16(addr, value);
    sherif_stop_tick = GET_CURRENT_TICK();
    sherif_cycle_count = GET_TICK_INTERVAL(sherif_stop_tick, sherif_start_tick);
    if (sherif_cycle_count >= TINYSYS_TICKS_PER_MS) {
        AUD_LOG_W("write sherif: addr = %p, value = 0x%x, start %u, stop %u, time = %u usec\n",
                  addr, value, sherif_start_tick, sherif_stop_tick,
                  TICK_TO_US(sherif_cycle_count));
    }
    AUD_ASSERT(sherif_cycle_count < TINYSYS_TICKS_PER_MS);

#if 0
    uint16_t read_value = 0;
    uint32_t fail_cnt = 0;

    while (fail_cnt < 3) {
        sherif_start_tick = GET_CURRENT_TICK();
        read_value = drv_reg16(addr);
        sherif_stop_tick = GET_CURRENT_TICK();
        sherif_cycle_count = GET_TICK_INTERVAL(sherif_stop_tick, sherif_start_tick);
        if (sherif_cycle_count >= TINYSYS_TICKS_PER_MS) {
            AUD_LOG_W("read sherif: addr = %p, value = 0x%x, read_value = 0x%x, start %u, stop %u, time = %u usec\n",
                      addr, value, read_value, sherif_start_tick, sherif_stop_tick,
                      TICK_TO_US(sherif_cycle_count));
        }
        AUD_ASSERT(sherif_cycle_count < TINYSYS_TICKS_PER_MS);

        if (read_value == value) {
            break;
        }
        else {
            if (fail_cnt == 0) {
                show_md_infra_path_regs();
            }
            fail_cnt++;
            if (read_value != 0) { // only print non-zero wrong read value
                AUD_LOG_W("read sherif: addr = %p, value = 0x%x, read_value = 0x%x\n",
                          addr, value, read_value);
            }
        }
    }
    if (fail_cnt > 0) {
        AUD_LOG_W("read sherif: addr = %p, value = 0x%x, read_value = 0x%x, fail_cnt = %u\n",
                  addr, value, read_value, fail_cnt);
    }
#endif
    if (in_isr == 0) {
        kal_taskEXIT_CRITICAL();
    }
}


static uint16_t get_modem_data_handshake()
{
    uint16_t modem_data_handshake = 0;

    irq_start_tick = GET_CURRENT_TICK();

    /* notify modem side to clear interrupt */ // TODO: MD1 only
    AUD_LOG_V("+DSP_SPH_OD_INT_CTRL2(0x%x) = 0x%x\n",
              DSP_SPH_OD_INT_CTRL2, drv_reg16(DSP_SPH_OD_INT_CTRL2));
    write_sherif_reg(DSP_SPH_OD_INT_CTRL2, 0);
    AUD_LOG_V("-DSP_SPH_OD_INT_CTRL2(0x%x) = 0x%x\n",
              DSP_SPH_OD_INT_CTRL2, drv_reg16(DSP_SPH_OD_INT_CTRL2));

    /* get handshake to decide it is UL or DL data */
    modem_data_handshake = drv_reg16(DSP_OD_MASK);
    AUD_LOG_V("DSP_OD_MASK(0x%x) = 0x%x\n", DSP_OD_MASK, modem_data_handshake);

    return modem_data_handshake;
}


static void notify_modem_data_arrival(
    struct AudioTask *this, uint16_t *buf, uint16_t modem_data_handshake)
{
    AUD_LOG_V("scp_log_buf = %p\n", buf);

#if 0 // 0x0, 0x1, 0x2, ...
    int i;
    for (i = 0; i < FRAME_BUF_SIZE / sizeof(uint16_t); i++) {
        buf[i] = i;
    }
#endif

#if 0 // dump
    AUD_LOG_D("buf[0] = 0x%x, buf[1] = 0x%x, buf[318] = 0x%x, buf[319] = 0x%x\n",
              buf[0], buf[1], buf[318], buf[319]);
#endif


    /* only do once when speech on ?? */
    write_sherif_reg(DSP_SPH_OD_TCM_ADDR_H, (uint32_t)buf >> 16);
    write_sherif_reg(DSP_SPH_OD_TCM_ADDR_L, (uint32_t)buf & 0xFFFF);
    AUD_LOG_V("DSP_SPH_OD_TCM_ADDR_H(0x%x) = 0x%x, DSP_SPH_OD_TCM_ADDR_L(0x%x) = 0x%x\n",
              DSP_SPH_OD_TCM_ADDR_H, drv_reg16(DSP_SPH_OD_TCM_ADDR_H),
              DSP_SPH_OD_TCM_ADDR_L, drv_reg16(DSP_SPH_OD_TCM_ADDR_L));

    /* process UL/DL data done */
    irq_flag = 0;
    //unmask_irq(MD_IRQn);
    write_sherif_reg(DSP_OD_MASK, 0);
    AUD_LOG_V("-%x\n", drv_reg16(DSP_OD_MASK));

    check_irq_timeout(__LINE__, modem_data_handshake);
}


static void check_irq_timeout(uint16_t line, uint16_t modem_data_handshake)
{
    uint32_t cycle_count = 0;

    irq_stop_tick = GET_CURRENT_TICK();

    /* check timeout */
    cycle_count = GET_TICK_INTERVAL(irq_stop_tick, irq_start_tick);
    if (modem_data_handshake == MODEM_UL_DATA) {
        if (cycle_count > max_ul_irq_tick) {
            max_ul_irq_tick = cycle_count;
            if (cycle_count >= MAX_UL_IRQ_CYCLE_COUNT) {
                AUD_LOG_W("line: %u, tick: start %u, stop %u\n",
                          line, irq_start_tick, irq_stop_tick);
                AUD_LOG_W("irq#%d, UL irq %u usec, max process %u usec\n", irq_idx,
                          TICK_TO_US(cycle_count),
                          TICK_TO_US(max_ul_process_tick));
                AUD_LOG_W("request freq:%d\n", drv_reg32(EXPECTED_FREQ_REG));
                AUD_LOG_W("current freq:%d\n", drv_reg32(CURRENT_FREQ_REG));
                AUD_ASSERT(cycle_count < MAX_UL_IRQ_CYCLE_COUNT);
            }
        }
    }
    else if (modem_data_handshake == MODEM_DL_DATA) {
        if (cycle_count > max_dl_irq_tick) {
            max_dl_irq_tick = cycle_count;
            if (cycle_count >= MAX_DL_IRQ_CYCLE_COUNT) {
                AUD_LOG_W("line: %u, tick: start %u, stop %u\n",
                          line, irq_start_tick, irq_stop_tick);
                AUD_LOG_W("irq#%d, DL irq %u usec, max process %u usec\n", irq_idx,
                          TICK_TO_US(cycle_count),
                          TICK_TO_US(max_dl_process_tick));
                AUD_LOG_W("request freq:%d\n", drv_reg32(EXPECTED_FREQ_REG));
                AUD_LOG_W("current freq:%d\n", drv_reg32(CURRENT_FREQ_REG));
                AUD_ASSERT(cycle_count < MAX_DL_IRQ_CYCLE_COUNT);
            }
        }
    }
}


static void task_phone_call_task_loop(void *pvParameters)
{
    AudioTask *this = (AudioTask *)pvParameters;
    uint8_t local_queue_idx = 0xFF;

    while (1) {
        xQueueReceive(this->msg_idx_queue, &local_queue_idx, portMAX_DELAY);
        task_phone_call_parsing_message(this, &this->msg_array[local_queue_idx]);
    }
}


void get_ap_dram_data(void *scp_mem_addr, void *ap_dram_phy_addr, uint32_t len)
{
    AUD_LOG_V("ap_dram_phy_addr = %p\n", ap_dram_phy_addr);

    dvfs_enable_DRAM_resource(OPENDSP_MEM_ID);
    dma_transaction(
        (uint32_t)scp_mem_addr,
        ap_to_scp((uint32_t)ap_dram_phy_addr),
        len);
    dvfs_disable_DRAM_resource(OPENDSP_MEM_ID);
}


void send_ap_dram_data(void *ap_dram_phy_addr, void *scp_mem_addr, uint32_t len)
{
    AUD_LOG_V("ap_dram_phy_addr = %p\n", ap_dram_phy_addr);

    dvfs_enable_DRAM_resource(OPENDSP_MEM_ID);
    dma_transaction(
        ap_to_scp((uint32_t)ap_dram_phy_addr),
        (uint32_t)scp_mem_addr,
        len);
    dvfs_disable_DRAM_resource(OPENDSP_MEM_ID);
}


void task_phone_call_parsing_message(struct AudioTask *this, ipi_msg_t *ipi_msg)
{
    AUD_LOG_V("+%x\n", ipi_msg->msg_id);
    static uint8_t set_task_config_flag = 0;

    switch (ipi_msg->msg_id) {
        case IPI_MSG_A2D_UL_GAIN: {
            AUD_LOG_V("UL gain 0x%x, 0x%x\n", ipi_msg->param2, ipi_msg->param1);
            if (arsi_handler != NULL) {
                arsi_set_ul_digital_gain(ipi_msg->param2,
                                         ipi_msg->param1,
                                         arsi_handler);
            }
            break;
        }
        case IPI_MSG_A2D_DL_GAIN: {
            AUD_LOG_V("DL gain 0x%x, 0x%x\n", ipi_msg->param2, ipi_msg->param1);
            if (arsi_handler != NULL) {
                arsi_set_dl_digital_gain(ipi_msg->param2,
                                         ipi_msg->param1,
                                         arsi_handler);
            }
            break;
        }
        case IPI_MSG_A2D_TASK_CFG: {
            flag_lib_update_state = 1;
            get_ap_dram_data(&arsi_task_config, ipi_msg->dma_addr, ipi_msg->param1);
            arsi_task_config.debug_log = myprint; // replace HAL print function
            set_task_config_flag = 1;

            if (rsrv_dram_phy_base == NULL) {
                rsrv_dram_phy_base = ipi_msg->dma_addr + 0x2000;
                rsrv_dram_phy_end  = rsrv_dram_phy_base + RSRV_DRAM_SZ - 0x2000 - 1;
                rsrv_dram_phy_cur  = rsrv_dram_phy_base;
                AUD_LOG_V("ipi_msg->dma_addr = %p\n", ipi_msg->dma_addr);
                AUD_LOG_V("base %p, end %p, cur %p\n",
                          rsrv_dram_phy_base, rsrv_dram_phy_end, rsrv_dram_phy_cur);
            }
            flag_lib_update_state = 0;
            break;
        }
        case IPI_MSG_A2D_SPH_PARAM: {
            flag_lib_update_state = 1;
            get_ap_dram_data(param_buf.p_buffer, ipi_msg->dma_addr, ipi_msg->param1);
            param_buf.data_size = ipi_msg->param1;
            if (arsi_handler != NULL) {
                if (set_task_config_flag == 1) {
                    if (arsi_update_device(&arsi_task_config, &param_buf, arsi_handler) != LIB_OK) {
                        AUD_LOG_W("arsi_update_device fail!!\n", __func__);
                    }
                    set_task_config_flag = 0;
                }
                else {
                    if (arsi_update_param(&arsi_task_config, &param_buf, arsi_handler) != LIB_OK) {
                        AUD_LOG_W("arsi_update_param fail!!\n", __func__);
                    }
                }
            }
            flag_lib_update_state = 0;
            break;
        }
        case IPI_MSG_A2D_SPH_ON: {
            if (ipi_msg->param1 == 1) {
                AUD_LOG_D("speech on\n");
                modem_index = ipi_msg->param2;
                this->state = AUDIO_TASK_INIT;
                task_phone_call_init(this);
                this->state = AUDIO_TASK_WORKING;
                audio_send_ipi_msg_ack_back(ipi_msg);
            }
            else if (ipi_msg->param1 == 0) {
                if (modem_index != 0xFF) {
                    AUD_LOG_D("speech off\n");
                    AUD_ASSERT(modem_index == ipi_msg->param2);
                    this->state = AUDIO_TASK_DEINIT;
                    task_phone_call_deinit(this);
                    this->state = AUDIO_TASK_IDLE;
                }
                modem_index = 0xFF;
                audio_send_ipi_msg_ack_back(ipi_msg);
            }
            break;
        }
        case IPI_MSG_A2D_TTY_ON: {
            if (ipi_msg->param1 == 1) {
                AUD_LOG_V("tty on\n");
                switch (ipi_msg->param2) { // tty_mode_t
                    case AUD_TTY_FULL: {
                        enh_on_tty_ul = 0;
                        enh_on_tty_dl = 0;
                        break;
                    }
                    case AUD_TTY_VCO: {
                        enh_on_tty_ul = 1;
                        enh_on_tty_dl = 0;
                        break;
                    }
                    case AUD_TTY_HCO: {
                        enh_on_tty_ul = 0;
                        enh_on_tty_dl = 1;
                        break;
                    }
                    default: {
                        AUD_LOG_W("TTY_ON: mode %d err\n", ipi_msg->param2);
                        enh_on_tty_ul = 1;
                        enh_on_tty_dl = 1;
                        break;
                    }
                }
            }
            else if (ipi_msg->param1 == 0) {
                AUD_LOG_V("tty off\n");
                switch (ipi_msg->param2) { // tty_mode_t
                    case AUD_TTY_OFF: {
                        enh_on_tty_ul = 1;
                        enh_on_tty_dl = 1;
                        break;
                    }
                    default: {
                        AUD_LOG_W("TTY_OFF: mode %d err\n", ipi_msg->param2);
                        enh_on_tty_ul = 1;
                        enh_on_tty_dl = 1;
                        break;
                    }
                }
            }

            // apply enhancement state
            if (arsi_handler != NULL) {
                arsi_set_ul_enhance(enh_on_ul & enh_on_tty_ul, arsi_handler);
                arsi_set_dl_enhance(enh_on_dl & enh_on_tty_dl, arsi_handler);
            }
            break;
        }
        case IPI_MSG_A2D_UL_MUTE_ON: {
            AUD_LOG_V("UL mute on %d\n", ipi_msg->param1);
            if (arsi_handler != NULL) {
                arsi_set_ul_mute(ipi_msg->param1, arsi_handler);
            }
            break;
        }
        case IPI_MSG_A2D_DL_MUTE_ON: {
            AUD_LOG_V("DL mute on %d\n", ipi_msg->param1);
            if (arsi_handler != NULL) {
                arsi_set_dl_mute(ipi_msg->param1, arsi_handler);
            }
            break;
        }
        case IPI_MSG_A2D_UL_ENHANCE_ON: {
            AUD_LOG_V("UL enh on %d\n", ipi_msg->param1);
            enh_on_ul = ipi_msg->param1;
            if (arsi_handler != NULL) {
                arsi_set_ul_enhance(enh_on_ul & enh_on_tty_ul, arsi_handler);
            }
            break;
        }
        case IPI_MSG_A2D_DL_ENHANCE_ON: {
            AUD_LOG_V("DL enh on %d\n", ipi_msg->param1);
            enh_on_dl = ipi_msg->param1;
            if (arsi_handler != NULL) {
                arsi_set_dl_enhance(enh_on_dl & enh_on_tty_dl, arsi_handler);
            }
            break;
        }
        case IPI_MSG_A2D_BT_NREC_ON: {
            // TODO: BT NR/EC
            break;
        }
        case IPI_MSG_A2D_SET_ADDR_VALUE: {
            AUD_LOG_V("1 addr = 0x%x, value = 0x%x\n", ipi_msg->param1, ipi_msg->param2);
            if (arsi_handler != NULL) {
                lib_status_t retval = arsi_set_addr_value(ipi_msg->param1,
                                                          ipi_msg->param2,
                                                          arsi_handler);
                if (retval != LIB_OK) {
                    AUD_LOG_W("arsi_set_addr_value fail!! retval = %d\n", retval);
                    ipi_msg->param1 = 0;
                }
                else {
                    AUD_LOG_V("arsi_set_addr_value pass!! retval = %d\n", retval);
                    ipi_msg->param1 = 1;
                }
                //retval = arsi_get_addr_value(ipi_msg->param1, &ipi_msg->param2, arsi_handler);
                //AUD_LOG_D("2 addr = 0x%x, value = 0x%x\n", ipi_msg->param1, ipi_msg->param2);
            }
            else {
                ipi_msg->param1 = 0;
            }
            audio_send_ipi_msg_ack_back(ipi_msg);
            break;
        }
        case IPI_MSG_A2D_GET_ADDR_VALUE: {
            AUD_LOG_V("1 addr = 0x%x, value = 0x%x\n", ipi_msg->param1, ipi_msg->param2);
            if (arsi_handler != NULL) {
                lib_status_t retval = arsi_get_addr_value(ipi_msg->param1,
                                                          &ipi_msg->param2,
                                                          arsi_handler);
                AUD_LOG_V("2 addr = 0x%x, value = 0x%x\n", ipi_msg->param1, ipi_msg->param2);

                if (retval != LIB_OK) {
                    AUD_LOG_W("arsi_get_addr_value fail!! retval = %d\n", retval);
                    ipi_msg->param1 = 0;
                }
                else {
                    AUD_LOG_V("arsi_get_addr_value pass!! retval = %d\n", retval);
                    ipi_msg->param1 = 1;
                }
            }
            else {
                ipi_msg->param1 = 0;
            }
            audio_send_ipi_msg_ack_back(ipi_msg);
            break;
        }
        case IPI_MSG_A2D_SET_KEY_VALUE: {
            char key_value[ipi_msg->param2];
            get_ap_dram_data(key_value, ipi_msg->dma_addr, ipi_msg->param1);

            string_buf_t key_value_pair;
            key_value_pair.memory_size = ipi_msg->param2;
            key_value_pair.string_size = strlen(key_value);
            key_value_pair.p_string = key_value;
            AUD_LOG_V("%u, %u, %s\n", key_value_pair.memory_size,
                      key_value_pair.string_size, key_value_pair.p_string);

            if (arsi_handler != NULL) {
                lib_status_t retval = arsi_set_key_value_pair(&key_value_pair,
                                                              arsi_handler);
                if (retval != LIB_OK) {
                    AUD_LOG_W("arsi_set_key_value_pair fail!! retval = %d\n", retval);
                    ipi_msg->param1 = 0;
                }
                else {
                    AUD_LOG_V("arsi_set_key_value_pair pass!! retval = %d\n", retval);
                    ipi_msg->param1 = 1;
                }
            }
            else {
                ipi_msg->param1 = 0;
            }
            audio_send_ipi_msg_ack_back(ipi_msg);
            break;
        }
        case IPI_MSG_A2D_GET_KEY_VALUE: {
            char key_value[ipi_msg->param2];
            get_ap_dram_data(key_value, ipi_msg->dma_addr, ipi_msg->param1);

            string_buf_t key_value_pair;
            key_value_pair.memory_size = ipi_msg->param2;
            key_value_pair.string_size = strlen(key_value);
            key_value_pair.p_string = key_value;
            AUD_LOG_V("%u, %u, %s\n", key_value_pair.memory_size,
                      key_value_pair.string_size, key_value_pair.p_string);

            if (arsi_handler != NULL) {
                lib_status_t retval = arsi_get_key_value_pair(&key_value_pair,
                                                              arsi_handler);
                if (retval != LIB_OK) {
                    AUD_LOG_W("arsi_set_key_value_pair fail!! retval = %d\n", retval);
                    ipi_msg->param1 = 0;
                }
                else {
                    AUD_LOG_V("arsi_set_key_value_pair pass!! retval = %d\n", retval);
                    ipi_msg->param1 = 1;
                    AUD_LOG_V("%s\n", key_value_pair.p_string);

                    AUD_ASSERT(ipi_msg->param2 > strlen(key_value));
                    AUD_ASSERT(strstr(key_value, "=") != NULL);

                    send_ap_dram_data(ipi_msg->dma_addr,
                                      key_value,
                                      strlen(key_value) + 1);
                }
            }
            else {
                ipi_msg->param1 = 0;
            }
            audio_send_ipi_msg_ack_back(ipi_msg);
            break;
        }
        case IPI_MSG_A2D_PCM_DUMP_ON: {
            dump_pcm_flag = ipi_msg->param1;
            AUD_LOG_D("dump_pcm_flag: %d\n", dump_pcm_flag);
            break;
        }
        case IPI_MSG_A2D_LIB_LOG_ON: {
            lib_log_flag = ipi_msg->param1;
            AUD_LOG_D("lib_log_flag: %d\n", lib_log_flag);
            break;
        }
        case IPI_MSG_M2D_CALL_DATA_READY: {
            AUD_LOG_V("IPI_MSG_M2D_CALL_DATA_READY\n");
            if (this->state == AUDIO_TASK_WORKING) {
                task_phone_call_working(this, ipi_msg);
            }
            else {
                AUD_LOG_V("bypass data, state = %d\n", this->state);
                memset(shared_tcm_buf_dl, 0, DL_OUT_BUF_SIZE);
                notify_modem_data_arrival(this, shared_tcm_buf_dl, BYPASS_DATA);
            }
            break;
        }
        default: {
            AUD_LOG_W("%s(), unknown msg_id 0x%x, drop!!\n",
                      __func__, ipi_msg->msg_id);
            break;
        }
    }


#if 0 /* TODO: queue */
    /* send ack back if need */
    audio_send_ipi_msg_ack_back(ipi_msg);
#endif

    AUD_LOG_V("(-)%x\n", ipi_msg->msg_id);

    /* clean msg */
    memset(ipi_msg, 0, sizeof(ipi_msg_t));
}


void dump_cfg()
{
    AUD_LOG_D("%s(), arsi_task_config.task_scene = %u\n",
              __func__, arsi_task_config.task_scene);
    AUD_LOG_D("%s(), arsi_task_config.frame_size_ms = %u\n",
              __func__, arsi_task_config.frame_size_ms);

    AUD_LOG_D("%s(), arsi_task_config.stream_uplink.device = %u\n",
              __func__, arsi_task_config.stream_uplink.device);

    AUD_LOG_D("%s(), arsi_task_config.stream_uplink.sample_rate_in = %lu\n",
              __func__, arsi_task_config.stream_uplink.sample_rate_in);
    AUD_LOG_D("%s(), arsi_task_config.stream_uplink.sample_rate_out = %lu\n",
              __func__, arsi_task_config.stream_uplink.sample_rate_out);

    AUD_LOG_D("%s(), arsi_task_config.stream_uplink.bit_format_in = %d\n",
              __func__, arsi_task_config.stream_uplink.bit_format_in);
    AUD_LOG_D("%s(), arsi_task_config.stream_uplink.bit_format_out = %d\n",
              __func__, arsi_task_config.stream_uplink.bit_format_out);

    AUD_LOG_D("%s(), arsi_task_config.stream_uplink.num_channels_in = %d\n",
              __func__, arsi_task_config.stream_uplink.num_channels_in);
    AUD_LOG_D("%s(), arsi_task_config.stream_uplink.num_channels_out = %d\n",
              __func__, arsi_task_config.stream_uplink.num_channels_out);


    AUD_LOG_D("%s(), arsi_task_config.stream_downlink.device = %d\n",
              __func__, arsi_task_config.stream_downlink.device);

    AUD_LOG_D("%s(), arsi_task_config.stream_downlink.sample_rate_in = %lu\n",
              __func__, arsi_task_config.stream_downlink.sample_rate_in);
    AUD_LOG_D("%s(), arsi_task_config.stream_downlink.sample_rate_out = %lu\n",
              __func__, arsi_task_config.stream_downlink.sample_rate_out);

    AUD_LOG_D("%s(), arsi_task_config.stream_downlink.bit_format_in = %d\n",
              __func__, arsi_task_config.stream_downlink.bit_format_in);
    AUD_LOG_D("%s(), arsi_task_config.stream_downlink.bit_format_out = %d\n",
              __func__, arsi_task_config.stream_downlink.bit_format_out);

    AUD_LOG_D("%s(), arsi_task_config.stream_downlink.num_channels_in = %d\n",
              __func__, arsi_task_config.stream_downlink.num_channels_in);
    AUD_LOG_D("%s(), arsi_task_config.stream_downlink.num_channels_out = %d\n",
              __func__, arsi_task_config.stream_downlink.num_channels_out);
}

extern void disable_task_monitor_log(void);
extern void enable_task_monitor_log(void);
static audio_status_t task_phone_call_init(struct AudioTask *this)
{
    AUD_LOG_D("Init, modem_index = %d, DSP_OD_MASK = 0x%x\n",
              modem_index, DSP_OD_MASK);

    /* enable system related API */
    CPU_RESET_CYCLECOUNTER();

    scp_ipi_wakeup_ap_registration(IPI_AUDIO);

    disable_task_monitor_log();

    set_max_cs_limit(1500000);

    register_feature(OPEN_DSP_FEATURE_ID);
    AUD_LOG_V("get_freq_setting() = %u\n", get_freq_setting());

    show_md_infra_path_regs();


    /* init global var */
#ifdef SWAP_DL_UL_LOOPBACK
    ul_tmp_buf = (uint16_t *)kal_pvPortMalloc(FRAME_BUF_SIZE);
    AUD_LOG_D("ul_tmp_buf = %p\n", ul_tmp_buf);
    dl_tmp_buf = (uint16_t *)kal_pvPortMalloc(FRAME_BUF_SIZE);
    AUD_LOG_D("dl_tmp_buf = %p\n", dl_tmp_buf);
#endif

    irq_idx = 0;
    irq_cnt_ul = 0;
    irq_cnt_dl = 0;
    irq_cnt_bypass_handshake_error = 0;
    irq_cnt_bypass_level_trigger = 0;
    irq_cnt_bypass_speech_off = 0;


    irq_flag = 0;
    old_modem_data_handshake = 0xFF;

    flag_lib_update_state = 0;

    extra_call_arg.call_band_type = PHONE_CALL_BAND_16K; // depends on network
    extra_call_arg.call_net_type = PHONE_CALL_NET_3G;    // depends on network

    enh_on_ul = 1;
    enh_on_dl = 1;
    enh_on_tty_ul = 1;
    enh_on_tty_dl = 1;

    /* dump current heap size before malloc */
    AUD_LOG_D("+Heap free/total = %d/%d\n",
              xPortGetFreeHeapSize(), configTOTAL_HEAP_SIZE);

    arsi_query_working_buf_size(&arsi_task_config, &working_buf.memory_size);
    AUD_LOG_D("working_buf.memory_size = %u\n", working_buf.memory_size);

    working_buf.p_buffer = kal_pvPortMalloc(working_buf.memory_size);
    AUD_LOG_D("working_buf.p_buffer = %p\n", working_buf.p_buffer);


    ul_buf_in.pcm_data.memory_size = UL_IN_BUF_SIZE;
    ul_buf_in.pcm_data.p_buffer = kal_pvPortMalloc(ul_buf_in.pcm_data.memory_size);
    AUD_LOG_D("ul_buf_in.pcm_data.p_buffer = %p\n",  ul_buf_in.pcm_data.p_buffer);

    ul_buf_out.pcm_data.memory_size = UL_OUT_BUF_SIZE;
    ul_buf_out.pcm_data.p_buffer = (void *)shared_tcm_buf_ul; // use share buf
    AUD_LOG_D("ul_buf_out.pcm_data.p_buffer = %p\n", ul_buf_out.pcm_data.p_buffer);

    dl_buf_in.pcm_data.memory_size = DL_IN_BUF_SIZE;
    dl_buf_in.pcm_data.p_buffer = kal_pvPortMalloc(dl_buf_in.pcm_data.memory_size);
    AUD_LOG_D("dl_buf_in.pcm_data.p_buffer = %p\n",  dl_buf_in.pcm_data.p_buffer);

    dl_buf_out.pcm_data.memory_size = DL_OUT_BUF_SIZE;
    dl_buf_out.pcm_data.p_buffer = (void *)shared_tcm_buf_dl; // use share buf
    AUD_LOG_D("dl_buf_out.pcm_data.p_buffer = %p\n", dl_buf_out.pcm_data.p_buffer);

    aec_buf_in.pcm_data.memory_size = AEC_BUF_SIZE;
    aec_buf_in.pcm_data.p_buffer = kal_pvPortMalloc(
                                       aec_buf_in.pcm_data.memory_size);
    AUD_LOG_D("aec_buf_in.pcm_data.p_buffer = %p\n", aec_buf_in.pcm_data.p_buffer);


    arsi_create_handler(&arsi_task_config, &param_buf, &working_buf, &arsi_handler);

    arsi_set_debug_log_fp(myprint, arsi_handler);

    arsi_set_ul_digital_gain(0, 0, arsi_handler);
    arsi_set_dl_digital_gain(0, 0, arsi_handler);
    arsi_set_ul_mute(0, arsi_handler); // 0: false, 1: true
    arsi_set_dl_mute(0, arsi_handler); // 0: false, 1: true
    arsi_set_ul_enhance(enh_on_ul & enh_on_tty_ul, arsi_handler);
    arsi_set_dl_enhance(enh_on_dl & enh_on_tty_dl, arsi_handler);

    AUD_LOG_V("%s(-)\n", __func__);
    return NO_ERROR;
}


static void *get_resv_dram_buf(const uint32_t len)
{
    // TODO: ring buf
    void *retval = rsrv_dram_phy_cur;

    if (retval + len < rsrv_dram_phy_end) {
        rsrv_dram_phy_cur += len;
    }
    else {
        retval = rsrv_dram_phy_base;
        rsrv_dram_phy_cur = rsrv_dram_phy_base + len;
    }

    return retval;
}

static audio_status_t task_phone_call_working(
    struct AudioTask *this,
    ipi_msg_t *ipi_msg)
{
    AUD_LOG_V("%s()\n", __func__);

    volatile uint16_t *dm_addr = NULL;
    const uint32_t aec_delay_ms = 40; // TODO: get from modem side
    uint16_t modem_data_handshake = ipi_msg->param1;
    uint32_t start_tick = 0;
    uint32_t stop_tick  = 0;
    uint32_t cycle_count = 0;

    pcm_dump_ul_t *pcm_dump_ul = NULL;
    pcm_dump_dl_t *pcm_dump_dl = NULL;


    check_irq_timeout(__LINE__, modem_data_handshake);


    /* processing */
    if (modem_data_handshake == MODEM_UL_DATA) {
        irq_cnt_ul++;

        /* UL ch1 */
        dm_addr = DSP_UL1_ADDR;
        AUD_LOG_V("UL1, dm_addr = %p\n", dm_addr);
        dma_transaction_16bit(
            (uint32_t)ul_buf_in.pcm_data.p_buffer,
            (uint32_t)dm_addr,
            FRAME_BUF_SIZE);
        ul_buf_in.pcm_data.data_size = FRAME_BUF_SIZE;

        /* UL ch2 */
        if (arsi_task_config.stream_uplink.num_channels_in == 1) {
            memset((void *)(ul_buf_in.pcm_data.p_buffer + FRAME_BUF_SIZE),
                   0,
                   FRAME_BUF_SIZE);
        }
        else {
            dm_addr = DSP_UL2_ADDR;
            AUD_LOG_V("UL2, dm_addr = %p\n", dm_addr);
            dma_transaction_16bit(
                (uint32_t)(ul_buf_in.pcm_data.p_buffer + FRAME_BUF_SIZE),
                (uint32_t)dm_addr,
                FRAME_BUF_SIZE);
            ul_buf_in.pcm_data.data_size += FRAME_BUF_SIZE;
        }

        /* Echo Ref */
        dm_addr = DSP_ECHO_REF_ADDR;
        AUD_LOG_V("AEC, dm_addr = %p\n", dm_addr);
        dma_transaction_16bit(
            (uint32_t)aec_buf_in.pcm_data.p_buffer,
            (uint32_t)dm_addr,
            FRAME_BUF_SIZE);
        aec_buf_in.pcm_data.data_size = AEC_BUF_SIZE;

        check_irq_timeout(__LINE__, modem_data_handshake);


        if (dump_pcm_flag) {
            pcm_dump_ul = (pcm_dump_ul_t *)get_resv_dram_buf(sizeof(pcm_dump_ul_t));
            AUD_LOG_V("UL %p\n", pcm_dump_ul);

            send_ap_dram_data(
                pcm_dump_ul->ul_in_ch1, ul_buf_in.pcm_data.p_buffer, FRAME_BUF_SIZE);
            send_ap_dram_data(
                pcm_dump_ul->ul_in_ch2, ul_buf_in.pcm_data.p_buffer + FRAME_BUF_SIZE,
                FRAME_BUF_SIZE);
            send_ap_dram_data(
                pcm_dump_ul->aec_in, aec_buf_in.pcm_data.p_buffer, FRAME_BUF_SIZE);

            check_irq_timeout(__LINE__, modem_data_handshake);
        }


        /* processing */
        start_tick = GET_CURRENT_TICK();
        arsi_process_ul_buf(
            &ul_buf_in,
            &ul_buf_out,
            &aec_buf_in,
            aec_delay_ms,
            arsi_handler,
            (void *)&extra_call_arg);
        stop_tick = GET_CURRENT_TICK();
        cycle_count = GET_TICK_INTERVAL(stop_tick, start_tick);
        if (cycle_count >= MAX_UL_PROCESS_CYCLE_COUNT) {
            AUD_LOG_W("tick: start %u, stop %u, cycle = %u, max = %u\n",
                      start_tick, stop_tick,
                      cycle_count, MAX_UL_PROCESS_CYCLE_COUNT);
        }
        AUD_ASSERT(cycle_count < MAX_UL_PROCESS_CYCLE_COUNT);
        if (cycle_count > max_ul_process_tick) {
            max_ul_process_tick = cycle_count;
        }
        check_irq_timeout(__LINE__, modem_data_handshake);


#if 0 // TODO: check
        AUD_ASSERT(ul_buf_in.pcm_data.data_size == 0);
        AUD_ASSERT(aec_buf_in.pcm_data.data_size == 0);
        AUD_ASSERT(ul_buf_out.pcm_data.data_size == FRAME_BUF_SIZE);
#endif

#ifdef SWAP_DL_UL_LOOPBACK
        memcpy(ul_tmp_buf, ul_buf_out.pcm_data.p_buffer, FRAME_BUF_SIZE);
        memcpy(ul_buf_out.pcm_data.p_buffer, dl_tmp_buf, FRAME_BUF_SIZE);
#endif

        if (dump_pcm_flag) {
            send_ap_dram_data(
                pcm_dump_ul->ul_out, ul_buf_out.pcm_data.p_buffer, FRAME_BUF_SIZE);
            check_irq_timeout(__LINE__, modem_data_handshake);
        }

        notify_modem_data_arrival(this, ul_buf_out.pcm_data.p_buffer, MODEM_UL_DATA);
        ul_buf_out.pcm_data.data_size = 0;

        if (dump_pcm_flag) {
            start_tick = GET_CURRENT_TICK();
            audio_send_ipi_msg(this->scene,
                               AUDIO_IPI_DMA, AUDIO_IPI_MSG_BYPASS_ACK,
                               IPI_MSG_D2A_PCM_DUMP_DATA_NOTIFY, sizeof(pcm_dump_ul_t), 0,
                               (char *)pcm_dump_ul);
            stop_tick = GET_CURRENT_TICK();
            cycle_count = GET_TICK_INTERVAL(stop_tick, start_tick);
            if (cycle_count >= TINYSYS_TICKS_PER_MS) {
                AUD_LOG_W("3 tick: start %u, stop %u, time = %u usec\n",
                          start_tick, stop_tick, TICK_TO_US(cycle_count));
            }
        }
    }
    else if (modem_data_handshake == MODEM_DL_DATA) {
        irq_cnt_dl++;

        /* DL ch 1 */
        dm_addr = DSP_DL_ADDR;
        AUD_LOG_V("DL, dm_addr = %p\n", dm_addr);
        dma_transaction_16bit(
            (uint32_t)dl_buf_in.pcm_data.p_buffer,
            (uint32_t)dm_addr,
            FRAME_BUF_SIZE);
        dl_buf_in.pcm_data.data_size = DL_IN_BUF_SIZE;
        check_irq_timeout(__LINE__, modem_data_handshake);

        if (dump_pcm_flag) {
            pcm_dump_dl = (pcm_dump_dl_t *)get_resv_dram_buf(sizeof(pcm_dump_dl_t));
            AUD_LOG_V("DL %p\n", pcm_dump_dl);

            send_ap_dram_data(
                pcm_dump_dl->dl_in, dl_buf_in.pcm_data.p_buffer, FRAME_BUF_SIZE);
            check_irq_timeout(__LINE__, modem_data_handshake);
        }


        /* processing */
        start_tick = GET_CURRENT_TICK();
        arsi_process_dl_buf(
            &dl_buf_in,
            &dl_buf_out,
            arsi_handler,
            (void *)&extra_call_arg);
        stop_tick = GET_CURRENT_TICK();
        cycle_count = GET_TICK_INTERVAL(stop_tick, start_tick);
        if (cycle_count >= MAX_DL_PROCESS_CYCLE_COUNT) {
            AUD_LOG_W("tick: start %u, stop %u, cycle = %u, max = %u\n",
                      start_tick, stop_tick,
                      cycle_count, MAX_DL_PROCESS_CYCLE_COUNT);
        }
        AUD_ASSERT(cycle_count < MAX_DL_PROCESS_CYCLE_COUNT);
        if (cycle_count > max_dl_process_tick) {
            max_dl_process_tick = cycle_count;
        }
        check_irq_timeout(__LINE__, modem_data_handshake);

#if 0 // TODO: check
        AUD_ASSERT(dl_buf_in.pcm_data.data_size == 0);
        AUD_ASSERT(dl_buf_out.pcm_data.data_size == FRAME_BUF_SIZE);
#endif


#ifdef SWAP_DL_UL_LOOPBACK
        memcpy(dl_tmp_buf, dl_buf_out.pcm_data.p_buffer, FRAME_BUF_SIZE);
        memcpy(dl_buf_out.pcm_data.p_buffer, ul_tmp_buf, FRAME_BUF_SIZE);
#endif

        if (dump_pcm_flag) {
            send_ap_dram_data(
                pcm_dump_dl->dl_out, dl_buf_out.pcm_data.p_buffer, FRAME_BUF_SIZE);
            check_irq_timeout(__LINE__, modem_data_handshake);
        }

        notify_modem_data_arrival(this, dl_buf_out.pcm_data.p_buffer, MODEM_DL_DATA);
        dl_buf_out.pcm_data.data_size = 0;

        if (dump_pcm_flag) {
            start_tick = GET_CURRENT_TICK();
            audio_send_ipi_msg(this->scene,
                               AUDIO_IPI_DMA, AUDIO_IPI_MSG_BYPASS_ACK,
                               IPI_MSG_D2A_PCM_DUMP_DATA_NOTIFY, sizeof(pcm_dump_dl_t), 1,
                               (char *)pcm_dump_dl);
            stop_tick = GET_CURRENT_TICK();
            cycle_count = GET_TICK_INTERVAL(stop_tick, start_tick);
            if (cycle_count >= TINYSYS_TICKS_PER_MS) {
                AUD_LOG_W("6 tick: start %u, stop %u, time = %u usec\n",
                          start_tick, stop_tick, TICK_TO_US(cycle_count));
            }
        }
    }
    else {
        AUD_LOG_W("modem_data_handshake(0x%x) bad value!!", modem_data_handshake);
        AUD_ASSERT(modem_data_handshake == MODEM_UL_DATA ||
                   modem_data_handshake == MODEM_DL_DATA);
        memset(shared_tcm_buf_dl, 0, DL_OUT_BUF_SIZE);
        notify_modem_data_arrival(this, shared_tcm_buf_dl, BYPASS_DATA);
    }

    return NO_ERROR;
}


static audio_status_t task_phone_call_deinit(struct AudioTask *this)
{
    AUD_LOG_D("%s()\n", __func__);

    arsi_destroy_handler(arsi_handler);
    arsi_handler = NULL;

    kal_vPortFree(working_buf.p_buffer);
    working_buf.memory_size = 0;
    working_buf.p_buffer = NULL;

    kal_vPortFree(ul_buf_in.pcm_data.p_buffer);
    ul_buf_in.pcm_data.memory_size = 0;
    ul_buf_in.pcm_data.p_buffer = NULL;

    //kal_vPortFree(ul_buf_out.pcm_data.p_buffer); // due to share buf
    ul_buf_out.pcm_data.memory_size = 0;
    ul_buf_out.pcm_data.p_buffer = NULL;

    kal_vPortFree(dl_buf_in.pcm_data.p_buffer);
    dl_buf_in.pcm_data.memory_size = 0;
    dl_buf_in.pcm_data.p_buffer = NULL;

    //kal_vPortFree(dl_buf_out.pcm_data.p_buffer); // due to share buf
    dl_buf_out.pcm_data.memory_size = 0;
    dl_buf_out.pcm_data.p_buffer = NULL;

    kal_vPortFree(aec_buf_in.pcm_data.p_buffer);
    aec_buf_in.pcm_data.memory_size = 0;
    aec_buf_in.pcm_data.p_buffer = NULL;

#ifdef SWAP_DL_UL_LOOPBACK
    kal_vPortFree(ul_tmp_buf);
    ul_tmp_buf = NULL;

    kal_vPortFree(dl_tmp_buf);
    dl_tmp_buf = NULL;
#endif

    deregister_feature(OPEN_DSP_FEATURE_ID);
    disable_cs_limit();
    enable_task_monitor_log();

#if 1
    AUD_LOG_D("max DL process time = %u usec, max UL process time = %u usec\n",
              TICK_TO_US(max_dl_process_tick), TICK_TO_US(max_ul_process_tick));
    AUD_LOG_D("max DL irq time = %u usec, max UL irq time = %u usec\n",
              TICK_TO_US(max_dl_irq_tick), TICK_TO_US(max_ul_irq_tick));
#endif

#if 1
    AUD_LOG_D("irq_idx = %u, sum = %u\n", irq_idx,
              (irq_cnt_ul + irq_cnt_dl + irq_cnt_bypass_handshake_error +
               irq_cnt_bypass_level_trigger + irq_cnt_bypass_speech_off));

    AUD_LOG_D("irq_cnt_ul = %u\n", irq_cnt_ul);
    AUD_LOG_D("irq_cnt_dl = %u\n", irq_cnt_dl);
    AUD_LOG_D("irq_cnt_bypass_handshake_error = %u\n",
              irq_cnt_bypass_handshake_error);
    AUD_LOG_D("irq_cnt_bypass_level_trigger = %u\n", irq_cnt_bypass_level_trigger);
    AUD_LOG_D("irq_cnt_bypass_speech_off = %u\n", irq_cnt_bypass_speech_off);
#endif


    /* dump current heap size after malloc */
    AUD_LOG_D("-Heap free/total = %d/%d\n",
              xPortGetFreeHeapSize(), configTOTAL_HEAP_SIZE);

    return NO_ERROR;
}


static void myprint(const char *message, ...)
{
    if (lib_log_flag) {
        static char printf_msg[256];

        va_list args;
        va_start(args, message);

        vsnprintf(printf_msg, sizeof(printf_msg), message, args);
        AUD_LOG_D("%s", printf_msg);

        va_end(args);
    }
}



