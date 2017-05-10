/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Header Files
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
#include <string.h>
#include <stdio.h>

// #include <mt_reg_base.h>

#include <stddef.h>
#include <platform.h>
#include <FreeRTOS.h>
#include <task.h>
#include <scp_ipi.h>

#include "uSnd_service.h"

#include "audio_task_interface.h"
#include "audio_task_ultrasound_proximity.h"



#define USND_E(fmt, args...)    PRINTF_E("[uSnd] ERR: "fmt, ##args)
#define USND_W(fmt, args...)    PRINTF_W("[uSnd] WARM: "fmt, ##args)
#define USND_L(fmt, args...)    PRINTF_I("[uSnd] LOG: "fmt, ##args)
#define USND_D(fmt, args...)    PRINTF_D("[uSnd] DBG: "fmt, ##args)


// QueueHandle_t xIpiSendQueue;

#define MAX_MSG_QUEUE_SIZE (8)



/***********************************************************************************
** uSnd_serv_task
************************************************************************************/

#if 0
typedef struct {
    int16_t *playbackData;
    uint32_t playbackDataLen;  // sample
} uSnd_serv_proc_t;

uSnd_serv_proc_t uSndProc;
#endif

static void uSnd_serv_task(void *pvParameters) {
    uSndServCmd_T ipiCmdInfo;
    while (1) {
        xQueueReceive(cmdQueue, &ipiCmdInfo, portMAX_DELAY);

        // USND_L("%s(), ipiCmdInfo.ipi_msgid=%d", __FUNCTION__, ipiCmdInfo.ipi_msgid);
        switch (ipiCmdInfo.ipi_msgid) {
            case USND_TASK_MSGID_ON_DONE:
            {
#if 0
                uSndProc.playbackDataLen = task_ultrasound_getDlSampleNumberPerFrame();
                uSndProc.playbackData = kal_pvPortMalloc(uSndProc.playbackDataLen*sizeof(int16_t));
                if ( NULL == uSndProc.playbackData ) {
                    USND_E("%s(), Cannet get memory for uSndProc.playbackData in USND_TASK_MSGID_ON_DONE", __func__);
                    configASSERT(0);
                }

                USND_D("uSndProc.playbackDataLen=%d", uSndProc.playbackDataLen);
#endif
            }
                break;
            case USND_TASK_MSGID_OFF_DONE:
            {
#if 0
                vPortFree(uSndProc.playbackData);
                uSndProc.playbackDataLen = 0;
#endif
            }
                break;
            case USND_TASK_MSGID_PROCESS:
            {
#if 0
                uint32_t i;
                uint32_t samples = task_ultrasound_getUlSampleNumberPerFrame();
                int16_t *ulData = task_ultrasound_getUlData();

                // do somthing
                for ( i = 0; i < uSndProc.playbackDataLen; i++ ) {
                    uSndProc.playbackData[i] = ulData[i];
                }

                task_ultrasound_writeDlData(uSndProc.playbackData, uSndProc.playbackDataLen);
#endif
            }
                break;
            case USND_TASK_MSGID_DLMIX:
            {
#if 0
                uint32_t i, j, r;
                uint32_t samples = task_ultrasound_getMddlSphSampleNumberPerFrame();
                int16_t *sphData = task_ultrasound_getMddlSphData();

                r = uSndProc.playbackDataLen/samples;
                for ( i = 0; i < (samples); i++ ) {
                    for ( j = 0; j < r; j++ ) {
                        uSndProc.playbackData[i*r+j] = sphData[i];
                    }
                }

                task_ultrasound_writeDlData(uSndProc.playbackData, uSndProc.playbackDataLen);
#endif
            }
                break;
            default:
                break;
        }
    }
}

void uSnd_Dl_Handler(void *param) {
    uSndServCmd_T msg;
    msg.ipi_msgid = USND_TASK_MSGID_DLMIX;

    if ( pdPASS != xQueueSend(cmdQueue, &msg, 0) ) {
        USND_E("%s, fail to send DL handler!", __FUNCTION__);
    }
}

void uSnd_Ul_Handler(void *param) {
    uSndServCmd_T msg;
    msg.ipi_msgid = USND_TASK_MSGID_PROCESS;

    if ( pdPASS != xQueueSend(cmdQueue, &msg, 0) ) {
        USND_E("%s, fail to send UL handler!", __FUNCTION__);
    }
}
/***********************************************************************************
** uSnd_ipi_callback - message handler from AP side
************************************************************************************/

void uSnd_Serv_callback(int id, void *data, unsigned int len) {
}


/***********************************************************************************
** uSnd_ipi_callback - message handler from AP side
************************************************************************************/
void uSnd_serv_init(void) {
    cmdQueue = xQueueCreate(MAX_MSG_QUEUE_SIZE, sizeof(uSndServCmd_T));

    if ( cmdQueue == NULL ) {
        USND_E("cmdQueue Create Fail");
        configASSERT(0);
    }

    kal_xTaskCreate(uSnd_serv_task, "uSS", 250, (void *)0, 3, NULL);

#if 0
    task_ultrasound_setDlHandler(uSnd_Dl_Handler, NULL);
    task_ultrasound_setUlHandler(uSnd_Ul_Handler, NULL);
#endif
}


