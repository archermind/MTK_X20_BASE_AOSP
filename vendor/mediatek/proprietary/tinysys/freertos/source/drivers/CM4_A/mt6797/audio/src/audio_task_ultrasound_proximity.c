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

#include "audio_task_ultrasound_proximity.h"

#include <interrupt.h>
#include <vcore_dvfs.h>

#include "audio_messenger_ipi.h"
#include "audio_task_factory.h"
#include "uSnd_service.h"


#define MAX_MSG_QUEUE_SIZE (8)

#define LOCAL_TASK_STACK_SIZE (1024)
#define LOCAL_TASK_NAME "uSnd"
#define LOCAL_TASK_PRIORITY (2)

#define IRQ4_SOURCE 0x08

#define AFE_SRAM_SIZE    (0xD000)
#define AFE_SRAM_CM4_MAP_BASE   (0xD0001000)
#define AFE_SRAM_PHY_BASE       (0x11221000)
#define AFE_SRAM_PHY_END        (0x1122CFFF)


#define AFE_CM4_MAP_BASE        (0xD0000000)

#define AFE_DAC_CON0            (AFE_CM4_MAP_BASE + 0x0010)
#define AFE_DAC_CON1            (AFE_CM4_MAP_BASE + 0x0014)


#define AFE_AWB_BASE            (AFE_CM4_MAP_BASE + 0x0070)
#define AFE_AWB_END             (AFE_CM4_MAP_BASE + 0x0078)
#define AFE_AWB_CUR             (AFE_CM4_MAP_BASE + 0x007C)
#define AFE_VUL_D2_BASE         (AFE_CM4_MAP_BASE + 0x0350)
#define AFE_VUL_D2_END          (AFE_CM4_MAP_BASE + 0x0358)
#define AFE_VUL_D2_CUR          (AFE_CM4_MAP_BASE + 0x035C)
#define AFE_DL3_BASE            (AFE_CM4_MAP_BASE + 0x0360)
#define AFE_DL3_END             (AFE_CM4_MAP_BASE + 0x0368)
#define AFE_DL3_CUR             (AFE_CM4_MAP_BASE + 0x0364)


#define AFE_IRQ_MCU_CON         (AFE_CM4_MAP_BASE + 0x03A0)
#define AFE_IRQ_MCU_STATUS      (AFE_CM4_MAP_BASE + 0x03A4)
#define AFE_IRQ_MCU_CLR         (AFE_CM4_MAP_BASE + 0x03A8)
#define AFE_IRQ_MCU_CNT4        (AFE_CM4_MAP_BASE + 0x03E8)

#define UL_SAMPLE_PER_S          (260000)
#define DL_SPH_SAMPLE_PER_S       (16000)

#define DL_SAMPLE_PER_S_96K       (96000)
#define DL_SAMPLE_PER_S_192K     (192000)

#define CM4_SRAM_ADDR_MAPPING(phyAdd) ((phyAdd)-AFE_SRAM_PHY_BASE+AFE_SRAM_CM4_MAP_BASE)

// ============================================================================
typedef enum {
    USND_TASK_BITS_PER_SAMPLE_16,
    USND_TASK_BITS_PER_SAMPLE_32,
}uSndTask_Bits_Per_Sample_t;

typedef enum {
    USND_TASK_DL_SR_96K,
    USND_TASK_DL_SR_192K,
}uSndTask_DL_SAMPLERATE_t;

typedef struct {
    uSndTask_Bits_Per_Sample_t bitPerSample;
    uSndTask_DL_SAMPLERATE_t dlSampleRate;

    // UL
    int16_t *ulDataBuf;
    uint32_t ulBufSize;  // samples. ulBufSize = ulFrameSize * 2
    uint32_t ulFrameSize;  // samples
    uint32_t ulBufWriteIdx;

    uint32_t ulSramFrameSizeInByte;  // bytes
    uint32_t ulSramRead;

    // DL SPH
    int16_t *mddlSphDataBuf;
    uint32_t mddlSphBufSize;  // unit is samples
    uint32_t mddlSphFrameSize;  // samples
    uint32_t mddlSphBufWriteIdx;

    uint32_t mddlSphSramFrameSizeInByte;  // bytes
    uint32_t mddlSphSramRead;

    uint32_t dlSramFrameSize;
    uint32_t dlSramFrameSizeInByte;
    // uint32_t dlSramRead;
    uint32_t dlSramWrite;

    uint32_t isFirstIrqCome;
    uint32_t irqTick;


    void (*ulHandler)(void *);
    void *ulHdlParam;

    void (*dlHandler)(void *);
    void *dlHdlParam;
} uSndInfo_t;

typedef struct uSndTask_msg_t {
    uint16_t msg_id;     /* defined by user */
    uint32_t param1;     /* see audio_ipi_msg_data_t */
    uint32_t param2;     /* see audio_ipi_msg_data_t */

    uint32_t payload0;
    uint32_t payload1;
    uint32_t payload2;
    uint32_t payload3;
    uint32_t payload4;
} uSndTask_msg_t;

typedef enum {
    AUDIO_TASK_USND_MSG_ID_OFF = 0x0,  // VOICE_ULTRA_DISABLE_ID
    AUDIO_TASK_USND_MSG_ID_ON = 0x1,  // VOICE_ULTRA_ENABLE_ID

    AUDIO_TASK_USND_MSG_ID_IRQ = 0x4444,
}uSndTask_msg_id_t;


static uSndInfo_t uSndInfo;
// ============================================================================

unsigned int start_time;

volatile unsigned int *uSnd_ITM_CONTROL = (unsigned int *)0xE0000E80;
volatile unsigned int *uSnd_DWT_CONTROL = (unsigned int *)0xE0001000;
volatile unsigned int *uSnd_DWT_CYCCNT = (unsigned int *)0xE0001004;
volatile unsigned int *uSnd_DEMCR = (unsigned int *)0xE000EDFC;

#if 0
#define CPU_RESET_CYCLECOUNTER() \
do { \
 *uSnd_DEMCR = *uSnd_DEMCR | 0x01000000; \
 *uSnd_DWT_CYCCNT = 0; \
 *uSnd_DWT_CONTROL = *uSnd_DWT_CONTROL | 1 ; \
 } while (0)

// Test Method
// CPU_RESET_CYCLECOUNTER();
// start_time = *uSnd_DWT_CYCCNT;
#endif

void task_ultrasound_setDlHandler(void (*dlHandler)(void *), void *dlHdlParam) {
    uSndInfo.dlHandler = dlHandler;
    uSndInfo.dlHdlParam = dlHdlParam;
}

void task_ultrasound_setUlHandler(void (*ulHandler)(void *), void *ulHdlParam) {
    uSndInfo.ulHandler = ulHandler;
    uSndInfo.ulHdlParam = ulHdlParam;
}

uint32_t task_ultrasound_getMddlSphSampleNumberPerFrame(void) {
    AudioTask *task = get_task_by_scene(TASK_SCENE_VOICE_ULTRASOUND);

    if ( AUDIO_TASK_INIT != task->state && AUDIO_TASK_WORKING != task->state ) {
        AUD_LOG_E("%s(), get DlSph without task running!", __FUNCTION__);
        return 0;
    }

    return uSndInfo.mddlSphFrameSize;
}

int16_t *task_ultrasound_getMddlSphData() {
    AudioTask *task = get_task_by_scene(TASK_SCENE_VOICE_ULTRASOUND);

    if ( AUDIO_TASK_INIT != task->state && AUDIO_TASK_WORKING != task->state ) {
        AUD_LOG_E("%s(), get DlSph without task running!", __FUNCTION__);
        return 0;
    }

    return uSndInfo.mddlSphDataBuf;
}

uint32_t task_ultrasound_getDlSampleNumberPerFrame() {
    AudioTask *task = get_task_by_scene(TASK_SCENE_VOICE_ULTRASOUND);

    if ( AUDIO_TASK_INIT != task->state && AUDIO_TASK_WORKING != task->state ) {
        AUD_LOG_E("%s(), write DL data without task running!", __FUNCTION__);
        return 0;
    }

    return uSndInfo.dlSramFrameSize;
}

/**
	@dlData: PCM data array
	@len: number of samples. for 96k, should be 960.
*/
void task_ultrasound_writeDlData(int16_t *dlData, uint32_t len) {
    uint32_t lenInByte;
    uint32_t sramBase;
    uint32_t sramEnd;
    // uint32_t sramCur;

    AudioTask *task = get_task_by_scene(TASK_SCENE_VOICE_ULTRASOUND);
    if ( AUDIO_TASK_INIT != task->state && AUDIO_TASK_WORKING != task->state ) {
        AUD_LOG_E("%s(), write DL data without task running!", __func__);
        return;
    }

    sramBase = drv_reg32(AFE_DL3_BASE);
    sramEnd = drv_reg32(AFE_DL3_END)+1;
    // sramCur = drv_reg32(AFE_DL3_CUR);

    if (0 == sramBase || 1 == sramEnd) {
        AUD_LOG_W("%s(), return due to no hardware setting!", __func__);
        return;
    }

    // AUD_LOG_D("uSndInfo.dlSramWrite=0x%x, sramCur=0x%x, sramBase=0x%x, sramEnd=0x%x, ",
    // uSndInfo.dlSramWrite, sramCur, sramBase, sramEnd);
    if ( USND_TASK_BITS_PER_SAMPLE_16 == uSndInfo.bitPerSample ) {
        uint32_t i;
        lenInByte = len*sizeof(int16_t);
        AUD_ASSERT(lenInByte == uSndInfo.dlSramFrameSizeInByte);


        for ( i = 0; i < len; i++ ) {
            *((int16_t *)(CM4_SRAM_ADDR_MAPPING(uSndInfo.dlSramWrite))) = dlData[i];

            uSndInfo.dlSramWrite = uSndInfo.dlSramWrite + sizeof(int16_t);
            if ( uSndInfo.dlSramWrite >= sramEnd ) {
                uSndInfo.dlSramWrite = uSndInfo.dlSramWrite-sramEnd+sramBase;
            }
        }

    } else {  // 24bit sample
        uint32_t i;
        AUD_ASSERT(len*sizeof(uint32_t) == uSndInfo.dlSramFrameSizeInByte);

        for ( i = 0; i < len; i++ ) {
            *((uint32_t *)(CM4_SRAM_ADDR_MAPPING(uSndInfo.dlSramWrite))) = ((((uint32_t)(dlData[i])) << 8)&0x00ffffff);

            uSndInfo.dlSramWrite = uSndInfo.dlSramWrite + sizeof(uint32_t);
            if (uSndInfo.dlSramWrite >= sramEnd) {
                uSndInfo.dlSramWrite = uSndInfo.dlSramWrite-sramEnd+sramBase;
            }
        }
    }

    // AUD_LOG_D("write done!! dlCur=0x%x, uSndInfo.dlSramWrite=0x%x", (drv_reg32(AFE_DL3_CUR)), uSndInfo.dlSramWrite);
}

uint32_t task_ultrasound_getUlSampleNumberPerFrame(void) {
    AudioTask *task = get_task_by_scene(TASK_SCENE_VOICE_ULTRASOUND);

    if ( AUDIO_TASK_INIT != task->state && AUDIO_TASK_WORKING != task->state ) {
        AUD_LOG_E("%s(), get DlSph without task running!", __FUNCTION__);
        return 0;
    }

    return uSndInfo.ulFrameSize;
}

int16_t *task_ultrasound_getUlData() {
    AudioTask *task = get_task_by_scene(TASK_SCENE_VOICE_ULTRASOUND);

    if ( AUDIO_TASK_INIT != task->state && AUDIO_TASK_WORKING != task->state ) {
        AUD_LOG_E("%s(), get DlSph without task running!", __FUNCTION__);
        return 0;
    }

    return uSndInfo.ulDataBuf;
}

// ============================================================================

static void task_ultrasound_proximity_start(AudioTask *this,
    uSndTask_Bits_Per_Sample_t bitsPerSample,
    uSndTask_DL_SAMPLERATE_t dlSampleRate) {
    // uSndServCmd_T ackCmd;
    uint32_t irqSr;
    uint32_t irqCn;

    if (AUDIO_TASK_IDLE != this->state) {
        AUD_LOG_E("%s Unexpected start with status=%d, %d, %d,", __FUNCTION__, this->state);
        return;
    }

    AUD_LOG_D("%s() ===== %d, %d", __FUNCTION__, bitsPerSample, dlSampleRate);

    irqSr = ((drv_reg32(AFE_IRQ_MCU_CON))>>20)&0xF;
    irqCn = drv_reg32(AFE_IRQ_MCU_CNT4);
    switch (irqSr) {
        case 0: irqSr = 8000; break;
        case 1: irqSr = 11025; break;
        case 2: irqSr = 12000; break;

        case 4: irqSr = 16000; break;
        case 5: irqSr = 22050; break;
        case 6: irqSr = 24000; break;
        case 7: irqSr = 130000; break;
        case 8: irqSr = 32000; break;
        case 9: irqSr = 44100; break;
        case 10: irqSr = 48000; break;
        case 11: irqSr = 88200; break;
        case 12: irqSr = 96000; break;
        case 13: irqSr = 176400; break;
        case 14: irqSr = 192000; break;
        case 15: irqSr = 260000; break;
    }
    uSndInfo.ulFrameSize = UL_SAMPLE_PER_S*irqCn/irqSr;
    uSndInfo.ulBufSize = (uSndInfo.ulFrameSize)*2;

    uSndInfo.ulDataBuf = kal_pvPortMalloc(uSndInfo.ulBufSize*sizeof(int16_t));
    uSndInfo.ulBufWriteIdx = 0;


    // uSndInfo.ulSramRead = drv_reg32(AFE_VUL_D2_BASE);


    if (uSndInfo.ulDataBuf == NULL) {
        AUD_LOG_E("%s(), kal_pvPortMalloc ulBufSize fail with size: %d", __func__, uSndInfo.ulBufSize);

        // reset
        uSndInfo.ulBufSize = 0;
        uSndInfo.ulFrameSize = 0;
        return;
    }


    uSndInfo.mddlSphFrameSize = DL_SPH_SAMPLE_PER_S*irqCn/irqSr;
    uSndInfo.mddlSphBufSize = (uSndInfo.mddlSphFrameSize);
    uSndInfo.mddlSphDataBuf = kal_pvPortMalloc(uSndInfo.mddlSphBufSize*sizeof(int16_t));
    uSndInfo.mddlSphBufWriteIdx = 0;

    if (uSndInfo.mddlSphDataBuf == NULL) {
        AUD_LOG_E("%s(), kal_pvPortMalloc mddlSphDataBuf fail with size: %d", __func__, uSndInfo.mddlSphBufSize);

        // reset
        uSndInfo.mddlSphBufSize = 0;
        uSndInfo.mddlSphFrameSize = 0;
        return;
    }

    uSndInfo.bitPerSample = bitsPerSample;
    uSndInfo.dlSampleRate = dlSampleRate;
    switch (bitsPerSample) {
        case USND_TASK_BITS_PER_SAMPLE_16:

            uSndInfo.ulSramFrameSizeInByte = (uSndInfo.ulFrameSize)*sizeof(int16_t);
            uSndInfo.mddlSphSramFrameSizeInByte = uSndInfo.mddlSphFrameSize*sizeof(int16_t);

            if (USND_TASK_DL_SR_96K == dlSampleRate) {
                uSndInfo.dlSramFrameSize = DL_SAMPLE_PER_S_96K*irqCn/irqSr;
                uSndInfo.dlSramFrameSizeInByte = uSndInfo.dlSramFrameSize*sizeof(int16_t);
            } else {  // USND_TASK_DL_SR_192K
                uSndInfo.dlSramFrameSize = DL_SAMPLE_PER_S_192K*irqCn/irqSr;
                uSndInfo.dlSramFrameSizeInByte = uSndInfo.dlSramFrameSize*sizeof(int16_t);
            }

            break;

        case USND_TASK_BITS_PER_SAMPLE_32:

            uSndInfo.ulSramFrameSizeInByte = (uSndInfo.ulFrameSize)*sizeof(uint32_t);
            uSndInfo.mddlSphSramFrameSizeInByte = uSndInfo.mddlSphFrameSize*sizeof(uint32_t);

            if (USND_TASK_DL_SR_96K == dlSampleRate) {
                uSndInfo.dlSramFrameSize = DL_SAMPLE_PER_S_96K*irqCn/irqSr;
                uSndInfo.dlSramFrameSizeInByte = uSndInfo.dlSramFrameSize*sizeof(uint32_t);
            } else {  // USND_TASK_DL_SR_192K
                uSndInfo.dlSramFrameSize = DL_SAMPLE_PER_S_192K*irqCn/irqSr;
                uSndInfo.dlSramFrameSizeInByte = uSndInfo.dlSramFrameSize*sizeof(uint32_t);
            }

            break;
        default:
            AUD_ASSERT(0);
    }

    uSndInfo.isFirstIrqCome = 0;
    uSndInfo.irqTick = 0;

    this->state = AUDIO_TASK_INIT;

    #if 0
    ackCmd.ipi_msgid = USND_TASK_MSGID_ON_DONE;
    if ( pdPASS != xQueueSend(cmdQueue, &ackCmd, 0) ) {
        AUD_LOG_E("%s(), send fail with ipi_msgid=%d", ackCmd.ipi_msgid);
    };
    #endif

    DRV_SetReg32(AFE_DAC_CON0, 0x260);  // trun on mem interface

    // CPU_RESET_CYCLECOUNTER();
    // start_time = *uSnd_DWT_CYCCNT;
}

static void task_ultrasound_proximity_stop(AudioTask *this) {
    // uSndServCmd_T ackCmd;

    if ( AUDIO_TASK_IDLE == this->state ) {
        return;
    }

    this->state = AUDIO_TASK_DEINIT;

    DRV_ClrReg32(AFE_DAC_CON0, 0x260);

    vPortFree(uSndInfo.ulDataBuf);
    vPortFree(uSndInfo.mddlSphDataBuf);

    this->state = AUDIO_TASK_IDLE;

    #if 0
    ackCmd.ipi_msgid = USND_TASK_MSGID_OFF_DONE;
    if ( pdPASS != xQueueSend(cmdQueue, &ackCmd, 0) ) {
        AUD_LOG_E("%s(), senf fail with ipi_msgid=%d", ackCmd.ipi_msgid);
    }
    #endif
}

static void task_ultrasound_proximity_ulProc(void) {
    uint32_t i;
    uint32_t ulReadNow = drv_reg32(AFE_VUL_D2_CUR);
    uint32_t ulSramEnd = drv_reg32(AFE_VUL_D2_END);

    // expect data length is one frame
    if (ulReadNow > uSndInfo.ulSramRead) {
        // AUD_LOG_D("1, ulReadNow=0x%x, uSndInfo.ulSramRead=0x%x, len=0x%x",
        // ulReadNow, uSndInfo.ulSramRead, ulReadNow-uSndInfo.ulSramRead );

        AUD_ASSERT((ulReadNow-uSndInfo.ulSramRead) >= uSndInfo.ulSramFrameSizeInByte);
    } else {
        // AUD_LOG_D("2, ulReadNow=0x%x, uSndInfo.ulSramRead=0x%x, len=0x%x", ulReadNow, uSndInfo.ulSramRead,
        // ulReadNow-drv_reg32(AFE_VUL_D2_CUR)+drv_reg32(AFE_VUL_D2_END)-uSndInfo.ulSramRead );
        AUD_ASSERT((ulReadNow + uSndInfo.ulSramFrameSizeInByte - uSndInfo.ulSramRead) >= 0);
        // (end-write) + (read-begin) >= frame
    }

    // process data to buffer
    ulReadNow = uSndInfo.ulSramRead;
    for ( i = 0; i < uSndInfo.ulFrameSize; i++ ) {
        if (USND_TASK_BITS_PER_SAMPLE_32 == uSndInfo.bitPerSample) {
            uint32_t temp32Pmc = *(uint32_t *)(ulReadNow - AFE_SRAM_PHY_BASE + AFE_SRAM_CM4_MAP_BASE);
            uSndInfo.ulDataBuf[uSndInfo.ulBufWriteIdx] = (int16_t) ((temp32Pmc >> 6) & 0xffff);
            ulReadNow += (sizeof(uint32_t));
        } else {
            uint32_t temp16Pmc = *(uint16_t *)(ulReadNow - AFE_SRAM_PHY_BASE + AFE_SRAM_CM4_MAP_BASE);
            uSndInfo.ulDataBuf[uSndInfo.ulBufWriteIdx] = (int16_t) ((temp16Pmc&0x3fff) << 2);
            ulReadNow += (sizeof(int16_t));
        }

        if (ulReadNow > ulSramEnd) {
            ulReadNow = drv_reg32(AFE_VUL_D2_BASE);  // ulReadNow - ulSramEnd - 1 + drv_reg32(AFE_VUL_D2_BASE);
        }

        uSndInfo.ulBufWriteIdx++;
        if ((uSndInfo.ulBufWriteIdx) >= uSndInfo.ulBufSize) {
            uSndInfo.ulBufWriteIdx = uSndInfo.ulBufWriteIdx - uSndInfo.ulBufSize;
        }
    }
    uSndInfo.ulSramRead = ulReadNow;

    if (NULL != uSndInfo.ulHandler) {
        uSndInfo.ulHandler(uSndInfo.ulHdlParam);
    }
}


static void task_ultrasound_proximity_dlSphProc(void) {
    uint32_t i;

    uint32_t dlSphReadNow = drv_reg32(AFE_AWB_CUR);
    uint32_t dlSphSramEnd = drv_reg32(AFE_AWB_END);

    if (dlSphReadNow > uSndInfo.mddlSphSramRead) {
        // AUD_LOG_D("1, dlSphReadNow=0x%x, uSndInfo.mddlSphSramRead=0x%x, len=0x%x",
        // dlSphReadNow, uSndInfo.mddlSphSramRead, dlSphReadNow-uSndInfo.mddlSphSramRead );

        AUD_ASSERT((dlSphReadNow-uSndInfo.mddlSphSramRead) >= uSndInfo.mddlSphSramFrameSizeInByte);
    } else {
        // AUD_LOG_D("2, dlSphReadNow=0x%x, uSndInfo.mddlSphSramRead=0x%x, len=0x%x"
        // , dlSphReadNow, uSndInfo.mddlSphSramRead,
        // dlSphReadNow-drv_reg32(AFE_AWB_BASE)+drv_reg32(AFE_AWB_END)-uSndInfo.mddlSphSramRead );

        AUD_ASSERT((dlSphReadNow + uSndInfo.mddlSphSramFrameSizeInByte - uSndInfo.mddlSphSramRead) >= 0);
        // (end-write) + (read-begin) >= frame
    }

    // process data to buffer
    dlSphReadNow = uSndInfo.mddlSphSramRead;
    for ( i = 0; i < uSndInfo.mddlSphFrameSize; i++ ) {
        if ( USND_TASK_BITS_PER_SAMPLE_32 == uSndInfo.bitPerSample ) {  // 4 bytes per sample
            uint32_t temp32Pmc = *(uint32_t *)(dlSphReadNow - AFE_SRAM_PHY_BASE + AFE_SRAM_CM4_MAP_BASE);
            uSndInfo.mddlSphDataBuf[uSndInfo.ulBufWriteIdx] = (int16_t) ((temp32Pmc >> 8) & 0xffff);
            dlSphReadNow += (sizeof(uint32_t));  // 4 bytes per sample
        } else {
            uSndInfo.mddlSphDataBuf[uSndInfo.mddlSphBufWriteIdx]
                = *(int16_t *)(dlSphReadNow - AFE_SRAM_PHY_BASE + AFE_SRAM_CM4_MAP_BASE);
            dlSphReadNow += (sizeof(int16_t));  // 2 bytes per sample
        }


        if (dlSphReadNow > dlSphSramEnd) {
            dlSphReadNow = drv_reg32(AFE_AWB_BASE);  // dlSphReadNow - dlSphSramEnd - 1 + drv_reg32(AFE_AWB_BASE);
        }


        uSndInfo.mddlSphBufWriteIdx++;
        if ((uSndInfo.mddlSphBufWriteIdx) >= uSndInfo.mddlSphBufSize) {
            uSndInfo.mddlSphBufWriteIdx = uSndInfo.mddlSphBufWriteIdx - uSndInfo.mddlSphBufSize;
        }
    }

    uSndInfo.mddlSphSramRead = dlSphReadNow;
}


static void task_ultrasound_proximity_dlProc(void) {
    if (NULL != uSndInfo.dlHandler) {
        uSndInfo.dlHandler(uSndInfo.dlHdlParam);
    }
}

/**
	The task like HISR.
	Please do data moving inside the task.
*/
static void task_ultrasound_proximity_task_loop(void *pvParameters) {
    AudioTask *this = (AudioTask *)pvParameters;
    uSndTask_msg_t uSndMsg;

    while (1) {
        xQueueReceive(this->msg_idx_queue, &uSndMsg, portMAX_DELAY);

        switch (uSndMsg.msg_id) {
            case AUDIO_TASK_USND_MSG_ID_ON:
                AUD_LOG_D("%s(), uSndMsg=0x%x, 0x%x, 0x%x, %d, %d, %d, %d, %d",
                            __func__, uSndMsg.msg_id, uSndMsg.param1, uSndMsg.param2,
                            uSndMsg.payload0, uSndMsg.payload1, uSndMsg.payload2, uSndMsg.payload3, uSndMsg.payload4);

                AUD_ASSERT(16000 == uSndMsg.payload0);
                AUD_ASSERT(260000 == uSndMsg.payload2);

                if ( 96000 == uSndMsg.payload1 ) {
                    if ( 2 == uSndMsg.payload3 ) {
                        task_ultrasound_proximity_start(this, USND_TASK_BITS_PER_SAMPLE_16, USND_TASK_DL_SR_96K);
                    } else if ( 4 == uSndMsg.payload3 ) {
                        task_ultrasound_proximity_start(this, USND_TASK_BITS_PER_SAMPLE_32, USND_TASK_DL_SR_96K);
                    } else {
                        AUD_ASSERT(0);
                    }
                } else if ( 192000 == uSndMsg.payload1 ) {
                    if ( 2 == uSndMsg.payload3 ) {
                        task_ultrasound_proximity_start(this, USND_TASK_BITS_PER_SAMPLE_16, USND_TASK_DL_SR_192K);
                    } else if ( 4 == uSndMsg.payload3 ) {
                        task_ultrasound_proximity_start(this, USND_TASK_BITS_PER_SAMPLE_32, USND_TASK_DL_SR_192K);
                    } else {
                        AUD_ASSERT(0);
                    }
                } else {
                    AUD_ASSERT(0);  // unexpect downlink sampling rate
                }

                break;
            case AUDIO_TASK_USND_MSG_ID_OFF:
                task_ultrasound_proximity_stop(this);
                break;
            case AUDIO_TASK_USND_MSG_ID_IRQ:
                {
                // uint32_t end_time;
                // end_time= *uSnd_DWT_CYCCNT;
                // AUD_LOG_D("%s(), end_time-start_time = %d", __FUNCTION__, (end_time-start_time));

                // -- CPU_RESET_CYCLECOUNTER();
                // start_time = end_time; //*uSnd_DWT_CYCCNT;

                if (AUDIO_TASK_INIT == this->state) {
                    uSndInfo.ulSramRead = drv_reg32(AFE_VUL_D2_CUR);
                    uSndInfo.mddlSphSramRead = drv_reg32(AFE_AWB_CUR);

                    uSndInfo.dlSramWrite = drv_reg32(AFE_DL3_CUR)+((uSndInfo.dlSramFrameSizeInByte*3)>>1);
                    if (uSndInfo.dlSramWrite > drv_reg32(AFE_DL3_END)) {
                        uSndInfo.dlSramWrite = uSndInfo.dlSramWrite-drv_reg32(AFE_DL3_END)-1+drv_reg32(AFE_DL3_BASE);
                    }

                    AUD_LOG_D(
                     "ulSramRead=0x%x, 0x%x, mddlSphSramRead=0x%x, mddlSphSramFrameSizeInByte=0x%x, dlSramWrite=0x%x",
                        uSndInfo.ulSramRead, uSndInfo.ulSramFrameSizeInByte,
                        uSndInfo.mddlSphSramRead, uSndInfo.mddlSphSramFrameSizeInByte, uSndInfo.dlSramWrite);

                    if ( uSndInfo.isFirstIrqCome > 0 ) {
                        this->state = AUDIO_TASK_WORKING;
                    }
                } else if (AUDIO_TASK_WORKING == this->state) {
                        task_ultrasound_proximity_ulProc();
                        task_ultrasound_proximity_dlSphProc();
                        task_ultrasound_proximity_dlProc();
                }
                }
                break;
            default:
                AUD_LOG_I("unknown uSndMsg=0x%x, 0x%x, 0x%x, 0x%x", uSndMsg.msg_id, uSndMsg.param1, uSndMsg.param2);
        }
    }
}


// ============================================================================

static void task_ultrasound_proximity_constructor(struct AudioTask *this) {
    AUD_ASSERT(this != NULL);

    /* assign initial value for class members & alloc private memory here */
    this->scene = TASK_SCENE_VOICE_ULTRASOUND;
    this->state = AUDIO_TASK_IDLE;

    /* queue */
    this->queue_idx = 0;

    this->msg_idx_queue = xQueueCreate(MAX_MSG_QUEUE_SIZE, sizeof(uSndTask_msg_t));
    AUD_ASSERT(this->msg_idx_queue != NULL);
}

static void task_ultrasound_proximity_destructor(struct AudioTask *this) {
    AUD_LOG_D("%s(), task_scene = %d", __func__, this->scene);

    /* dealloc private memory & dealloc object here */
    AUD_ASSERT(this != NULL);

    // vPortFree(this->msg_array);
    vPortFree(this);
}

static void task_ultrasound_proximity_create_task_loop(struct AudioTask *this) {
    /* Note: you can also bypass this function,
             and do kal_xTaskCreate until you really need it.
             Ex: create task after you do get the enable message. */

    BaseType_t xReturn = pdFAIL;

    xReturn = kal_xTaskCreate(
                  task_ultrasound_proximity_task_loop,
                  LOCAL_TASK_NAME,
                  LOCAL_TASK_STACK_SIZE,
                  (void *)this,
                  LOCAL_TASK_PRIORITY,
                  NULL);

    AUD_ASSERT(xReturn == pdPASS);
}

static audio_status_t task_ultrasound_proximity_recv_message(
    struct AudioTask *this,
    struct ipi_msg_t *ipi_msg) {
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    uSndTask_msg_t uSndMsg;
    uint32_t *tempPayload = (uint32_t *)(ipi_msg->payload);

    uSndMsg.msg_id = ipi_msg->msg_id;
    uSndMsg.param1 = ipi_msg->param1;
    uSndMsg.param2 = ipi_msg->param2;
    uSndMsg.payload0 = *tempPayload;
    uSndMsg.payload1 = *(tempPayload+1);
    uSndMsg.payload2 = *(tempPayload+2);
    uSndMsg.payload3 = *(tempPayload+3);
    uSndMsg.payload4 = *(tempPayload+4);

    xQueueSendToBackFromISR(
        this->msg_idx_queue,
        &uSndMsg,
        &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    return NO_ERROR;
}

static void task_ultrasound_proximity_irq_hanlder(
    struct AudioTask *this,
    uint32_t irq_type) {
    if (irq_type == AUDIO_IRQn) {
        static portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

        uSndTask_msg_t uSndMsg;
        int IRQSource;

        IRQSource = DRV_Reg32(AFE_IRQ_MCU_STATUS);
#if 0
        if ( (IRQSource & 0xff) == 0 ) {
            data = DRV_Reg32(AFE_IRQ_MCU_CLR);
            DRV_WriteReg32(AFE_IRQ_MCU_CLR,
                           data); /*Clears the MCU IRQ for AFE while all IRQ statuses are 0*/
        }
#endif
        // check IRQ 4
        if ( IRQSource & IRQ4_SOURCE ) {
            DRV_WriteReg32(AFE_IRQ_MCU_CLR, (IRQSource & 0xff));

            uSndMsg.msg_id = AUDIO_TASK_USND_MSG_ID_IRQ;

            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            if ( AUDIO_TASK_INIT == this->state || AUDIO_TASK_WORKING == this->state ) {
                if ( AUDIO_TASK_INIT == this->state ) {
                    uSndInfo.isFirstIrqCome++;
                }
                uSndInfo.irqTick++;

                // send message to task
                if (xQueueSendFromISR(this->msg_idx_queue, &uSndMsg,
                                      &xHigherPriorityTaskWoken) != pdTRUE) {
                    return;
                }

                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}


AudioTask *task_ultrasound_proximity_new() {
    /* alloc object here */
    AudioTask *task = (AudioTask *)kal_pvPortMalloc(sizeof(AudioTask));
    if (task == NULL) {
        AUD_LOG_E("%s(), kal_pvPortMalloc fail!!\n", __func__);
        return NULL;
    }

    /* only assign methods, but not class members here */
    task->constructor       = task_ultrasound_proximity_constructor;
    task->destructor        = task_ultrasound_proximity_destructor;

    task->create_task_loop  = task_ultrasound_proximity_create_task_loop;

    task->recv_message      = task_ultrasound_proximity_recv_message;

    task->irq_hanlder       = task_ultrasound_proximity_irq_hanlder;


    return task;
}

