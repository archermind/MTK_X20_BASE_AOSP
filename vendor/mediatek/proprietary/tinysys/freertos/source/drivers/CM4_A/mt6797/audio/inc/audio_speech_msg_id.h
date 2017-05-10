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

#ifndef AUDIO_SPEECH_MSG_ID_H
#define AUDIO_SPEECH_MSG_ID_H

#define IPI_MSG_A2D_BASE (0xAD00)
#define IPI_MSG_D2A_BASE (0xDA00)

#define IPI_MSG_M2D_BASE (0x3D00)
#define IPI_MSG_D2M_BASE (0xD300)


/* NOTE: all ack behaviors are rely on audio_ipi_msg_ack_t */
enum ipi_msg_id_call_t {
    /*======================================================================
     *                             AP to OpenDSP
     *====================================================================*/
    /* volume, 0xAD0- */
    IPI_MSG_A2D_UL_GAIN = IPI_MSG_A2D_BASE + 0x00,
    IPI_MSG_A2D_DL_GAIN,

    /* device environment info, 0xAD1-  */
    IPI_MSG_A2D_TASK_CFG = IPI_MSG_A2D_BASE + 0x10,
    IPI_MSG_A2D_SPH_PARAM,

    /* function control, 0xAD2-*/
    IPI_MSG_A2D_SPH_ON = IPI_MSG_A2D_BASE + 0x20,
    IPI_MSG_A2D_TTY_ON,

    /* speech enhancement control, 0xAD3-*/
    IPI_MSG_A2D_UL_MUTE_ON = IPI_MSG_A2D_BASE + 0x30,
    IPI_MSG_A2D_DL_MUTE_ON,
    IPI_MSG_A2D_UL_ENHANCE_ON,
    IPI_MSG_A2D_DL_ENHANCE_ON,
    IPI_MSG_A2D_BT_NREC_ON,

    /* tuning tool, 0xAD4-*/
    IPI_MSG_A2D_SET_ADDR_VALUE = IPI_MSG_A2D_BASE + 0x40,
    IPI_MSG_A2D_GET_ADDR_VALUE,
    IPI_MSG_A2D_SET_KEY_VALUE,
    IPI_MSG_A2D_GET_KEY_VALUE,


    /* debug, 0xADA- */
    IPI_MSG_A2D_PCM_DUMP_ON = IPI_MSG_A2D_BASE + 0xA0,
    IPI_MSG_A2D_LIB_LOG_ON,


    /*======================================================================
     *                             OpenDSP to AP
     *====================================================================*/
    IPI_MSG_D2A_PCM_DUMP_DATA_NOTIFY = IPI_MSG_D2A_BASE + 0x00,


    /*======================================================================
     *                             Modem to OpenDSP
     *====================================================================*/
    /* call data handshake, 0x3D0- */
    IPI_MSG_M2D_CALL_DATA_READY = IPI_MSG_M2D_BASE + 0x00,

};



#endif // end of AUDIO_SPEECH_MSG_ID_H

