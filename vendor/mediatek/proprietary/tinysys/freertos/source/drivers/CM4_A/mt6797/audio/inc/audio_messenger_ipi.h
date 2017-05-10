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

#ifndef AUDIO_MESSENGER_IPI_H
#define AUDIO_MESSENGER_IPI_H

#include "audio_type.h"
#include "arsi_api.h"

#define IPI_MSG_HEADER_SIZE      (16)
#define MAX_IPI_MSG_PAYLOAD_SIZE (32)
#define MAX_IPI_MSG_BUF_SIZE     (IPI_MSG_HEADER_SIZE + MAX_IPI_MSG_PAYLOAD_SIZE)

#define IPI_MSG_MAGIC_NUMBER     (0x8888)


typedef enum {
    AUDIO_IPI_LAYER_HAL,     /* HAL    <-> SCP */
    AUDIO_IPI_LAYER_KERNEL,  /* kernel <-> SCP */
    AUDIO_IPI_LAYER_MODEM,   /* MODEM  <-> SCP */
} audio_ipi_msg_layer_t;


typedef enum {
    AUDIO_IPI_MSG_ONLY, /* param1: defined by user,       param2: defined by user */
    AUDIO_IPI_PAYLOAD,  /* param1: payload length (<=32), param2: defined by user */
    AUDIO_IPI_DMA,      /* param1: dma data length,       param2: defined by user */
} audio_ipi_msg_data_t;


typedef enum {
    AUDIO_IPI_MSG_BYPASS_ACK = 0,
    AUDIO_IPI_MSG_NEED_ACK   = 1,
    AUDIO_IPI_MSG_ACK_BACK   = 2,
    AUDIO_IPI_MSG_CANCELED   = 8
} audio_ipi_msg_ack_t;


typedef struct ipi_msg_t {
    uint16_t magic;      /* IPI_MSG_MAGIC_NUMBER */
    uint8_t  task_scene; /* see task_scene_t */
    uint8_t  msg_layer;  /* see audio_ipi_msg_layer_t */
    uint8_t  data_type;  /* see audio_ipi_msg_data_t */
    uint8_t  ack_type;   /* see audio_ipi_msg_ack_t */
    uint16_t msg_id;     /* defined by user */
    uint32_t param1;     /* see audio_ipi_msg_data_t */
    uint32_t param2;     /* see audio_ipi_msg_data_t */
    union {
        char payload[MAX_IPI_MSG_PAYLOAD_SIZE];
        char *dma_addr;  /* for AUDIO_IPI_DMA only */
    };
} ipi_msg_t;



/*==============================================================================
 *                     public functions - declaration
 *============================================================================*/

void audio_messenger_ipi_init(void);

void audio_send_ipi_msg(
    task_scene_t task_scene,
    audio_ipi_msg_data_t data_type,
    audio_ipi_msg_ack_t ack_type,
    uint16_t msg_id,
    uint32_t param1,
    uint32_t param2,
    char    *payload);

void audio_send_ipi_msg_ack_back(const ipi_msg_t *ipi_msg);

#endif /* end of AUDIO_MESSENGER_IPI_H */

