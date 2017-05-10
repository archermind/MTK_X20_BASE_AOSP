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

#include <string.h>
#include "ccci.h"

#ifdef CCCI_SENSOR_SUPPORT

typedef enum {
    MTK_MDM_DISABLE = 0x0, // default
    MTK_MDM_ENABLE_CHANGE_OF_CELL_ID, // enable phase I: only report when camping cell ID has change
    MTK_MDM_ENABLE_CHANGE_OF_CELL_RSSI, // enable phase II: report under phase I + report when camping cell’s RSSI has change
    MTK_MDM_CMD_END
} MTK_MDM_CMD;

extern struct ccci_modem md_list[MD_NUM];

static int sensor_modem_operate(int md_id, Sensor_Command command,
                                void* buffer_in, int size_in, void* buffer_out, int size_out)
{
    int err = 0;
    int value = 0;
    MTK_MDM_HEADER_T *mdm_head;
    UINT32 mdm_command;

    if (NULL == buffer_in)
        return -1;
    switch (command) {
        case ACTIVATE:
            if ((buffer_in == NULL) || (size_in < sizeof(int))) {
                err = -1;
            } else {
                value = *(int *)buffer_in;
                if (0 == value) { //disable modem
                    mdm_head = pvPortMalloc(sizeof(MTK_MDM_HEADER_T) + sizeof(UINT32));
                    if (mdm_head != NULL) {
                        mdm_head->length = sizeof(UINT32);
                        mdm_command = MTK_MDM_DISABLE;
                        memcpy(((UINT8 *)mdm_head + sizeof(MTK_MDM_HEADER_T)), &mdm_command, sizeof(UINT32));
                        ccci_geo_fence_send(md_id, mdm_head, (sizeof(MTK_MDM_HEADER_T) + sizeof(UINT32)));
                        vPortFree(mdm_head);
                    }
                } else { //enable modem
                    mdm_head = pvPortMalloc(sizeof(MTK_MDM_HEADER_T) + sizeof(UINT32));
                    if (mdm_head != NULL) {
                        mdm_head->length = sizeof(UINT32);
                        mdm_command = MTK_MDM_ENABLE_CHANGE_OF_CELL_ID;
                        memcpy(((UINT8 *)mdm_head + sizeof(MTK_MDM_HEADER_T)), &mdm_command, sizeof(UINT32));
                        ccci_geo_fence_send(md_id, mdm_head, (sizeof(MTK_MDM_HEADER_T) + sizeof(UINT32)));
                        vPortFree(mdm_head);
                    }
                }
            }
            break;
        default:
            break;
    }
    return err;
}

static int sensor_modem_1_operate(Sensor_Command command,
                                  void* buffer_in, int size_in, void* buffer_out, int size_out)
{
    return sensor_modem_operate(MD_SYS1, command, buffer_in, size_in, buffer_out, size_out);
}

static int sensor_modem_3_operate(Sensor_Command command,
                                  void* buffer_in, int size_in, void* buffer_out, int size_out)
{
    return sensor_modem_operate(MD_SYS3, command, buffer_in, size_in, buffer_out, size_out);
}

static int sensor_modem_1_run_algorithm(struct data_t *output)
{
    struct ccci_header *ccci_h = (struct ccci_header *)md_list[MD_SYS1].ccism_rx_msg;
    modem_vec_t *modem_vec = (modem_vec_t *)(md_list[MD_SYS1].ccism_rx_msg + sizeof(struct ccci_header));

    CCCI_INF_MSG(MD_SYS1, "SNR", "run algorithm %d (%d %d %d) %d\n",
                 md_list[MD_SYS1].state, ccci_h->peer_channel, ccci_h->seq_num, ccci_h->data[1],
                 modem_vec->header.length);
    if (ccci_h->peer_channel != CCCI_CELLINFO_CHANNEL_RX || md_list[MD_SYS1].state != MD_BOOT_STAGE_2)
        return 0;
    if (modem_vec->header.length > sizeof(output->data->modem_t.data))
        return 0;
    output->data_exist_count = 1;
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->sensor_type = SENSOR_TYPE_MODEM_1;
    memcpy(&output->data->modem_t, modem_vec, modem_vec->header.length + sizeof(output->data->modem_t.header));
    return 0;
}

static int sensor_modem_3_run_algorithm(struct data_t *output)
{
    struct ccci_header *ccci_h = (struct ccci_header *)md_list[MD_SYS3].ccism_rx_msg;
    modem_vec_t *modem_vec = (modem_vec_t *)(md_list[MD_SYS3].ccism_rx_msg + sizeof(struct ccci_header));

    CCCI_INF_MSG(MD_SYS3, "SNR", "run algorithm %d (%d %d %d) %d\n",
                 md_list[MD_SYS3].state, ccci_h->peer_channel, ccci_h->seq_num, ccci_h->data[1],
                 modem_vec->header.length);
    if (ccci_h->peer_channel != CCCI_C2K_CH_GEOFENCE || md_list[MD_SYS3].state != MD_BOOT_STAGE_2)
        return 0;
    if (modem_vec->header.length > sizeof(output->data->modem_t.data))
        return 0;
    output->data_exist_count = 1;
    output->data->time_stamp = read_xgpt_stamp_ns();
    output->data->sensor_type = SENSOR_TYPE_MODEM_3;
    memcpy(&output->data->modem_t, modem_vec, modem_vec->header.length + sizeof(output->data->modem_t.header));
    return 0;
}

int sensor_modem_notify(struct ccci_modem *md)
{
    switch (md->index) {
        case MD_SYS1:
            return sensor_subsys_algorithm_notify(SENSOR_TYPE_MODEM_1);
        case MD_SYS3:
            return sensor_subsys_algorithm_notify(SENSOR_TYPE_MODEM_3);
        default:
            CCCI_ERR_MSG(md->index, "SNR", "wrong MD index\n");
            break;
    };
    return 0;
}

static int sensor_modem_init(struct ccci_modem *md)
{
    struct SensorDescriptor_t *sensor = &md->sensor;
    int ret;

    switch (md->index) {
        case MD_SYS1:
            sensor->sensor_type = SENSOR_TYPE_MODEM_1;
            sensor->run_algorithm = sensor_modem_1_run_algorithm;
            sensor->operate = sensor_modem_1_operate;
            break;
        case MD_SYS3:
            sensor->sensor_type = SENSOR_TYPE_MODEM_3;
            sensor->run_algorithm = sensor_modem_3_run_algorithm;
            sensor->operate = sensor_modem_3_operate;
            break;
        default:
            CCCI_ERR_MSG(md->index, "SNR", "wrong MD index\n");
            break;
    };

    sensor->version = 0x20150809;
    sensor->report_mode = on_change;
    sensor->hw.max_sampling_rate = 1;
    sensor->hw.support_HW_FIFO = 0;
    sensor->set_data = NULL;
    sensor->accumulate = 0;

    ret = sensor_subsys_algorithm_register_type(sensor);
    CCCI_INF_MSG(md->index, "SNR", "regsiter algorithm %d\n", ret);

    ret = sensor_subsys_algorithm_register_data_buffer(sensor->sensor_type, 1);
    CCCI_INF_MSG(md->index, "SNR", "regsiter data buffer %d\n", ret);
    return ret;
}

static int sensor_modem1_init(void)
{
    return sensor_modem_init(&md_list[MD_SYS1]);
}

static int sensor_modem3_init(void)
{
    return sensor_modem_init(&md_list[MD_SYS3]);
}

MODULE_DECLARE(modem1, MOD_VIRT_SENSOR, sensor_modem1_init);
MODULE_DECLARE(modem3, MOD_VIRT_SENSOR, sensor_modem3_init);
#else
int sensor_modem_notify(struct ccci_modem *md)
{
    return 0;
}
#endif
