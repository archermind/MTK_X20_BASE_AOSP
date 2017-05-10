#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>

#include "sensor_manager_fw.h"
#include "sensor_manager.h"

#define SM_DEBUG
#ifdef SM_DEBUG
#define SM_TAG                  "[SensorTest]"
#define SM_ERR(fmt, args...)    printf(SM_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define SM_LOG(fmt, args...)    printf(SM_TAG fmt, ##args)
#else
#define SM_ERR(fmt, args...)
#define SM_LOG(fmt, args...)
#endif

#define mainCHECK_DELAY	( ( portTickType) 1000 / portTICK_RATE_MS )

static int handle = -1;


void xSCPTestonSensorChanged(int sensor, struct data_unit_t values)
{
    //SM_LOG( "INFO: xSCPTestonSensorChanged run 1: sensor(%d), data[%d, %d, %d]!!\n\r", sensor, values.pedometer_t.accumulated_step_count, values.pedometer_t.accumulated_step_length, values.pedometer_t.step_frequency, values.pedometer_t.step_length);
    //SM_LOG( "INFO: xSCPTestonSensorChanged run 1: sensor(%d), data[%d, %d, %d]!!\n\r", sensor, values.pedometer_t.accumulated_step_count, values.pedometer_t.accumulated_step_length, values.pedometer_t.step_frequency, values.pedometer_t.step_length);
    SM_LOG( "INFO: xSCPTestonSensorChanged run 1: sensor(%d), data[%d, %d, %d]!!\n\r", sensor, values.gyroscope_t.azimuth, values.gyroscope_t.pitch, values.gyroscope_t.roll);
    //SM_LOG( "INFO: xSCPTestonSensorChanged run 1: sensor(%d), data[%d, %d, %d]!!\n\r", sensor, values.pdr_event.x, values.pdr_event.y, values.pdr_event.z);

    return;
}

void xSCPTestonAccuracyChanged(int sensor, int accuracy)
{
    SM_LOG( "INFO: xSCPTestonAccuracyChanged run 1!!\n\r");

    return;
}

#define ENABLE_ACCEL 0
#define ENABLE_PEDO 0
#define ENABLE_PDR 0
#define ENABLE_GYRO 0
#define ENABLE_MAG 0


#define ENABLE_ACCEL_WITHDISABLE 0
#define ENABLE_PEDO_WITHDISABLE 0
#define ENABLE_PDR_WITHDISABLE 0
#define ENABLE_GYRO_WITHDISABLE 0
#define ENABLE_MAG_WITHDISABLE 0

#if ENABLE_PEDO
static unsigned int is_pedo_enabled = 0;
#endif

#if ENABLE_ACCEL
static unsigned int is_acc_enabled = 0;
#endif

#if ENABLE_GYRO
static unsigned int is_gyro_enabled = 0;
#endif


#if ENABLE_MAG
static unsigned int is_mag_enabled = 0;
#endif


#if ENABLE_PDR
static unsigned int is_pdr_enabled = 0;
#endif

void xSensorFrameworkTestEntry(void *arg)
{

    struct CM4TaskRequestStruct testReq;

    portTickType xLastExecutionTime, xDelayTime;

    xLastExecutionTime = xTaskGetTickCount();
    xDelayTime = mainCHECK_DELAY*10;

    SM_LOG( "INFO: xSensorFrameworkTestEntry run 0(%p)!!\n\r", xSMINITSemaphore);
    xSemaphoreTake(xSMINITSemaphore, portMAX_DELAY);
    while(1) {
        vTaskDelayUntil( &xLastExecutionTime, xDelayTime );
        xLastExecutionTime = xTaskGetTickCount();

        SM_LOG( "INFO: xSensorFrameworkTestEntry run 1!!\n\r");

#if ENABLE_MAG
        if(!is_mag_enabled) {
            SM_LOG( "INFO: xSensorFrameworkTestEntry run 2!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_MAGNETIC_FIELD;
            testReq.command = SENSOR_SETDELAY_CMD;
            testReq.value = 5;

            SCP_Sensor_Manager_control(&testReq);

            SM_LOG( "INFO: xSensorFrameworkTestEntry run 3!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_MAGNETIC_FIELD;
            testReq.command = SENSOR_ACTIVATE_CMD;
            testReq.value = 1;

            SCP_Sensor_Manager_control(&testReq);

            is_mag_enabled = 1;

        } else {
#if ENABLE_MAG_WITHDISABLE
            SM_LOG( "INFO: xSensorFrameworkTestEntry run 4!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_MAGNETIC_FIELD;
            testReq.command = SENSOR_ACTIVATE_CMD;
            testReq.value = 0;

            SCP_Sensor_Manager_control(&testReq);

            is_mag_enabled = 0;
#endif
        }
#endif

#if ENABLE_ACCEL
        if(!is_acc_enabled) {
            SM_LOG( "INFO: xSensorFrameworkTestEntry run 2!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_ACCELEROMETER;
            testReq.command = SENSOR_SETDELAY_CMD;
            testReq.value = 100;

            SCP_Sensor_Manager_control(&testReq);

            SM_LOG( "INFO: xSensorFrameworkTestEntry run 3!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_ACCELEROMETER;
            testReq.command = SENSOR_ACTIVATE_CMD;
            testReq.value = 1;

            SCP_Sensor_Manager_control(&testReq);

            is_acc_enabled = 1;

        } else {

#if ENABLE_ACCEL_WITHDISABLE
            SM_LOG( "INFO: xSensorFrameworkTestEntry run 4!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_ACCELEROMETER;
            testReq.command = SENSOR_ACTIVATE_CMD;
            testReq.value = 0;

            SCP_Sensor_Manager_control(&testReq);

            is_acc_enabled = 0;
#endif
        }
#endif

#if ENABLE_GYRO
        if(!is_gyro_enabled) {
            SM_LOG( "INFO: xSensorFrameworkTestEntry run 2!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_GYROSCOPE;
            testReq.command = SENSOR_SETDELAY_CMD;
            testReq.value = 20;

            SCP_Sensor_Manager_control(&testReq);

            SM_LOG( "INFO: xSensorFrameworkTestEntry run 3!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_GYROSCOPE;
            testReq.command = SENSOR_ACTIVATE_CMD;
            testReq.value = 1;

            SCP_Sensor_Manager_control(&testReq);

            is_gyro_enabled = 1;

        } else {

#if ENABLE_GYRO_WITHDISABLE
            SM_LOG( "INFO: xSensorFrameworkTestEntry run 4!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_GYROSCOPE;
            testReq.command = SENSOR_ACTIVATE_CMD;
            testReq.value = 0;

            SCP_Sensor_Manager_control(&testReq);

            is_gyro_enabled = 0;
#endif
        }
#endif


#if ENABLE_PEDO
        if(!is_pedo_enabled) {
            SM_LOG( "INFO: xSensorFrameworkTestEntry run 5!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_PEDOMETER;
            testReq.command = SENSOR_SETDELAY_CMD;
            testReq.value = 100;

            SCP_Sensor_Manager_control(&testReq);

            SM_LOG( "INFO: xSensorFrameworkTestEntry run 6!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_PEDOMETER;
            testReq.command = SENSOR_ACTIVATE_CMD;
            testReq.value = 1;

            SCP_Sensor_Manager_control(&testReq);

            is_pedo_enabled = 1;
        } else {

#if ENABLE_PEDO_WITHDISABLE
            SM_LOG( "INFO: xSensorFrameworkTestEntry run 7!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_PEDOMETER;
            testReq.command = SENSOR_ACTIVATE_CMD;
            testReq.value = 0;

            SCP_Sensor_Manager_control(&testReq);
            is_pedo_enabled = 0;
#endif
        }
#endif

#if ENABLE_PDR
        if(!is_pdr_enabled) {
            SM_LOG( "INFO: xSensorFrameworkTestEntry run 8!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_PDR;
            testReq.command = SENSOR_SETDELAY_CMD;
            testReq.value = 100;

            SCP_Sensor_Manager_control(&testReq);

            SM_LOG( "INFO: xSensorFrameworkTestEntry run 9!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_PDR;
            testReq.command = SENSOR_ACTIVATE_CMD;
            testReq.value = 1;

            SCP_Sensor_Manager_control(&testReq);

            is_pdr_enabled = 1;
        } else {
#if ENABLE_PDR_WITHDISABLE
            SM_LOG( "INFO: xSensorFrameworkTestEntry run 10!!\n\r");
            testReq.handle = handle;
            testReq.sensor_type = SENSOR_TYPE_PDR;
            testReq.command = SENSOR_ACTIVATE_CMD;
            testReq.value = 0;

            SCP_Sensor_Manager_control(&testReq);
            is_pdr_enabled = 0;
#endif
        }
#endif
        vTaskDelayUntil( &xLastExecutionTime, xDelayTime );
        xLastExecutionTime = xTaskGetTickCount();
    }
}


int xSensorFrameworkTestInit(void)
{
    BaseType_t ret;
    struct CM4TaskInformationStruct scp_test_task;

    SM_LOG( "INFO: xSensorFrameworkTestInit run 0!!\n\r");

    scp_test_task.onSensorChanged = xSCPTestonSensorChanged;
    scp_test_task.onAccuracyChanged = xSCPTestonAccuracyChanged;
    handle = SCP_Sensor_Manager_open(&scp_test_task); //register gTaskInformation[0] for ap

    SM_LOG( "INFO: xSensorFrameworkTestInit run 1!!\n\r");

    ret = xTaskCreate(xSensorFrameworkTestEntry,"ST", 130, ( void * ) NULL, 2, NULL);
    if (ret != pdPASS) {
        SM_ERR( "Error: Sensor Test task cannot be created(%d)!!\n\r", ret);
        return ret;
    }

    SM_LOG( "INFO: xSensorFrameworkTestInit run 2!!\n\r");

    return SM_SUCCESS;
}

