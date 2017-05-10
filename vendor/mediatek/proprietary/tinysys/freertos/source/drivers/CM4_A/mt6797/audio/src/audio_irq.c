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
#include "audio_irq.h"

#include <interrupt.h>

#include "audio_task_interface.h"
#include "audio_task_factory.h"

#ifdef CFG_MTK_AURISYS_PHONE_CALL_SUPPORT
#include "ccci.h"
#endif



/*==============================================================================
 *                     private functions - declaration
 *============================================================================*/

#ifdef CFG_MTK_AURISYS_PHONE_CALL_SUPPORT
static void modem_irq_hanlder_md1(void);
static void modem_irq_hanlder_md3(void *dummy);
#endif
#ifdef CFG_MTK_AUDIO_TUNNELING_SUPPORT
static void mp3_irq_hanlder(void);
#endif
#ifdef CFG_MTK_VOICE_ULTRASOUND_SUPPORT
static void ultrasound_irq_hanlder(void);
#endif

/*==============================================================================
 *                     public functions - implementation
 *============================================================================*/

void audio_irq_init(void)
{
#ifdef CFG_MTK_AURISYS_PHONE_CALL_SUPPORT
    AUD_LOG_V("%s()", __func__);
    request_irq(MD_IRQn, modem_irq_hanlder_md1, "MD");
    ccci_register_rx(MD_SYS3, CCCI_C2K_CH_AUDIO, modem_irq_hanlder_md3);
#endif
#ifdef CFG_MTK_AUDIO_TUNNELING_SUPPORT
    request_irq(AUDIO_IRQn, mp3_irq_hanlder, "MP3");
#endif
#ifdef CFG_MTK_VOICE_ULTRASOUND_SUPPORT
    request_irq(AUDIO_IRQn, ultrasound_irq_hanlder, "USND");
#endif
}


/*==============================================================================
 *                     private functions - implementation
 *============================================================================*/

#ifdef CFG_MTK_AURISYS_PHONE_CALL_SUPPORT
static void modem_irq_hanlder_md1(void)
{
    AudioTask *task = get_task_by_scene(TASK_SCENE_PHONE_CALL);
    if (task != NULL) {
        task->irq_hanlder(task, (uint32_t)MD_IRQn);
    }
}


static void modem_irq_hanlder_md3(void *dummy)
{
    AudioTask *task = get_task_by_scene(TASK_SCENE_PHONE_CALL);
    if (task != NULL) {
        task->irq_hanlder(task, (uint32_t)MD_IRQn);
    }
}
#endif
#ifdef CFG_MTK_AUDIO_TUNNELING_SUPPORT
static void mp3_irq_hanlder(void)
{
    AudioTask *task = NULL;
    task = get_task_by_scene(TASK_SCENE_PLAYBACK_MP3);
    if (task != NULL) {
        task->irq_hanlder(task, (uint32_t)AUDIO_IRQn);
    }
}
#endif

#ifdef CFG_MTK_VOICE_ULTRASOUND_SUPPORT
static void ultrasound_irq_hanlder(void) {
    AudioTask *task = NULL;
    task = get_task_by_scene(TASK_SCENE_VOICE_ULTRASOUND);
    if (task != NULL) {
        task->irq_hanlder(task, (uint32_t)AUDIO_IRQn);
    }
}
#endif

