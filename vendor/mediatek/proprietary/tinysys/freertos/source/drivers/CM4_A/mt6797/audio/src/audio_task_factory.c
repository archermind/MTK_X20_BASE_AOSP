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
#include "audio_task_factory.h"

#include "audio_type.h"

#include "audio_task_interface.h"

#ifdef CFG_MTK_AURISYS_PHONE_CALL_SUPPORT
#include "audio_task_phone_call.h"
#endif
#ifdef CFG_MTK_AUDIO_TUNNELING_SUPPORT
#include "audio_task_offload_mp3.h"
#endif
#ifdef CFG_MTK_VOICE_ULTRASOUND_SUPPORT
#include "audio_task_ultrasound_proximity.h"
#endif

static AudioTask *g_task_array[TASK_SCENE_SIZE];


static AudioTask *create_audio_tasks(const task_scene_t task_scene);



static AudioTask *create_audio_tasks(const task_scene_t task_scene)
{
    AudioTask *task = NULL;

    /* alloc memory for task */
    switch (task_scene) {
#ifdef CFG_MTK_AURISYS_PHONE_CALL_SUPPORT
        case TASK_SCENE_PHONE_CALL: {
            task = task_phone_call_new();
            break;
        }
#endif
#ifdef CFG_VOW_SUPPORT
        case TASK_SCENE_VOW: {
            task = NULL; /* TODO: add vow class */
            break;
        }
#endif
#ifdef CFG_MTK_AUDIO_TUNNELING_SUPPORT
        case TASK_SCENE_PLAYBACK_MP3: {
            task = task_offload_mp3_new();
            break;
        }
#endif

#ifdef CFG_MTK_VOICE_ULTRASOUND_SUPPORT
        case TASK_SCENE_VOICE_ULTRASOUND: {
            task = task_ultrasound_proximity_new();
            break;
        }
#endif
        default: {
            AUD_LOG_V("%s(), not support %d fail!!\n", __func__, task_scene);
            task = NULL;
            break;
        }
    }

    if (task == NULL) {
        //AUD_LOG_D("%s(), task == NULL!! task_scene = %d, return\n", __func__, task_scene);
        return NULL;
    }

    /* constructor */
    task->constructor(task);

    return task;
}


void audio_task_factory_init()
{
    int task_index = 0;

    /* alloc tasks */
    for (task_index = 0; task_index < TASK_SCENE_SIZE; task_index++) {
        g_task_array[task_index] = create_audio_tasks(task_index);
    }

    /* create task thread loop */
    for (task_index = 0; task_index < TASK_SCENE_SIZE; task_index++) {
        if (g_task_array[task_index] != NULL &&
            g_task_array[task_index]->create_task_loop != NULL) {
            g_task_array[task_index]->create_task_loop(g_task_array[task_index]);
        }
    }
}


void audio_task_factory_deinit()
{
    int task_index = 0;

    /* free tasks */
    for (task_index = 0; task_index < TASK_SCENE_SIZE; task_index++) {
        if (g_task_array[task_index] != NULL &&
            g_task_array[task_index]->destructor != NULL) {
            g_task_array[task_index]->destructor(g_task_array[task_index]);
            g_task_array[task_index] = NULL;
        }
    }
}

AudioTask *get_task_by_scene(const task_scene_t task_scene)
{
    AudioTask *task;

    if (task_scene < 0 || task_scene >= TASK_SCENE_SIZE) {
        AUD_LOG_W("%s(), task_scene(%d) error\n", __func__, task_scene);
        return NULL;
    }

    task = g_task_array[task_scene];
    if (task == NULL) {
        AUD_LOG_W("%s(), g_task_array[%d] == NULL\n", __func__, task_scene);
        return NULL;
    }

    //AUD_LOG_D("%s(), task_scene(%d), task = %p\n", __func__, task_scene, task);
    return task;
}

