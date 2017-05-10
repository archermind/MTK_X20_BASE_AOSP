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
#ifndef AUDIO_TASK_OFFLOAD_MP3_PARAMS_H
#define AUDIO_TASK_OFFLOAD_MP3_PARAMS_H

#include "mp3dec_common.h"
#include "mp3dec_exp.h"
#include "blisrc_exp.h"

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define DL3_ON      5

#define AFE_INTERNAL_SRAM_CM4_MAP_BASE (0xD0001000)
#define AFE_INTERNAL_SRAM_PHY_BASE  (0x11221000)

#define AFE_BASE                (0xD0000000)
#define AFE_DAC_CON0            (AFE_BASE + 0x0010)
#define AFE_DL3_CUR             (AFE_BASE + 0x0364)
#define AFE_IRQ_MCU_STATUS      (AFE_BASE + 0x03A4)
#define AFE_IRQ_MCU_CLR         (AFE_BASE + 0x03A8)

#ifndef DRV_WriteReg32
#define DRV_WriteReg32(addr,data)   ((*(volatile unsigned int *)(addr)) = (unsigned int)data)
#endif
#ifndef DRV_Reg32
#define DRV_Reg32(addr)             (*(volatile unsigned int *)(addr))
#endif
#define vSetVarBit(variable,bit) variable =   ((variable) | (1<<(bit)))
#define vResetVarBit(variable,bit) variable = ((variable) & (~(1<<(bit))))

typedef enum {
    MP3_NEEDDATA = 11,
    MP3_PCMCONSUMED = 12,
    MP3_DRAINDONE = 13,
} IPI_MSG_ID;

typedef enum {
    MP3_INIT = 0,
    MP3_RUN = 1,
    MP3_PAUSE = 2,
    MP3_CLOSE = 3,
    MP3_SETPRAM = 4,
    MP3_SETMEM = 5,
    MP3_SETWRITEBLOCK = 6,
    MP3_DRAIN = 7,
    MP3_VOLUME = 8,
    MP3_WRITEIDX = 9,
    MP3_TSTAMP = 10,
} IPI_RECEIVED_MP3;

typedef struct {
    uint32_t samplerate;
    uint32_t channels;
    uint32_t format;
    uint32_t volume[2];
    uint32_t u4DataRemained;
    uint8_t bIsDrain;
    uint8_t bIsPause;
    uint8_t bIsClose;
    uint8_t bgetTime;
} SCP_OFFLOAD_T;

typedef struct {
    void *pBufAddr;
    int u4BufferSize;
    char *pWriteIdx;//  Write Index.
    char *pReadIdx;   //  Read Index,update by scp
    int u4DataRemained;
    int consumedBS;
    unsigned char   bneedFill;
} DMA_BUFFER_T;



typedef struct {
    Mp3dec_handle *handle;
    int workingbuf_size1;
    int workingbuf_size2;
    int min_bs_size;
    int pcm_size;
    void *working_buf1;
    void *working_buf2;
    void *pcm_buf;
} mp3DecEngine;

typedef struct {

    Blisrc_Handle *mHandle;
    Blisrc_Param mBliParam;
    unsigned int  mInternalBufSize;
    unsigned int  mTempBufSize;
    void *mInternalBuf;
    void *mTempBuf;
} blisrcEngine;

int mp3_dma_transaction_wrap(uint32_t dst_addr, uint32_t src_addr,
                             uint32_t len, uint8_t IsDram);

#endif // end of AUDIO_TASK_OFFLOAD_MP3_PARAMS_H

