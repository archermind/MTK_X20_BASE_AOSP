/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        Header Files
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
#include "vow_service.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <mt_reg_base.h>
#include <platform.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <scp_ipi.h>


QueueHandle_t xIpiSendQueue;

struct ipiCmd {
    unsigned int ipi_msgid;
    unsigned int ipi_ret;
    unsigned int para1;
    unsigned int para2;
    unsigned int para3;
    unsigned int para4;
};

static void vow_ipi_task(void *pvParameters);

/*
============================================================================================================
------------------------------------------------------------------------------------------------------------
||                        IPI Hander
------------------------------------------------------------------------------------------------------------
============================================================================================================
*/
void vow_ipi_init(void)
{
    scp_ipi_registration(IPI_VOW, vow_ipi_handler, "vow");
    scp_ipi_wakeup_ap_registration(IPI_VOW);
    //VOW_LOG("register vow ipi\n\r");

    xIpiSendQueue = xQueueCreate( 15, sizeof(struct ipiCmd));
    if (xIpiSendQueue == NULL) {
        // The semaphore was created failly.
        // The semaphore can not be used.
        VOW_DBG("Ipi_S Fail\n\r");
    }
    kal_xTaskCreate(vow_ipi_task, "ipi_send_task", 250, (void *)0, 3, NULL);
}

/***********************************************************************************
** vow_ipi_sendmsg - send message to AP side
************************************************************************************/
int vow_ipi_sendmsg(vow_ipi_msgid_t id, void *buf, vow_ipi_result result, unsigned int size, unsigned int wait)
{
    ipi_status status;
    short ipibuf[24];

    ipibuf[0] = id;
    ipibuf[1] = size;
    ipibuf[2] = result;

    if (size != 0) {
        memcpy((void *)&ipibuf[3], buf, size);
    }
    status = scp_ipi_send(IPI_VOW, (void*)ipibuf, size+6, wait, IPI_SCP2AP);
    //VOW_LOG("IPI Send:%x %x %x %x\n\r", status, id, size, result);
    if (status == BUSY) {
        //VOW_LOG("IPI Send F:%x %x %x %x\n\r", status, id, size, result);
        return false;
    } else if (status == ERROR) {
        VOW_DBG("IPI ERROR NEED CHECK\n\r");
        return -1;
    }

    return true;
}

/***********************************************************************************
** vow_ipi_handler - message handler from AP side
************************************************************************************/

void vow_ipi_handler(int id, void * data, unsigned int len)
{
    short *vowmsg;
    //short msglen;
    struct ipiCmd ipiCmdInfo;
    static BaseType_t xHigherPriorityTaskWoken;

    xHigherPriorityTaskWoken = pdFALSE;
    vowmsg = (short*)data;
    //msglen = vowmsg[1];
    //(void)msglen;
    switch (*vowmsg) {
        case AP_IPIMSG_VOW_ENABLE:
            //VOW_LOG("AP_IPIMSG_VOW_ENABLE\n\r");
            ipiCmdInfo.ipi_msgid = SCP_IPIMSG_VOW_ENABLE_ACK;
            ipiCmdInfo.ipi_ret   = 0;
            ipiCmdInfo.para1     = 0;
            ipiCmdInfo.para2     = 0;
            ipiCmdInfo.para3     = 0;
            ipiCmdInfo.para4     = 0;
            xQueueSendToBackFromISR(xIpiSendQueue, &ipiCmdInfo, &xHigherPriorityTaskWoken);
            break;
        case AP_IPIMSG_VOW_DISABLE:
            //VOW_LOG("AP_IPIMSG_VOW_DISABLE\n\r");
            ipiCmdInfo.ipi_msgid = SCP_IPIMSG_VOW_DISABLE_ACK;
            ipiCmdInfo.ipi_ret   = 0;
            ipiCmdInfo.para1     = 0;
            ipiCmdInfo.para2     = 0;
            ipiCmdInfo.para3     = 0;
            ipiCmdInfo.para4     = 0;
            xQueueSendToBackFromISR(xIpiSendQueue, &ipiCmdInfo, &xHigherPriorityTaskWoken);
            break;
        case AP_IPIMSG_VOW_SETMODE:
            //VOW_LOG("AP_IPIMSG_VOW_SETMODE\n\r");
            ipiCmdInfo.ipi_msgid = SCP_IPIMSG_VOW_SETMODE_ACK;
            ipiCmdInfo.ipi_ret   = 0;
            ipiCmdInfo.para1     = vowmsg[2];
            ipiCmdInfo.para2     = 0;
            ipiCmdInfo.para3     = 0;
            ipiCmdInfo.para4     = 0;
            xQueueSendToBackFromISR(xIpiSendQueue, &ipiCmdInfo, &xHigherPriorityTaskWoken);
            break;
        case AP_IPIMSG_VOW_APREGDATA_ADDR: {
            int *msg;
            //VOW_LOG("AP_IPIMSG_VOW_APREGDATA_ADDR\n\r");
            msg = (int*)vowmsg;
            msg++;
            //addr = *msg++;
            ipiCmdInfo.ipi_msgid = SCP_IPIMSG_VOW_APREGDATA_ADDR_ACK;
            ipiCmdInfo.ipi_ret   = 0;
            ipiCmdInfo.para1     = *msg++;
            ipiCmdInfo.para2     = 0;
            ipiCmdInfo.para3     = 0;
            ipiCmdInfo.para4     = 0;
            xQueueSendToBackFromISR(xIpiSendQueue, &ipiCmdInfo, &xHigherPriorityTaskWoken);
            break;
        }
        case AP_IPIMSG_SET_VOW_MODEL: {
            int *msg;
            //VOW_LOG("AP_IPIMSG_SET_VOW_MODEL\n\r");
            msg  = (int*)vowmsg;
            msg++;
            //type = *msg++;
            //id   = *msg++;
            //addr = *msg++;
            //size = *msg;
            ipiCmdInfo.ipi_msgid = SCP_IPIMSG_SET_VOW_MODEL_ACK;
            ipiCmdInfo.ipi_ret   = 0;
            ipiCmdInfo.para1     = *msg++;
            ipiCmdInfo.para2     = *msg++;
            ipiCmdInfo.para3     = *msg++;
            ipiCmdInfo.para4     = *msg++;
            xQueueSendToBackFromISR(xIpiSendQueue, &ipiCmdInfo, &xHigherPriorityTaskWoken);
            break;
        }
        case AP_IPIMSG_VOW_SET_FLAG: {
            int *msg;
            //VOW_LOG("AP_IPIMSG_VOW_SET_FLAG\n\r");
            msg = (int*)vowmsg;
            msg++;
            ipiCmdInfo.ipi_msgid = SCP_IPIMSG_SET_FLAG_ACK;
            ipiCmdInfo.ipi_ret   = 0;
            ipiCmdInfo.para1     = *msg++;
            ipiCmdInfo.para2     = *msg++;
            ipiCmdInfo.para3     = 0;
            ipiCmdInfo.para4     = 0;
            xQueueSendToBackFromISR(xIpiSendQueue, &ipiCmdInfo, &xHigherPriorityTaskWoken);
            break;
        }

        /*
        case AP_IPIMSG_VOW_SETGAIN:
        {
            vow_setGain(vowmsg[2]);
            vow_ipi_sendmsg(SCP_IPIMSG_VOW_SETGAIN_ACK, 0, VOW_IPI_SUCCESS, 0, 1);
            break;
        }

        */
        default:
            break;
    }
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/***********************************************************************************
** vow_ipi_task - Process VOW IPI send command
************************************************************************************/
static void vow_ipi_task(void *pvParameters)
{
    struct ipiCmd ipiCmdInfo;
    vow_ipi_result result;
    while (1) {
        xQueueReceive(xIpiSendQueue, &ipiCmdInfo, portMAX_DELAY);

        switch (ipiCmdInfo.ipi_msgid) {
            case SCP_IPIMSG_VOW_ENABLE_ACK:
                vow_enable();
                //VOW_LOG("S_EN_A\n\r");
                vow_ipi_sendmsg(ipiCmdInfo.ipi_msgid, &(ipiCmdInfo.ipi_ret), VOW_IPI_SUCCESS, 0, 1);
                break;
            case SCP_IPIMSG_VOW_DISABLE_ACK:
                vow_disable();
                //VOW_LOG("S_DIS_A\n\r");
                vow_ipi_sendmsg(ipiCmdInfo.ipi_msgid, &(ipiCmdInfo.ipi_ret), VOW_IPI_SUCCESS, 0, 1);
                break;
            case SCP_IPIMSG_VOW_SETMODE_ACK:
                vow_setmode((vow_mode_t)ipiCmdInfo.para1);
                //VOW_LOG("S_MODE_A\n\r");
                vow_ipi_sendmsg(ipiCmdInfo.ipi_msgid, &(ipiCmdInfo.ipi_ret), VOW_IPI_SUCCESS, 0, 1);
                break;
            case SCP_IPIMSG_VOW_APREGDATA_ADDR_ACK:
                vow_setapreg_addr(ipiCmdInfo.para1);
                ipiCmdInfo.ipi_ret = vow_gettcmreg_addr();
                //VOW_LOG("S_AP_ADR_A=0x%x\n\r", ipiCmdInfo.ipi_ret);
                vow_ipi_sendmsg(ipiCmdInfo.ipi_msgid, &(ipiCmdInfo.ipi_ret), VOW_IPI_SUCCESS, 4, 1);
                break;
            case SCP_IPIMSG_SET_VOW_MODEL_ACK:
                result = vow_setModel((vow_event_info_t)ipiCmdInfo.para1, ipiCmdInfo.para2, ipiCmdInfo.para3, ipiCmdInfo.para4);
                if (result != VOW_IPI_SUCCESS) {
                    vow_ipi_sendmsg(ipiCmdInfo.ipi_msgid, &(ipiCmdInfo.ipi_ret), VOW_IPI_SUCCESS, 0, 1);
                }
                //VOW_LOG("S_MDL_A:%x\n\r", result);
                break;
            case SCP_IPIMSG_SET_FLAG_ACK:
                vow_set_flag((vow_flag_t)ipiCmdInfo.para1, (short)ipiCmdInfo.para2);
                //VOW_LOG("S_FG_A\n\r");
                vow_ipi_sendmsg(ipiCmdInfo.ipi_msgid, &(ipiCmdInfo.ipi_ret), VOW_IPI_SUCCESS, 0, 1);
                break;
            default:
                break;
        }
    }
}
