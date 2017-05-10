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
#include "audio_task_offload_mp3.h"

#include <stdarg.h>

#include <interrupt.h>
#include <dma.h>
#include <vcore_dvfs.h>
#include <feature_manager.h>
#include <scp_ipi.h>

#include "audio_messenger_ipi.h"
#include "audio_task_offload_mp3_params.h"
#include "arsi_api.h"

#include "RingBuf.h"



/*==============================================================================
 *                     MACRO
 *============================================================================*/

#define LOCAL_TASK_STACK_SIZE (512)
#define LOCAL_TASK_NAME "mp3"
#define LOCAL_TASK_PRIORITY (3)

#define MAX_MSG_QUEUE_SIZE (8)
#define IRQ7_FLAG 0x40
/* TODO: calculate it */
#define USE_PERIODS_MAX     8192
#define BUFFER_OFFSET       16

#define POST_PROCESS
#define AudioDecoder
#define BliSrc
//#define CYCLE
/*==============================================================================
 *                     private global members
 *============================================================================*/

/* TODO: put to derived class */
static DMA_BUFFER_T bs_buf;
static DMA_BUFFER_T pcm_buf;
static SCP_OFFLOAD_T afe_offload_block;
static void *pRemainedPCM_Buf;
static int *pBitConvertBuf;

//Ringbuf : DRAM, SRAM
static RingBuf rMemDL;
static RingBuf rMemDRAM;

#ifdef AudioDecoder
static mp3DecEngine *mMp3Dec = NULL;
static uint8_t mMp3InitFlag = false;
#endif
#ifdef BliSrc
static blisrcEngine *mBliSrc = NULL;
static uint8_t mSrcInitFlag = false;
void *pSrcOutBuf;
static uint32_t decoder_sameplerateidx[9] = {44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000};
#endif
unsigned char bRemainedPcm = false;
unsigned char bDramPlayback = 0;
unsigned char AFEenable = false;
unsigned char bfirsttime = true;

uint32_t RemainedPcmlen = 0;

#ifdef AudioDecoder
static int dma_ch = 2;
#endif
#ifdef CYCLE
volatile int *DWT_CONTROL = (int *)0xE0001000;
volatile int *DWT_CYCCNT = (int *)0xE0001004;
volatile int *DEMCR = (int *)0xE000EDFC;

#define CPU_RESET_CYCLECOUNTER    do { *DEMCR = *DEMCR | 0x01000000;  \
        *DWT_CYCCNT = 0;              \
        *DWT_CONTROL = *DWT_CONTROL | 1 ; } while(0)
#endif
/*==============================================================================
 *                     derived functions - declaration
 *============================================================================*/

static void           task_offload_mp3_constructor(struct AudioTask *this);
static void           task_offload_mp3_destructor(struct AudioTask *this);

static void           task_offload_mp3_create_task_loop(struct AudioTask *this);

static audio_status_t task_offload_mp3_recv_message(
    struct AudioTask *this,
    struct ipi_msg_t *ipi_msg);

static void           task_offload_mp3_irq_hanlder(
    struct AudioTask *this,
    uint32_t irq_type);


/*==============================================================================
 *                     private functions - declaration
 *============================================================================*/


static uint8_t task_offload_mp3_preparsing_message(struct AudioTask *this,
        ipi_msg_t *ipi_msg);

static void           task_offload_mp3_task_loop(void *pvParameters);

static audio_status_t task_offload_mp3_init(struct AudioTask *this);
static audio_status_t task_offload_mp3_working(struct AudioTask *this);
static audio_status_t task_offload_mp3_deinit(struct AudioTask *this);

static int mp3_init_decoder(void);
static int mp3_deinit_blisrc(void);
static void mp3_deinit_decoder(void);

static int mp3_init_blisrc(int32_t inSampleRate, int32_t inChannelCount,
                           int32_t outSampleRate, int32_t outChannelCount);

static int mp3_blisrc_process(void *pInBuffer, unsigned int inBytes,
                              void **ppOutBuffer, unsigned int *pOutBytes);
static int mp3_bitconvert_process(void *pInBuffer, unsigned int inBytes,
                                  void **ppOutBuffer, unsigned int *pOutBytes);
static int mp3_volume_process(void *pInBuffer, unsigned int inBytes,
                              uint32_t *vc, uint32_t *vp,
                              void **ppOutBuffer, unsigned int *pOutBytes);

static int mp3_init_bsbuffer(void);
static int mp3_fillbs_fromDram(uint32_t bs_fill_size);
static int mp3_decode_process(void);
static void mp3_get_timestamp(struct AudioTask *this);
static void mp3_process_pause(uint8_t enable);
static int mp3_dma_length_fourbyte(uint32_t len, uint32_t dst_addr,
                                   uint32_t src_addr);
int BsbufferSize(void);
int PcmsbufferSize(void);
extern void scp_wakeup_src_setup(wakeup_src_id irq, uint32_t enable);


#if 0
static void           myprint(const char *message, ...);
#endif

/*==============================================================================
 *                     class new/construct/destruct functions
 *============================================================================*/

AudioTask *task_offload_mp3_new()
{
    /* alloc object here */
    AudioTask *task = (AudioTask *)kal_pvPortMalloc(sizeof(AudioTask));
    if (task == NULL) {
        AUD_LOG_E("%s(), kal_pvPortMalloc fail!!\n", __func__);
        return NULL;
    }

    /* only assign methods, but not class members here */
    task->constructor       = task_offload_mp3_constructor;
    task->destructor        = task_offload_mp3_destructor;

    task->create_task_loop  = task_offload_mp3_create_task_loop;

    task->recv_message      = task_offload_mp3_recv_message;

    task->irq_hanlder       = task_offload_mp3_irq_hanlder;


    return task;
}


static void task_offload_mp3_constructor(struct AudioTask *this)
{
    AUD_ASSERT(this != NULL);

    /* assign initial value for class members & alloc private memory here */
    this->scene = TASK_SCENE_PLAYBACK_MP3;
    this->state = AUDIO_TASK_IDLE;

    /* queue */
    this->queue_idx = 0;

    this->msg_array = NULL;

    this->msg_idx_queue = xQueueCreate(MAX_MSG_QUEUE_SIZE, sizeof(uint32_t));

    AUD_ASSERT(this->msg_idx_queue != NULL);

    memset(&afe_offload_block, 0, sizeof(afe_offload_block));
    afe_offload_block.volume[0] = 0x0F;
}


static void task_offload_mp3_destructor(struct AudioTask *this)
{
    AUD_LOG_D("%s(), task_scene = %d", __func__, this->scene);

    /* dealloc private memory & dealloc object here */
    AUD_ASSERT(this != NULL);

    // vPortFree(this->msg_array);

    vPortFree(this);
}


static void task_offload_mp3_create_task_loop(struct AudioTask *this)
{
    /* Note: you can also bypass this function,
             and do kal_xTaskCreate until you really need it.
             Ex: create task after you do get the enable message. */

    BaseType_t xReturn = pdFAIL;

    xReturn = kal_xTaskCreate(
                  task_offload_mp3_task_loop,
                  LOCAL_TASK_NAME,
                  LOCAL_TASK_STACK_SIZE,
                  (void *)this,
                  LOCAL_TASK_PRIORITY,
                  NULL);

    AUD_ASSERT(xReturn == pdPASS);
}

static audio_status_t task_offload_mp3_recv_message(
    struct AudioTask *this,
    struct ipi_msg_t *ipi_msg)
{
    static portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    uint8_t ret = false;
    ret = task_offload_mp3_preparsing_message(this, ipi_msg);
    if (ret) {
        if (xQueueSendToBackFromISR(this->msg_idx_queue, &ipi_msg->msg_id,
                                    &xHigherPriorityTaskWoken) != pdTRUE) {
            return UNKNOWN_ERROR;
        }
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return NO_ERROR;
}


static void task_offload_mp3_irq_hanlder(
    struct AudioTask *this,
    uint32_t irq_type)
{
    enable_clk_bus_from_isr(MP3_AUDSYS_MEM_ID);
    if (irq_type == AUDIO_IRQn) {
        int IRQSource, data;
        IRQSource = DRV_Reg32(AFE_IRQ_MCU_STATUS);
        if ((IRQSource & 0xff) == 0) {
            data = DRV_Reg32(AFE_IRQ_MCU_CLR);
            DRV_WriteReg32(AFE_IRQ_MCU_CLR,
                           data); /*Clears the MCU IRQ for AFE while all IRQ statuses are 0*/
        }
        /*check IRQ 7*/
        if (IRQSource & IRQ7_FLAG) {
            DRV_WriteReg32(AFE_IRQ_MCU_CLR, (IRQSource & 0xff));
            /*update DL read pointer*/
            data = DRV_Reg32(AFE_DL3_CUR);

            if (bDramPlayback == 0) {
                rMemDL.pRead = (char *)(data - AFE_INTERNAL_SRAM_PHY_BASE +
                                        AFE_INTERNAL_SRAM_CM4_MAP_BASE);
            } else {
                if (data != 0) {
                    rMemDL.pRead = (char *)ap_to_scp((uint32_t)data);
                }
            }
            if (this->state == AUDIO_TASK_WORKING) {
                static portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
                int state = MP3_RUN;
                if (xQueueSendToBackFromISR(this->msg_idx_queue, &state,
                                            &xHigherPriorityTaskWoken) != pdTRUE) {
                    return;
                }
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}



static void task_offload_mp3_task_loop(void *pvParameters)
{
    AudioTask *this = (AudioTask *)pvParameters;
    uint8_t local_queue_idx = 0xFF;

    while (1) {
        if (xQueueReceive(this->msg_idx_queue, &local_queue_idx,
                          portMAX_DELAY) == pdTRUE) {
            //   AUD_LOG_E("vMP3TaskProject received = %d\n", local_queue_idx);
        }
        /*wakeup  on state change*/
        switch (local_queue_idx) {
            case MP3_SETMEM: {
                this->state = AUDIO_TASK_INIT;
                task_offload_mp3_init(this);
            }
            break;
            case MP3_RUN: {
                if (this->state == AUDIO_TASK_DEINIT) {
                    this->state = AUDIO_TASK_INIT;
                    task_offload_mp3_init(this);
                }

                if (this->state != AUDIO_TASK_DEINIT) {
                    this->state = AUDIO_TASK_WORKING;
                    task_offload_mp3_working(this);
                }
            }
            break;
            case MP3_CLOSE: {
                this->state = AUDIO_TASK_DEINIT;
                task_offload_mp3_deinit(this);
            }
            break;
        }
    }
}

uint8_t task_offload_mp3_preparsing_message(struct AudioTask *this,
        ipi_msg_t *ipi_msg)
{
    uint8_t ret = false;

    unsigned int *item = (unsigned int *)ipi_msg->payload;
    switch (ipi_msg->msg_id) {
        case MP3_SETPRAM: {
            this->state = AUDIO_TASK_INIT;
            afe_offload_block.channels = item[0];
            afe_offload_block.samplerate = item[1];
            afe_offload_block.format = item[2];
        }
        break;
        case MP3_SETMEM: {
            this->state = AUDIO_TASK_INIT;
            rMemDRAM.pBufBase = (char *)item[0];
            rMemDRAM.bufLen = item[1];
            rMemDL.pBufBase = (char *)item[2];
            rMemDL.bufLen = item[3];
            bDramPlayback = item[4];
            this->state = AUDIO_TASK_WORKING;
            ret = true;
        }
        break;
        case MP3_INIT: {
            this->state = AUDIO_TASK_INIT;
        }
        break;
        case MP3_SETWRITEBLOCK: {
            rMemDRAM.pWrite = rMemDRAM.pBufBase + ipi_msg->param1;
        }
        break;
        case MP3_RUN: {
            if (afe_offload_block.bIsPause && !afe_offload_block.bIsClose) {
                mp3_process_pause(false);
            }
            ret = true;
        }
        break;
        case MP3_PAUSE: {
            mp3_process_pause(true);
        }
        break;
        case MP3_CLOSE: {
            afe_offload_block.bIsClose = true;
            afe_offload_block.volume[1] = afe_offload_block.volume[0];
            afe_offload_block.volume[0] = 0;
            mp3_process_pause(true);
            this->state = AUDIO_TASK_DEINIT;
            ret = true;
        }
        case MP3_DRAIN: {
            afe_offload_block.bIsDrain = true;
            rMemDRAM.pWrite = rMemDRAM.pBufBase + ipi_msg->param1;
            AUD_LOG_E("DRAIN IDX = %p\n", rMemDRAM.pWrite);
        }
        break;
        case MP3_VOLUME:
            afe_offload_block.volume[1] =
                afe_offload_block.volume[0]; // record prev v.s. current volume
            afe_offload_block.volume[0] = ipi_msg->param1;
            //afe_offload_block.volume[1]= item[2];
            break;
        case MP3_WRITEIDX:
            rMemDRAM.pWrite = rMemDRAM.pBufBase + ipi_msg->param1;
            break;
        case MP3_TSTAMP:
            afe_offload_block.bgetTime = true;
            break;
    }
    return ret;
    /* clean msg */

}

static audio_status_t task_offload_mp3_init(struct AudioTask *this)
{
    AUD_LOG_D("%s(+)\n", __func__);
    //init decoder ---------------------------------------------------------
    mp3_init_decoder();
    //init Sram addr & buffer-----------------------------------------------
    uint32_t u4PlaybackPhyBase = 0;
    if (bDramPlayback == 0) {
        if ((0xf0000000 & (uint32_t)rMemDL.pBufBase) == (0xf0000000 &
                AFE_INTERNAL_SRAM_PHY_BASE)) {
            u4PlaybackPhyBase = (uint32_t)rMemDL.pBufBase - AFE_INTERNAL_SRAM_PHY_BASE +
                                AFE_INTERNAL_SRAM_CM4_MAP_BASE;
        } else {
            u4PlaybackPhyBase = AFE_INTERNAL_SRAM_CM4_MAP_BASE;
            rMemDL.bufLen = 0x8000;
            AUD_LOG_E("Somthing wrong in passing MEM params\n");
        }
    } else {
        u4PlaybackPhyBase = ap_to_scp((uint32_t)rMemDL.pBufBase);
    }
    rMemDL.pBufBase = (char *)u4PlaybackPhyBase; // SW use virtual addr
    rMemDL.pBufEnd  = (char *)u4PlaybackPhyBase + rMemDL.bufLen;
    rMemDL.pRead    = rMemDL.pBufBase; // CUR
    rMemDL.pWrite   = rMemDL.pBufBase;

    AUD_LOG_D("[process_init] SRAM BufBase = %p, BufEnd = %p, Read = %p, Write = %p\n",
              rMemDL.pBufBase, rMemDL.pBufEnd, rMemDL.pRead, rMemDL.pWrite);

    //DRAM setting ------------------------------------------------------------------
    rMemDRAM.pBufBase = (char *)ap_to_scp((uint32_t)rMemDRAM.pBufBase);
    rMemDRAM.pBufEnd  = rMemDRAM.pBufBase + rMemDRAM.bufLen;
    rMemDRAM.pRead    = rMemDRAM.pBufBase; // CUR
    rMemDRAM.pWrite = rMemDRAM.pBufBase + (rMemDRAM.bufLen - 1);

    AUD_LOG_D("[process_init] DRAM BufBase = %p, BufEnd = %p, Read = %p, Write = %p\n",
              rMemDRAM.pBufBase, rMemDRAM.pBufEnd, rMemDRAM.pRead, rMemDRAM.pWrite);
    afe_offload_block.bIsDrain = false;
    afe_offload_block.bIsClose = false;
    afe_offload_block.bIsPause = false;
    register_feature(MP3_FEATURE_ID);
    unmask_irq(AUDIO_IRQn);
    scp_wakeup_src_setup(AUDIO_WAKE_CLK_CTRL, true);
    scp_ipi_wakeup_ap_registration(IPI_AUDIO);
    AUD_LOG_D("%s(-)\n", __func__);
    return NO_ERROR;
}


static audio_status_t task_offload_mp3_working(struct AudioTask *this)
{
    AUD_LOG_V("%s()\n", __func__);

    int bs_buf_needfill = 0;

    mp3_init_bsbuffer();

    mp3_get_timestamp(this);

    bs_buf_needfill = bs_buf.u4BufferSize - bs_buf.u4DataRemained;

    //check dram data
    if (!afe_offload_block.bIsDrain) {
        if ((rMemDRAM.pRead < rMemDRAM.pWrite) &&
                (rMemDRAM.pRead > (rMemDRAM.pWrite - USE_PERIODS_MAX * 8))) {
            audio_send_ipi_msg(this->scene, AUDIO_IPI_MSG_ONLY, AUDIO_IPI_MSG_BYPASS_ACK,
                               MP3_NEEDDATA, (unsigned int)(rMemDRAM.pRead - rMemDRAM.pBufBase),
                               0, NULL);
        }
    }

    mp3_fillbs_fromDram(bs_buf_needfill);


    while ((bs_buf.u4DataRemained > (BsbufferSize() >> 4)) &&
            !afe_offload_block.bIsPause) {
        int count = 0;
        if (AFEenable) {
            if ((count = RingBuf_getFreeSpace(&rMemDL)) == 0) { /* no space in sram*/
                break;
            }
        }
        if ((bs_buf.consumedBS == 1) && afe_offload_block.bIsDrain &&
                !bs_buf.bneedFill) {
            break;
        }
        /* do decode */
        mp3_decode_process();

        mp3_init_blisrc(mMp3Dec->handle->sampleRateIndex, mMp3Dec->handle->CHNumber,
                        afe_offload_block.samplerate, afe_offload_block.channels);
        /* do blisrc 16in 16out*/
        void *pBufferAfterBliSrc = NULL;
        unsigned int bytesAfterBliSrc = 0;

        mp3_blisrc_process(mMp3Dec->pcm_buf, mMp3Dec->pcm_size, &pBufferAfterBliSrc,
                           &bytesAfterBliSrc);
        /* do Volume Gain */
        void *pBufferAfterSetVolume = NULL;
        unsigned int  bytesAfterSetVolume = 0;
        mp3_volume_process(pBufferAfterBliSrc, bytesAfterBliSrc,
                           &afe_offload_block.volume[0], &afe_offload_block.volume[1],
                           &pBufferAfterSetVolume, &bytesAfterSetVolume);

        void *pBufferAfterBitConvertion = NULL;
        unsigned int  bytesAfterBitConvertion = 0;
        mp3_bitconvert_process(pBufferAfterSetVolume, bytesAfterSetVolume,
                               &pBufferAfterBitConvertion, &bytesAfterBitConvertion);
        /* copy pcm to SRAM */
#if 1
        if (bRemainedPcm) {
            /* flush remined pcm buf first */
            RingBuf_copyFromLinear_dma(&rMemDL, pRemainedPCM_Buf, RemainedPcmlen,
                                       bDramPlayback);
            // PRINTF(" Flush RemainedPCM  = %d\n",RemainedPcmlen);
            RemainedPcmlen = 0;
            bRemainedPcm = false;
            /* copy pcm buffer on it */
            if ((count = RingBuf_getFreeSpace(&rMemDL)) >=  bytesAfterBitConvertion) {
                RingBuf_copyFromLinear_dma(&rMemDL, pBufferAfterBitConvertion,
                                           bytesAfterBitConvertion, bDramPlayback);
            } else {
                RingBuf_copyFromLinear_dma(&rMemDL, pBufferAfterBitConvertion, count,
                                           bDramPlayback);
                RemainedPcmlen = bytesAfterBitConvertion - count;
                memcpy(pRemainedPCM_Buf, pBufferAfterBitConvertion + count,
                       RemainedPcmlen); //copy  to pcm remained buf
                bRemainedPcm = true;
            }
        } else {
            if (count >=  bytesAfterBitConvertion) {
                RingBuf_copyFromLinear_dma(&rMemDL, pBufferAfterBitConvertion,
                                           bytesAfterBitConvertion, bDramPlayback);
            } else {
                RingBuf_copyFromLinear_dma(&rMemDL, pBufferAfterBitConvertion, count,
                                           bDramPlayback);
                RemainedPcmlen = bytesAfterBitConvertion - count;
                memcpy(pRemainedPCM_Buf, pBufferAfterBitConvertion + count,
                       RemainedPcmlen); //copy  to pcm remained buf
                bRemainedPcm = true;
            }
        }
    }
    disable_clk_bus(MP3_AUDSYS_MEM_ID);
    // AUD_LOG_E("-FINISHED\n");
    if ((bs_buf.consumedBS == 1) //|| bs_buf.u4DataRemained < (BsbufferSize() >> 4))
            && afe_offload_block.bIsDrain && !bs_buf.bneedFill) {
        PRINTF_D("DECODE FINISHED!! Send DARIN DONE IPI !!!\n");
        audio_send_ipi_msg(this->scene, AUDIO_IPI_MSG_ONLY, AUDIO_IPI_MSG_BYPASS_ACK,
                           MP3_DRAINDONE, 0, 0, NULL);
    }
#endif
    return NO_ERROR;
}


static audio_status_t task_offload_mp3_deinit(struct AudioTask *this)
{
    AUD_LOG_D("%s()+\n", __func__);
    bfirsttime = true;
    bRemainedPcm = false;
    mask_irq(AUDIO_IRQn);
    //free working memory..
    mp3_deinit_decoder();
    //blisrc
    mp3_deinit_blisrc();
    //free malloc memory
    if (pBitConvertBuf != NULL) {
        kal_vPortFree(pBitConvertBuf);
        pBitConvertBuf = NULL;
    }
    if (pRemainedPCM_Buf != NULL) {
        kal_vPortFree(pRemainedPCM_Buf);
        pRemainedPCM_Buf = NULL;
    }

    //  disable_clk_bus(MP3_AUDSYS_MEM_ID);
    deregister_feature(MP3_FEATURE_ID);
    scp_wakeup_src_setup(AUDIO_WAKE_CLK_CTRL, false);
    AUD_LOG_D("%s()-\n", __func__);
    return NO_ERROR;
}

/*******************************************************
*                             MP3 Decoder API
*
*********************************************************/

int BsbufferSize(void)
{
#ifdef AudioDecoder
    return mMp3Dec->min_bs_size;
#else
    return 0;
#endif

}

int PcmbufferSize(void)
{
#ifdef AudioDecoder
    return mMp3Dec->pcm_size;
#else
    return 0;

#endif

}
static int mp3_init_bsbuffer()
{
#ifdef AudioDecoder
    //check remained data in DRAM
    afe_offload_block.u4DataRemained = RingBuf_getDataCount(&rMemDRAM);
    if (bfirsttime) {
        memset(&pcm_buf, 0, sizeof(pcm_buf));
        bs_buf.consumedBS = 0;
        //copy data from dram to bs buffer
        if (afe_offload_block.u4DataRemained > bs_buf.u4BufferSize) {
            bs_buf.u4BufferSize = (USE_PERIODS_MAX * 8);
            bs_buf.bneedFill = true;
        } else {
            bs_buf.u4BufferSize = afe_offload_block.u4DataRemained;
            bs_buf.bneedFill = false;
        }
        if (bs_buf.pBufAddr == NULL) {
            bs_buf.pBufAddr = kal_pvPortMalloc(bs_buf.u4BufferSize + 3); // add backup bit
            AUD_ASSERT(bs_buf.pBufAddr != NULL);
        }
        mp3_dma_transaction_wrap((uint32_t)bs_buf.pBufAddr, (uint32_t)rMemDRAM.pBufBase,
                                 bs_buf.u4BufferSize, true);
        bs_buf.u4DataRemained = bs_buf.u4BufferSize;

        rMemDRAM.pRead = rMemDRAM.pRead + bs_buf.u4BufferSize;
        bfirsttime = false;
        AUD_LOG_E("initBsBuffer :bs_buf.pBufAddr =%p  bs_buf.u4BufferSize = %d\n",
                  bs_buf.pBufAddr, bs_buf.u4BufferSize);
    }
#endif
    return NO_ERROR;
}

static int mp3_fillbs_fromDram(uint32_t bs_fill_size)
{

    int ret = 0;
#ifdef AudioDecoder
    /*Copy DRAM data to decode buffer*/
    if (bs_fill_size % 4 != 0) {
        uint32_t need_add  = 4 - (bs_fill_size % 4);
        bs_fill_size += need_add;
    }
    if (bs_buf.bneedFill && ((bs_buf.u4DataRemained <= BsbufferSize()) ||
                             (((uint32_t)(bs_buf.pBufAddr + bs_buf.u4DataRemained) % 4 == 0) &&
                              (bs_buf.u4DataRemained <= 2 * BsbufferSize())))) {
        /*should only happened in Drain state, check is drain*/
        if (afe_offload_block.u4DataRemained < bs_fill_size) {
            AUD_LOG_D("[process_run] AFE DATA = %d < bs_buf_needfill \n",
                      afe_offload_block.u4DataRemained);
            if (afe_offload_block.bIsDrain) {
                if (rMemDRAM.pWrite > rMemDRAM.pRead) {
                    ret = mp3_dma_transaction_wrap((uint32_t)(bs_buf.pBufAddr +
                                                   bs_buf.u4DataRemained), (uint32_t)rMemDRAM.pRead,
                                                   ((afe_offload_block.u4DataRemained >> 2) << 2), true);
                } else {
                    int data_length = rMemDRAM.pBufEnd - rMemDRAM.pRead;
                    ret = mp3_dma_transaction_wrap((uint32_t)(bs_buf.pBufAddr +
                                                   bs_buf.u4DataRemained), (uint32_t)rMemDRAM.pRead, data_length, true);
                    data_length = (uint32_t)(rMemDRAM.pWrite - rMemDRAM.pBufBase);
                    ret = mp3_dma_transaction_wrap((uint32_t)(bs_buf.pBufAddr +
                                                   bs_buf.u4DataRemained + data_length), (uint32_t)rMemDRAM.pBufBase,
                                                   (uint32_t)(rMemDRAM.pWrite - rMemDRAM.pBufBase), true);
                }
                bs_buf.u4DataRemained += afe_offload_block.u4DataRemained;
                rMemDRAM.pRead += afe_offload_block.u4DataRemained;
                bs_buf.bneedFill = false; /*copy done*/
            }
        } else {
            if (rMemDRAM.pWrite >= rMemDRAM.pRead) {
                AUD_LOG_D("COPY01 needfill = %d\n", bs_fill_size);
                ret = mp3_dma_transaction_wrap((uint32_t)(bs_buf.pBufAddr +
                                               bs_buf.u4DataRemained),
                                               (uint32_t)rMemDRAM.pRead, bs_fill_size, true);
                AUD_LOG_E("[process_run] COPY01 BSADDR = %p, rMemDRAM.pRead = %p,size = %d\n",
                          (bs_buf.pBufAddr + bs_buf.u4DataRemained), rMemDRAM.pRead, bs_fill_size);
            } else {
                int data_length =  rMemDRAM.pBufEnd -  rMemDRAM.pRead;
                if (bs_fill_size <= data_length) {
                    ret = mp3_dma_transaction_wrap((uint32_t)(bs_buf.pBufAddr +
                                                   bs_buf.u4DataRemained), (uint32_t)rMemDRAM.pRead, bs_fill_size, true);
                } else {
                    ret = mp3_dma_transaction_wrap((uint32_t)(bs_buf.pBufAddr +
                                                   bs_buf.u4DataRemained), (uint32_t)rMemDRAM.pRead, data_length, true);
                    ret = mp3_dma_transaction_wrap((uint32_t)(bs_buf.pBufAddr +
                                                   bs_buf.u4DataRemained + data_length), (uint32_t)rMemDRAM.pBufBase,
                                                   (uint32_t)(bs_fill_size - data_length), true);
                }
                AUD_LOG_D("COPY02 needfill = %d\n", bs_fill_size);
            }
            bs_buf.u4DataRemained += bs_fill_size;
            rMemDRAM.pRead += bs_fill_size;
        }
        if (rMemDRAM.pRead == rMemDRAM.pBufEnd) {
            rMemDRAM.pRead = rMemDRAM.pBufBase;
        } else if (rMemDRAM.pRead > rMemDRAM.pBufEnd) {
            rMemDRAM.pRead = rMemDRAM.pBufBase + (rMemDRAM.pRead - rMemDRAM.pBufEnd);
        }
    }
#endif
    return ret;
}

static int mp3_decode_process(void)
{
#ifdef AudioDecoder
    AUD_LOG_D(" + Decoderprocess()\n");
#ifdef CYCLE
    volatile uint32_t mycount = 0;
    volatile uint32_t offset = 0;
    CPU_RESET_CYCLECOUNTER;
    __asm volatile("nop");
    mycount = *DWT_CYCCNT;
    offset = mycount - 1;
    CPU_RESET_CYCLECOUNTER;
    /*STEP 4 : Start to decode */
    bs_buf.consumedBS = MP3Dec_Decode(mMp3Dec->handle, mMp3Dec->pcm_buf,
                                      bs_buf.pBufAddr, bs_buf.u4BufferSize,
                                      bs_buf.pBufAddr);
    mycount = *DWT_CYCCNT - offset;
    AUD_LOG_D("\n\r Cycle count %d\n", mycount);
#else
    bs_buf.consumedBS = MP3Dec_Decode(mMp3Dec->handle, mMp3Dec->pcm_buf,
                                      bs_buf.pBufAddr, bs_buf.u4BufferSize,
                                      bs_buf.pBufAddr);
#endif
    bs_buf.u4DataRemained -= bs_buf.consumedBS;
    pcm_buf.u4DataRemained += PcmbufferSize();
    if (bs_buf.consumedBS == 1) {
        AUD_LOG_E("bs_buf.consumedBS == 1,BS remained = %d\n", bs_buf.u4DataRemained);
    }
    memmove(bs_buf.pBufAddr, bs_buf.pBufAddr + bs_buf.consumedBS,
            bs_buf.u4DataRemained);
    if (!AFEenable && !afe_offload_block.bIsPause &&
            pcm_buf.u4DataRemained > 16000) {
        /*trun on mem interface*/
        AUD_LOG_D("Turn on DL3!\n");
        mp3_process_pause(false);
    }
#endif
    return 0;
}


static int mp3_init_decoder(void)
{
#ifdef AudioDecoder
    if (!mMp3InitFlag) {
        //     PRINTF("+%s()", __func__);
        mMp3Dec = (mp3DecEngine *)kal_pvPortMalloc(sizeof(mp3DecEngine));
        if (mMp3Dec == NULL) {
            AUD_LOG_E("%s() allocate engine fail", __func__);
        }
        memset(mMp3Dec, 0, sizeof(mp3DecEngine));
        MP3Dec_GetMemSize(&mMp3Dec->min_bs_size, &mMp3Dec->pcm_size,
                          &mMp3Dec->workingbuf_size1, &mMp3Dec->workingbuf_size2);

        AUD_LOG_V("%s >> min_bs_size=%u, pcm_size=%u, workingbuf_size1=%u,workingbuf_size2=%u",
                  __func__,
                  mMp3Dec->min_bs_size, mMp3Dec->pcm_size, mMp3Dec->workingbuf_size1,
                  mMp3Dec->workingbuf_size2);

        mMp3Dec->working_buf1 = kal_pvPortMalloc(mMp3Dec->workingbuf_size1);
        mMp3Dec->working_buf2 = kal_pvPortMalloc(mMp3Dec->workingbuf_size2);
        mMp3Dec->pcm_buf      = kal_pvPortMalloc(mMp3Dec->pcm_size);

        if ((NULL == mMp3Dec->working_buf1) || (NULL == mMp3Dec->working_buf2)) {
            AUD_LOG_E("%s() allocate working buf fail", __func__);
            return false;
        }

        memset(mMp3Dec->working_buf1, 0, mMp3Dec->workingbuf_size1);
        memset(mMp3Dec->working_buf2, 0, mMp3Dec->workingbuf_size2);
        memset(mMp3Dec->pcm_buf, 0, mMp3Dec->pcm_size);

        if (mMp3Dec->handle == NULL) {
            mMp3Dec->handle = MP3Dec_Init(mMp3Dec->working_buf1, mMp3Dec->working_buf2);
            if (mMp3Dec->handle == NULL) {

                AUD_LOG_E("%s() Init Decoder Fail", __func__);

                if (mMp3Dec->working_buf1) {
                    kal_vPortFree(mMp3Dec->working_buf1);
                    mMp3Dec->working_buf1 = NULL;
                }

                if (mMp3Dec->working_buf2) {
                    kal_vPortFree(mMp3Dec->working_buf2);
                    mMp3Dec->working_buf2 = NULL;
                }

                if (mMp3Dec->pcm_buf) {
                    kal_vPortFree(mMp3Dec->pcm_buf);
                    mMp3Dec->pcm_buf = NULL;
                }

                kal_vPortFree(mMp3Dec);
                mMp3Dec = NULL;
                return false;
            }
        }
        mMp3InitFlag = true;
    }
#endif
    return 0;

}

static void mp3_deinit_decoder(void)
{
    // PRINTF("+%s()", __func__);
#ifdef AudioDecoder
    if ((mMp3InitFlag == true) && (mMp3Dec != NULL)) {
        if (mMp3Dec->working_buf1) {
            kal_vPortFree(mMp3Dec->working_buf1);
            mMp3Dec->working_buf1 = NULL;
        }

        if (mMp3Dec->working_buf2) {
            kal_vPortFree(mMp3Dec->working_buf2);
            mMp3Dec->working_buf2 = NULL;
        }

        if (mMp3Dec->pcm_buf) {
            kal_vPortFree(mMp3Dec->pcm_buf);
            mMp3Dec->pcm_buf = NULL;
        }
        kal_vPortFree(mMp3Dec);
        mMp3Dec = NULL;
        mMp3InitFlag = false;
    }

    if (bs_buf.pBufAddr) {
        kal_vPortFree(bs_buf.pBufAddr);
        bs_buf.pBufAddr = NULL;
    }
#endif
    //  PRINTF("-%s()", __func__);
}

static void mp3_process_pause(uint8_t enable)
{
    /*trun off mem interface*/
    if (enable && AFEenable) {
        afe_offload_block.bIsPause = true;
        mask_irq(AUDIO_IRQn);
        unsigned int gReg_AFE_DAC_CON0 = DRV_Reg32(AFE_DAC_CON0);
        vResetVarBit(gReg_AFE_DAC_CON0, DL3_ON);
        DRV_WriteReg32(AFE_DAC_CON0, gReg_AFE_DAC_CON0);
        AFEenable = false;
        AUD_LOG_E(" AFE Disable DL3 PATH\n");
    } else if (!enable && !AFEenable) {
        afe_offload_block.bIsPause = false;
        unmask_irq(AUDIO_IRQn);
        unsigned int gReg_AFE_DAC_CON0 = DRV_Reg32(AFE_DAC_CON0);
        vSetVarBit(gReg_AFE_DAC_CON0, DL3_ON);
        DRV_WriteReg32(AFE_DAC_CON0, gReg_AFE_DAC_CON0);
        AFEenable = true;
        AUD_LOG_E(" AFE Enable DL3 PATH\n");

    }

}


/*******************************************************
*                            DMA Control   API
*
*********************************************************/
#ifdef AudioDecoder

static int mp3_dma_length_fourbyte(uint32_t len, uint32_t dst_addr,
                                   uint32_t src_addr)
{
    if (len % 4 || dst_addr % 4 || src_addr % 4) {
        return 0; //config can't four byte
    } else {
        return 1;
    }
}

int mp3_dma_transaction_wrap(uint32_t dst_addr, uint32_t src_addr,
                             uint32_t len, uint8_t IsDram)
{
    DMA_RESULT ret = 0;
    if (IsDram) {
        dvfs_enable_DRAM_resource(MP3_MEM_ID);
    } else {
        enable_clk_bus(MP3_MEM_ID);
    }

    if (mp3_dma_length_fourbyte(len, dst_addr, src_addr) == 0) {
        uint32_t need_add  = 4 - (dst_addr % 4);
        ret = dma_transaction_manual(dst_addr + need_add, src_addr, len, NULL,
                                     (uint32_t *)&dma_ch);
        memmove((void *)dst_addr, (void *)(dst_addr + need_add), len);
    } else {
        ret = dma_transaction_manual(dst_addr, src_addr, len, NULL,
                                     (uint32_t *)&dma_ch);
    }
    if (IsDram) {
        dvfs_disable_DRAM_resource(MP3_MEM_ID);
    } else {
        disable_clk_bus(MP3_MEM_ID);
    }
    return ret;
}
#endif

/*******************************************************
*                             Blisrc API
*
*********************************************************/
static int mp3_init_blisrc(int32_t inSampleRate, int32_t inChannelCount,
                           int32_t outSampleRate, int32_t outChannelCount)
{
#ifdef BliSrc
    int32_t result;
    if (!mSrcInitFlag) {
        if ((inSampleRate > 8) || (inSampleRate < 0) || inChannelCount < 0) {
            inSampleRate = outSampleRate;
            inChannelCount = outChannelCount;
            AUD_LOG_E("Blisrc failed ins = %d outs = %d inch = %d outch = %d\n",
                      inSampleRate, outSampleRate, inChannelCount, outChannelCount);
        } else {
            inSampleRate = decoder_sameplerateidx[inSampleRate];
            AUD_LOG_D("Blisrc ins = %d outs = %d inch = %d outch = %d\n",
                      inSampleRate, outSampleRate, inChannelCount, outChannelCount);
        }

        mBliSrc = (blisrcEngine *)kal_pvPortMalloc(sizeof(blisrcEngine));
        if (mBliSrc == NULL) {
            AUD_LOG_E("%s() allocate engine fail", __func__);
            AUD_ASSERT(mBliSrc != NULL);
        }
        memset(mBliSrc, 0, sizeof(blisrcEngine));
        // set params
        mBliSrc->mBliParam.in_channel = inChannelCount;
        mBliSrc->mBliParam.in_sampling_rate = inSampleRate;
        mBliSrc->mBliParam.ou_channel = outChannelCount;
        mBliSrc->mBliParam.ou_sampling_rate = outSampleRate;
        mBliSrc->mBliParam.PCM_Format = BLISRC_IN_Q1P15_OUT_Q1P15;
        result = Blisrc_GetBufferSize(&mBliSrc->mInternalBufSize,
                                      &mBliSrc->mTempBufSize, &mBliSrc->mBliParam);
        if (result < 0) {
            AUD_LOG_E("Blisrc_GetBufferSize error %d", result);
        }
        mBliSrc->mInternalBuf = kal_pvPortMalloc(mBliSrc->mInternalBufSize);
        if (NULL == mBliSrc->mInternalBuf) {
            AUD_LOG_E("%s() allocate working buf fail", __func__);
            AUD_ASSERT(mBliSrc->mInternalBuf != NULL);
            return false;
        }
        if (Blisrc_Open(&mBliSrc->mHandle, (void *)mBliSrc->mInternalBuf,
                        &mBliSrc->mBliParam) != 0) {
            AUD_LOG_E("Blisrc_Open error \n");
            if (mBliSrc->mInternalBuf) {
                kal_vPortFree(mBliSrc->mInternalBuf);
                mBliSrc->mInternalBuf = NULL;
            }
            kal_vPortFree(mBliSrc);
            mBliSrc = NULL;
            result = false;
        } else {
            Blisrc_Reset(mBliSrc->mHandle);
            result = true;
            mSrcInitFlag = true;
            AUD_LOG_D("Blisrc_Open Success ins = %d outs = %d inch = %d outch = %d\n",
                      inSampleRate, outSampleRate, inChannelCount, outChannelCount);
        }
    }
    return result;
#else
    return 0;
#endif
}

//wait for complete

static int mp3_blisrc_process(void *pInBuffer, unsigned int inBytes,
                              void **ppOutBuffer, unsigned int *pOutBytes)
{
#ifdef BliSrc
    unsigned int outputIndex = 0;
    unsigned int outBuffSize = (mMp3Dec->pcm_size) << 1;
    int totalPutSize = 0;
    int putSize = inBytes ;
    unsigned int ouputlen = 0;
    unsigned int inLen;
    unsigned int remained;
    if (pSrcOutBuf == NULL) {
        pSrcOutBuf = kal_pvPortMalloc(outBuffSize);
        AUD_LOG_D("- BlisrcProcess() pSrcOutBuf addr = %p\n", pSrcOutBuf);
        AUD_ASSERT(pSrcOutBuf != NULL);
    }
    while (totalPutSize < putSize) {
        //    AUD_LOG_E(" BlisrcProcess() totalPutSize = %d, remained = %d outBuffSize = %d\n",
        //              totalPutSize, putSize, outBuffSize);
        remained = (putSize - totalPutSize);
        inLen = remained;
        Blisrc_Process(mBliSrc->mHandle, NULL, (void *)pInBuffer + totalPutSize,
                       &remained,
                       (void *)pSrcOutBuf + ouputlen, &outBuffSize);
        totalPutSize += (inLen - remained);
        outputIndex += outBuffSize;
        ouputlen = outputIndex;
    }

    *ppOutBuffer = pSrcOutBuf;
    *pOutBytes = outputIndex;
#else
    *ppOutBuffer = pInBuffer;
    *pOutBytes = inBytes;
#endif
    return 0;
}


static int mp3_deinit_blisrc()
{
#ifdef BliSrc
    AUD_LOG_V("+%s()", __func__);
    if ((mSrcInitFlag == true) && (mBliSrc != NULL)) {
        if (mBliSrc->mInternalBuf) {
            kal_vPortFree(mBliSrc->mInternalBuf);
            mBliSrc->mInternalBuf = NULL;
        }

        kal_vPortFree(pSrcOutBuf);
        kal_vPortFree(mBliSrc);
        pSrcOutBuf = NULL;
        mBliSrc = NULL;
        mSrcInitFlag = false;

    }

    AUD_LOG_V("-%s()", __func__);
#endif
    return 0;
}

/*******************************************************
*                            Volume process API
*
*********************************************************/
#ifdef POST_PROCESS
#if 1
static int mp3_volume_process(void *pInBuffer, unsigned int inBytes,
                              uint32_t *vc, uint32_t *vp, void **ppOutBuffer, unsigned int *pOutBytes)
{
    //change to AUDIO_FORMAT_PCM_8_24_BIT
    int16_t *ptr16 ;
    ptr16 = (int16_t *)pInBuffer;
    int16_t data16 = 0;
    int data32 = 0;
    uint32_t volume_cur = *vc;
    uint32_t volume_prev = *vp;
    int32_t diff = (volume_cur - volume_prev);
    uint32_t count = inBytes >> 1;

    if (volume_prev == 0xF || diff == 0) { //initial value //no need to ramp
        while (count) {
            data16 = *ptr16;
            if (0x1000000 == volume_cur) {
                break;
            }
            data32 = data16 * (int16_t)(volume_cur >> 8);
            *ptr16 = (int16_t)(data32 >> 16);
            ptr16++;
            count--;
        }
    } else if (diff != 0) { //need ramp
        if (volume_cur > volume_prev) { //do ramp up
            int Volume_inverse = (diff / count);
            int ConsumeSample = 0;
            while ((count - 1)) {
                data16 = *ptr16;
                data32 = data16 * (int16_t)((volume_prev + (Volume_inverse * ConsumeSample)) >>
                                            8);
                *ptr16 = (int16_t)(data32 >> 16);
                ptr16++;
                count--;
                ConsumeSample++;
            }
            data16 = *ptr16;
            data32 = data16 * (int16_t)(volume_cur >> 8);
            *ptr16 = (int16_t)(data32 >> 16);
            AUD_LOG_V("[ramp up] Volume_inverse = %d volume_cur = %d volume_prev = %d\n",
                      Volume_inverse
                      , volume_cur, volume_prev);
        } else { //do ramp down
            diff = (volume_prev - volume_cur);
            int Volume_inverse = (diff / count);
            int ConsumeSample = 0;
            while ((count - 1)) {
                data16 = *ptr16;
                data32 = data16 * (int16_t)((volume_prev - (Volume_inverse * ConsumeSample)) >>
                                            8);
                *ptr16 = (int16_t)(data32 >> 16);
                ptr16++;
                count--;
                ConsumeSample++;
            }
            data16 = *ptr16;
            data32 = data16 * (int16_t)(volume_cur >> 8);
            *ptr16 = (int16_t)(data32 >> 16);
            AUD_LOG_V("[ramp down] Volume_inverse = %d volume_cur = %d volume_prev = %d\n",
                      Volume_inverse
                      , volume_cur, volume_prev);
        }
    }
    *ppOutBuffer = pInBuffer;
    *pOutBytes = inBytes;
    *vp = *vc; //only process one frame
    return 0;
}
#endif
#endif

static int mp3_bitconvert_process(void *pInBuffer, unsigned int inBytes,
                                  void **ppOutBuffer, unsigned int *pOutBytes)
{
    int pcmsize = inBytes; //44k max
    pcmsize = 6912; //48k max
    //change to AUDIO_FORMAT_PCM_8_24_BIT
    if (pBitConvertBuf == NULL) {
        pBitConvertBuf = kal_pvPortMalloc(pcmsize << 1); //same as pcm buf size
        AUD_ASSERT(pBitConvertBuf != NULL);
    }
    if (pRemainedPCM_Buf == NULL) {
        pRemainedPCM_Buf = kal_pvPortMalloc(pcmsize << 1); //same as pcm buf size
        AUD_LOG_E(" + mp3_bitconvert, pRemainedPCM_Buf = %p\n", pRemainedPCM_Buf);
        AUD_ASSERT(pRemainedPCM_Buf != NULL);
    }
    int16_t *ptr16;
    int *ptr32 = pBitConvertBuf;
    int data32 = 0;
    int ptr32_cnt;
    ptr16 = (int16_t *)pInBuffer;
    //PRINTF("32Buf = %p, ptr16 = %p\n",pBitConvertBuf,ptr16);
    for (ptr32_cnt = 0; ptr32_cnt < (inBytes >> 1); ptr32_cnt++) {
        data32 = *ptr16;
        data32 = (data32 & 0xFFFF) << 8;
        *ptr32 = data32;
        ptr16++;
        ptr32++;
    }
    *ppOutBuffer = pBitConvertBuf;
    *pOutBytes = inBytes * 2;
    return 0;
}
//DMA don not need data pending function since HW  dont need 64byte align


static void mp3_get_timestamp(struct AudioTask *this)
{
    if (afe_offload_block.bgetTime == true) {

        AUD_LOG_D("+%s() data = %d\n", __func__, (unsigned int)pcm_buf.u4DataRemained);
        audio_send_ipi_msg(this->scene, AUDIO_IPI_MSG_ONLY, AUDIO_IPI_MSG_BYPASS_ACK,
                           MP3_PCMCONSUMED,
                           (unsigned int)pcm_buf.u4DataRemained, 0, NULL);
        afe_offload_block.bgetTime = false;
    }
}
#if 0
static void myprint(const char *message, ...)
{
    va_list args;
    va_start(args, message);

#ifdef DEBUG_KERNEL
    vprintk(message, ##args );
#endif

#ifdef ANDROID_HAL
    __android_log_vprint(ANDROID_LOG_DEBUG, "TAG", message, args);
#endif

#ifdef DEBUG_CM4
    AUD_LOG_D("[Setlog]");
    vAUD_LOG_D(message, args);
#endif
    va_end(args);
}
#endif

