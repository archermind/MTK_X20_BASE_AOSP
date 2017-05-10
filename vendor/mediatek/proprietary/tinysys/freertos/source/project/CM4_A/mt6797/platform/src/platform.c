#include "main.h"
/*  Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include <tinysys_config.h>
#include <stdio.h>
#include <feature_manager.h>
#include "sensorhub.h"
#include "scp_ipi.h"
#include "interrupt.h"
#ifdef CFG_MTK_VOW_SUPPORT
#include "vow_service.h"
#endif

#ifdef CFG_MTK_VOICE_ULTRASOUND_SUPPORT
#include "uSnd_service.h"
#endif

#ifdef CFG_MTK_SENSOR_HUB_SUPPORT
#include "shf_main.h"
#endif

#ifdef CFG_AUDIO_SUPPORT
#include <audio.h>
#endif

#ifdef CFG_FLP_SUPPORT
#include "flp_service.h"
#endif

#ifdef TINYSYS_DEBUG_BUILD
static int task_monitor_log = 1;
static unsigned int ix = 0;
static void vTaskMonitor(void *pvParameters);
static char list_buffer[512];
#define mainCHECK_DELAY                                         ( ( portTickType) 60000 / portTICK_RATE_MS )
static void vTaskMonitor(void *pvParameters)
{
    portTickType xLastExecutionTime, xDelayTime;

    xLastExecutionTime = xTaskGetTickCount();
    xDelayTime = mainCHECK_DELAY;

    do {
        if (task_monitor_log == 1) {
            vTaskList(list_buffer);
            PRINTF_D("[%d]Heap: free size/total size:%d/%d\n", ix, xPortGetFreeHeapSize(), configTOTAL_HEAP_SIZE);
            PRINTF_D("Task Status:\n\r%s", list_buffer);
            PRINTF_D("max duration: %llu, limit: %llu\n\r", get_max_cs_duration_time(), get_max_cs_limit());
        }
        ix ++;
        vTaskDelayUntil(&xLastExecutionTime, xDelayTime);
    } while (1);
}
#endif
void enable_task_monitor_log(void)
{
#ifdef TINYSYS_DEBUG_BUILD
    task_monitor_log = 1;
#endif
}
void disable_task_monitor_log(void)
{
#ifdef TINYSYS_DEBUG_BUILD
    task_monitor_log = 0;
#endif
}
void platform_init(void)
{
    PRINTF_D("in %s\n\r", __func__);

#ifdef TINYSYS_DEBUG_BUILD
    xTaskCreate(vTaskMonitor, "TMon", 384, (void*)4, 0, NULL);
#endif
#ifdef CFG_SENSORHUB_SUPPORT
    xSensorManagerInit();
    xSensorFrameworkInit();
#endif
#ifdef CFG_MTK_SENSOR_HUB_SUPPORT
    xTaskCreate(shf_entry, "SHF", 500, NULL, 2, NULL);
#endif
#ifdef CFG_AUDIO_SUPPORT
    audio_init();
#endif
#ifdef CFG_MTK_VOW_SUPPORT
    vow_init();
#endif
#ifdef CFG_MTK_VOICE_ULTRASOUND_SUPPORT
    // uSnd_serv_init();
#endif
#ifdef CFG_FLP_SUPPORT
    FlpService_init();
#endif
    ipi_status_dump();
    irq_status_dump();
    feature_manager_init();

    return;
}
