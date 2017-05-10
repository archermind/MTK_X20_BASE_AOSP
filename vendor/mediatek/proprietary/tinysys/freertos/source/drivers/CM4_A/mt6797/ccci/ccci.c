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

#include <driver_api.h>
#include <platform.h>
#include <interrupt.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <scp_ipi.h>
#include <string.h>
#include <interrupt.h>
#include <vcore_dvfs.h>
#include <semphr.h>
#include "ccism_ringbuf.h"
#include "dma.h"

#define CCCI_REQUEST_DRAM() \
do { \
    dvfs_enable_DRAM_resource(CCCI_MEM_ID); \
    taskENTER_CRITICAL(); \
    get_emi_semaphore(); \
} while(0)

#define CCCI_RELEASE_DRAM() \
do { \
    release_emi_semaphore(); \
    taskEXIT_CRITICAL(); \
    dvfs_disable_DRAM_resource(CCCI_MEM_ID); \
} while(0)

#ifdef CCCI_PROFILING
//static volatile unsigned int *ITM_CONTROL = (unsigned int *)0xE0000E80;
static volatile unsigned int *DWT_CONTROL = (unsigned int *)0xE0001000;
static volatile unsigned int *DWT_CYCCNT = (unsigned int *)0xE0001004;
static volatile unsigned int *DEMCR = (unsigned int *)0xE000EDFC;

#define CPU_RESET_CYCLECOUNTER() \
    do { \
        *DEMCR = *DEMCR | 0x01000000; \
        *DWT_CYCCNT = 0; \
        *DWT_CONTROL = *DWT_CONTROL | 1 ; \
    } while(0)
static unsigned int start_time;
static unsigned int end_time;
#endif

static struct ccci_ipi_msg ipi_tx_msg;
SemaphoreHandle_t ccci_mutex_to_ap;
SemaphoreHandle_t ccci_mutex_to_md;
unsigned int ccci_log_level = 0;

struct ccci_modem md_list[MD_NUM];
static int rx_queue_buffer_size[MD_NUM][MD_QUEUE_NUM] = {
    {16 * 1024, 0},
    {1 * 1024, 16 * 1024}
};
static int tx_queue_buffer_size[MD_NUM][MD_QUEUE_NUM] = {
    {1 * 1024, 0},
    {1 * 1024, 1 * 1024}
};
static int gpio_num_list[MD_NUM] = {SPM2MD_DVFS, C2K_CONFIG};
static int eint_num_list[MD_NUM] = {GIPC_IN_3, GIPC_IN_3};
static struct ccci_rx_callback rx_callbacks[MAX_RX_CALLBACKS];
static QueueHandle_t ccci_ipi_queue;
static QueueHandle_t ccci_eint_queue;

static void ccci_reset_seq_num(struct ccci_modem * md)
{
    memset(md->seq_nums[OUT], 0, sizeof(md->seq_nums[OUT]));
    memset(md->seq_nums[IN], -1, sizeof(md->seq_nums[IN]));
}

static void ccci_wakeup_md(struct ccci_modem *md)
{
    switch (md->index) {
        case MD_SYS1:
            if (INREG32(md->gpio_num) & (1 << 7)) {
                CCCI_DBG_MSG(md->index, "HIF", "no need wakeup MD\n");
            } else {
                OUTREG32(md->gpio_num, INREG32(md->gpio_num) | (1 << 23));
                OUTREG32(md->gpio_num, INREG32(md->gpio_num) | (1 << 7));
                CCCI_DBG_MSG(md->index, "HIF", "wakeup MD 0x%x->0x%x\n", md->gpio_num, INREG32(md->gpio_num));
            }
            break;
        case MD_SYS3:
            if (INREG32(md->gpio_num) & (1 << 14)) {
                CCCI_DBG_MSG(md->index, "HIF", "no need wakeup MD\n");
            } else {
                OUTREG32(md->gpio_num, INREG32(md->gpio_num) | (1 << 14));
                CCCI_DBG_MSG(md->index, "HIF", "wakeup MD 0x%x->0x%x\n", md->gpio_num, INREG32(md->gpio_num));
            }
            break;
    }
}

static void ccci_ack_md(struct ccci_modem *md)
{
    OUTREG32(md->eint_num, INREG32(md->eint_num) & (~0x1));
    CCCI_DBG_MSG(md->index, "HIF", "acked MD GIPO\n");
}

static int ccci_send_msg_to_md(struct ccci_modem *md, int qno, int ch, void *buf, int length, int wait)
{
    int ret;
    struct ccci_header ccci_h;

retry:
    if (md->state != MD_BOOT_STAGE_2 && ch != CCCI_MD_SCP_TX_LB_IT) {
        CCCI_ERR_MSG(md->index, "HIF", "MD is not ready %d\n", md->state);
        return -1;
    }
    if (length > CCCI_MTU) {
        CCCI_ERR_MSG(md->index, "HIF", "Txq%d packet too big %d\n", qno, length);
        return -2;
    }
    if (md->smem_ring_buff[qno] == NULL) {
        CCCI_ERR_MSG(md->index, "HIF", "Txq%d not exist\n", qno);
        return -3;
    }
    if (xSemaphoreTake(ccci_mutex_to_md, portMAX_DELAY) != pdTRUE) {
        CCCI_ERR_MSG(md->index, "HIF", "fail to take muxtex\n");
        return -4;
    }
    //add CCCI header
    length += sizeof(struct ccci_header);
    ccci_h.data[0] = 0;
    ccci_h.data[1] = length;
    switch (md->index) {
        case MD_SYS1:
            ccci_h.peer_id = 1;
            break;
        case MD_SYS3:
            ccci_h.peer_id = 2;
            break;
        default:
            ccci_h.peer_id = 0;
            break;
    };
    ccci_h.peer_channel = ch;
    ccci_h.seq_num = md->seq_nums[OUT][ch];
    ccci_h.assert_bit = 1;
    ccci_h.reserved = 0;
    //write share memory
    CCCI_REQUEST_DRAM();
#ifdef CCCI_PROFILING
    CPU_RESET_CYCLECOUNTER();
    start_time = *DWT_CYCCNT;
#endif
    ret = ccci_ringbuf_write(md->index, md->smem_ring_buff[qno], (unsigned char *)&ccci_h, buf, length);
#ifdef CCCI_PROFILING
    end_time = *DWT_CYCCNT;
    CCCI_INF_MSG(md->index, "PRF", "%d, %d, %d, %d\n", start_time, end_time, end_time - start_time, length);
#endif
    CCCI_RELEASE_DRAM();
    CCCI_INF_MSG(md->index, "HIF", "ring buffer write(%d %d %d): req(%d),ret(%d)\n",
                 ccci_h.peer_channel, ccci_h.seq_num, ccci_h.data[1], length, ret);
    //success
    if (ret == length) {
        ccci_wakeup_md(md);
        md->seq_nums[OUT][ch]++;
    }
    xSemaphoreGive(ccci_mutex_to_md);

    if (wait == 1 && ret == -CCCI_RINGBUF_NOT_ENOUGH) {
        vTaskDelay(5 / portTICK_RATE_MS); // otherwise we'll have no chance to handle IPI message
        goto retry;
    }
    return ret;
}

static void ccci_send_msg_to_ap(int md_id, int op_id, void *data)
{
    if (xSemaphoreTake(ccci_mutex_to_ap, portMAX_DELAY) != pdTRUE) {
        CCCI_ERR_MSG(md_id, "IPI", "fail to take muxtex\n");
        return;
    }
    memset(&ipi_tx_msg, 0, sizeof(ipi_tx_msg));
    ipi_tx_msg.md_id = md_id == MD_SYS3 ? 2 : md_id; // FIXME, modem index convert
    ipi_tx_msg.op_id = op_id;
    ipi_tx_msg.data[0] = *((uint32_t *)data);
    while (scp_ipi_send(IPI_APCCCI, &ipi_tx_msg, sizeof(ipi_tx_msg), 1, IPI_SCP2AP) != DONE);
    CCCI_INF_MSG(md_id, "IPI", "IPI send %d/0x%x, %d\n",
                 ipi_tx_msg.op_id, ipi_tx_msg.data[0], sizeof(struct ccci_ipi_msg));
    xSemaphoreGive(ccci_mutex_to_ap);
}

static void ccci_trigger_eint_task(struct ccci_modem *md)
{
    struct ccci_eint_queue_event event;
    BaseType_t ret;

    event.id = md->index;
    ret = xQueueSendFromISR(ccci_eint_queue, &event, NULL);
    if (ret != pdPASS)
        CCCI_ERR_MSG(md->index, "EINT", "xQueueSendFromISR error %d\n", ret);
}

static void ccci_eint_handler(void)
{
    struct ccci_modem *md = &md_list[0]; // FIXME, hardcode modem number

    ccci_trigger_eint_task(md);
    ccci_ack_md(md);
    mask_irq(IPC3_IRQn);
}

static inline void ccci_call_rx(int host_id, int ch_id, void *data)
{
    int i;

    for (i = 0; i < sizeof(rx_callbacks) / sizeof(struct ccci_rx_callback); i++) {
        if (rx_callbacks[i].host_id == host_id && rx_callbacks[i].ch_id == ch_id) {
            CCCI_DBG_MSG(host_id, "RX", "call %p for ch%d\n", rx_callbacks[i].callback, ch_id);
            rx_callbacks[i].callback(data);
        }
    }
}

static void ccci_collect_rx_data(struct ccci_modem *md)
{
    int j, pkg_size, ret;
    struct ccci_header ccci_h;
    unsigned char *rx_buff;

    //read share memory
    for (j = 0; j < MD_QUEUE_NUM; j++) {
        while (1) {
            if (md->smem_ring_buff[j] == NULL) // "!md->smem_ring_buff[j]" not working, weired
                break; // avaliable queues must be continuous
            CCCI_REQUEST_DRAM();
#ifdef CCCI_PROFILING
            CPU_RESET_CYCLECOUNTER();
            start_time = *DWT_CYCCNT;
#endif
            //collect packet
            pkg_size = ccci_ringbuf_readable(md->index, md->smem_ring_buff[j]);
            if (pkg_size < (int)(sizeof(struct ccci_header))) {
                CCCI_RELEASE_DRAM();
                break; // header only packet is allowed
            }
            if (pkg_size > (int)(sizeof(md->ccism_rx_msg))) {
                CCCI_ERR_MSG(md->index, "HIF", "Rxq%d packet too big %d\n", j, pkg_size);
                CCCI_RELEASE_DRAM();
                break;
            }
            // read CCCI header
            rx_buff = (unsigned char *)&ccci_h;
            ret = ccci_ringbuf_read(md->index, md->smem_ring_buff[j], rx_buff, sizeof(struct ccci_header));
            CCCI_DBG_MSG(md->index, "HIF", "ring buffer read header(%d %d %d): ret(%d)\n",
                         ccci_h.peer_channel, ccci_h.seq_num, ccci_h.data[1], ret);
            // sanity check
            if (ccci_h.seq_num - md->seq_nums[IN][ccci_h.peer_channel] != 1)
                CCCI_ERR_MSG(md->index, "HIF", "channel %d seq number out-of-order %d->%d\n",
                             ccci_h.peer_channel, ccci_h.seq_num, md->seq_nums[IN][ccci_h.peer_channel]);
            md->seq_nums[IN][ccci_h.peer_channel] = ccci_h.seq_num;
            // read whole packet, including CCCI header
            rx_buff = md->ccism_rx_msg;
            ret = ccci_ringbuf_read(md->index, md->smem_ring_buff[j], rx_buff, pkg_size);
            // read end
            ccci_ringbuf_move_rpointer(md->index, md->smem_ring_buff[j], pkg_size);
#ifdef CCCI_PROFILING
            end_time = *DWT_CYCCNT;
            CCCI_INF_MSG(md->index, "PRF", "%d, %d, %d, %d\n", start_time, end_time, end_time - start_time, pkg_size);
#endif
            CCCI_RELEASE_DRAM();
            CCCI_DBG_MSG(md->index, "HIF", "ring buffer read whole: ret(%d), req(%d)\n", ret, pkg_size);
#if 0
            // remove CCCI header
            pkg_size -= sizeof(struct ccci_header);
            memmove(rx_buff, rx_buff + sizeof(struct ccci_header), pkg_size);
#endif
            // handle packet (do not put these code in function, seems stack size is quite limited and weired thing happens)
            switch (md->index) {
                case MD_SYS1:
                    switch (ccci_h.peer_channel) {
                        case CCCI_CELLINFO_CHANNEL_RX:
                            ret = sensor_modem_notify(md);
                            CCCI_INF_MSG(md->index, "HIF", "cell info (%d %d)(%d)\n",
                                         ccci_h.seq_num, ccci_h.data[1], ret);
#ifndef CCCI_SENSOR_SUPPORT
                            ccci_send_msg_to_md(md, j, CCCI_CELLINFO_CHANNEL_TX, rx_buff + sizeof(struct ccci_header),
                                                pkg_size - sizeof(struct ccci_header), 0);
#endif
                            break;
                        case CCCI_MD_SCP_RX_LB_IT:
                            CCCI_INF_MSG(md->index, "HIF", "loopback (%d %d)\n", ccci_h.seq_num, ccci_h.data[1]);
                            ccci_send_msg_to_md(md, j, CCCI_MD_SCP_TX_LB_IT, rx_buff + sizeof(struct ccci_header),
                                                pkg_size - sizeof(struct ccci_header), 0);
                            break;
                        default:
                            CCCI_ERR_MSG(md->index, "HIF", "illegal channel ID %d\n", ccci_h.peer_channel);
                            break;
                    };
                    break;
                case MD_SYS3:
                    switch (ccci_h.peer_channel) {
                        case CCCI_C2K_CH_AUDIO:
                            ccci_call_rx(MD_SYS3, CCCI_C2K_CH_AUDIO, NULL);
                            break;
                        case CCCI_C2K_CH_GEOFENCE:
                            ret = sensor_modem_notify(md);
                            CCCI_INF_MSG(md->index, "HIF", "cell info (%d %d)(%d)\n",
                                         ccci_h.seq_num, ccci_h.data[1], ret);
#ifndef CCCI_SENSOR_SUPPORT
                            /*
                                                   * different from MD1, MD3 loopback's Tx and Rx is seperated, so SCP may send fail
                                                   * when MD3 is still reading and not having enough space.
                                                   */
                            ccci_send_msg_to_md(md, j, CCCI_C2K_CH_GEOFENCE, rx_buff + sizeof(struct ccci_header),
                                                pkg_size - sizeof(struct ccci_header), 1);
#endif
                            break;
                        default:
                            CCCI_ERR_MSG(md->index, "HIF", "illegal channel ID %d\n", ccci_h.peer_channel);
                            break;
                    };
                    break;
                default:
                    CCCI_ERR_MSG(md->index, "HIF", "illegal MD index %d\n", md->index);
                    break;
            };
        }
    }
}

static void ccci_eint_task(void *pvParameters)
{
    struct ccci_eint_queue_event event;
    BaseType_t queue_recv;
    int i;

    CCCI_INF_MSG(-1, "EINT", "task run\n");
    while (1) {
        queue_recv = xQueueReceive(ccci_eint_queue, &event, portMAX_DELAY);
        if (queue_recv != pdPASS) {
            CCCI_ERR_MSG(-1, "EINT", "xQueueReceive failed\n");
            continue;
        }
        CCCI_DBG_MSG(-1, "EINT", "xQueueReceive %d\n", event.id);

        //traverse every MD
        for (i = 0; i < MD_NUM; i++) { // FIXME: as both modem share one EINT, we do not check event.id
            if (md_list[i].state != MD_BOOT_STAGE_2)
                continue;
            ccci_collect_rx_data(&md_list[i]);
        }
        unmask_irq(IPC3_IRQn);
    }
}

static void ccci_ipi_handler(int id, void *data, uint len)
{
    struct ccci_ipi_queue_event event;
    BaseType_t ret;
    struct ccci_ipi_msg *ipi_msg_ptr = data;

    if (len != sizeof(struct ccci_ipi_msg)) {
        CCCI_ERR_MSG(-1, "IPI", "hanlder, data length wrong %d vs. %d\n", len, sizeof(struct ccci_ipi_msg));
        return;
    }

    event.id = id;
    memcpy(&event.data, data, sizeof(struct ccci_ipi_msg));
    ret = xQueueSendFromISR(ccci_ipi_queue, &event, NULL);
    if (ret != pdPASS) {
        CCCI_ERR_MSG(-1, "IPI", "xQueueSendFromISR error, %d/%d\n", ipi_msg_ptr->md_id, ipi_msg_ptr->op_id);
    }
}

static void ccci_ipi_task(void *pvParameters)
{
    struct ccci_ipi_queue_event event;
    BaseType_t queue_recv;
    uint32_t test_data;
    int i, buff_size;
    unsigned char *buf;
    struct ccci_modem *md;

    CCCI_INF_MSG(-1, "IPI", "task run\n");
    while (1) {
        queue_recv = xQueueReceive(ccci_ipi_queue, &event, portMAX_DELAY);
        if (queue_recv != pdPASS) {
            CCCI_ERR_MSG(-1, "IPI", "xQueueReceive failed\n");
            continue;
        }
        if (event.data.md_id < 0 || event.data.md_id > MD_NUM) {
            CCCI_ERR_MSG(-1, "IPI", "xQueueReceive false msg %d\n", event.data.md_id);
            continue;
        }
        event.data.md_id = event.data.md_id == 2 ? MD_SYS3 : event.data.md_id; // FIXME, modem index convert
        CCCI_DBG_MSG(event.data.md_id, "IPI", "xQueueReceive %d\n", event.id);

        md = &md_list[event.data.md_id];
        CCCI_INF_MSG(event.data.md_id, "IPI", "handler %d/0x%x\n", event.data.op_id, event.data.data[0]);

        switch (event.data.op_id) {
            case CCCI_OP_LOG_LEVEL:
                ccci_log_level = event.data.data[0];
                break;
            case CCCI_OP_MSGSND_TEST:
                test_data = 0x20150702;
                ccci_send_msg_to_md(md, 0, CCCI_MD_SCP_TX_LB_IT, &test_data, sizeof(test_data), 0);
                break;
            case CCCI_OP_ASSERT_TEST:
                //configASSERT();
                test_data = SCP_CCCI_STATE_BOOTING;
                ccci_send_msg_to_ap(MD_SYS1, CCCI_OP_SCP_STATE, &test_data);
                break;

            case CCCI_OP_SHM_INIT:
                //test SCP DRAM access
                buf = (unsigned char *)ap_to_scp(event.data.data[0]);
                if (buf == NULL)
                    break;
                //create ring buffer
                md->smem_base_addr = buf;
                for (i = 0; i < MD_QUEUE_NUM; i++) {
                    if (rx_queue_buffer_size[md->index][i] == 0 || tx_queue_buffer_size[md->index][i] == 0)
                        continue;
                    buff_size = CCCI_RINGBUF_CTL_LEN +
                                rx_queue_buffer_size[md->index][i] + tx_queue_buffer_size[md->index][i];
                    CCCI_REQUEST_DRAM();
#ifdef CCCI_PROFILING
                    CPU_RESET_CYCLECOUNTER();
                    start_time = *DWT_CYCCNT;
#endif
                    md->smem_ring_buff[i] = ccci_create_ringbuf(md->index, buf, buff_size,
                                            rx_queue_buffer_size[md->index][i], tx_queue_buffer_size[md->index][i]);
#ifdef CCCI_PROFILING
                    end_time = *DWT_CYCCNT;
                    CCCI_INF_MSG(md->index, "PRF", "%d, %d, %d, %d\n", start_time, end_time, end_time - start_time, buff_size);
#endif
                    CCCI_RELEASE_DRAM();
                    if (md->smem_ring_buff[i] == NULL) {
                        CCCI_ERR_MSG(md->index, "IPI", "ccci_create_ringbuf %d failed\n", i);
                        break;
                    }
                    buf += buff_size;
                }
                ccci_reset_seq_num(md);
                //send message to AP
                test_data = SCP_CCCI_STATE_RBREADY;
                ccci_send_msg_to_ap(md->index, CCCI_OP_SCP_STATE, &test_data);
                break;
            case CCCI_OP_SHM_RESET:
                for (i = 0; i < MD_QUEUE_NUM; i++) {
                    if (md->smem_ring_buff[i] == NULL)
                        continue;
                    CCCI_REQUEST_DRAM();
#ifdef CCCI_PROFILING
                    CPU_RESET_CYCLECOUNTER();
                    start_time = *DWT_CYCCNT;
#endif
                    ccci_ringbuf_reset(md->index, md->smem_ring_buff[i], 0);
                    ccci_ringbuf_reset(md->index, md->smem_ring_buff[i], 1);
#ifdef CCCI_PROFILING
                    end_time = *DWT_CYCCNT;
                    CCCI_INF_MSG(md->index, "PRF", "%d, %d, %d, %d\n", start_time, end_time, end_time - start_time, 0);
#endif
                    CCCI_RELEASE_DRAM();
                }
                break;
            case CCCI_OP_MD_STATE:
                md->state = event.data.data[0];
                if (md->state == MD_BOOT_STAGE_2) // to catch early MD message
                    ccci_trigger_eint_task(md);
                break;
        };
    };
}

void ccci_init(void)
{
    int ret, i;
    struct ccci_modem *md;

    //register IPI handler
    ret = scp_ipi_registration(IPI_APCCCI, ccci_ipi_handler, "CCCI IPI");
    CCCI_INF_MSG(-1, "INIT", "register IPI %d, %d\n", IPI_APCCCI, ret);
    if (ret != DONE)
        return;
    ccci_ipi_queue = xQueueCreate(2, sizeof(struct ccci_ipi_queue_event));
    ret = kal_xTaskCreate(ccci_ipi_task, "CC_I", 256, NULL, 2, NULL);
    CCCI_INF_MSG(-1, "INIT", "create IPI task %d %p\n", ret, ccci_ipi_queue);

    //register EINT handler
    request_irq(IPC3_IRQn, ccci_eint_handler, "IPC3"); // GIPC_IN_3
    CCCI_INF_MSG(-1, "INIT", "register EINT %d\n", IPC3_IRQn);
    ccci_eint_queue = xQueueCreate(2, sizeof(struct ccci_eint_queue_event));
    ret = kal_xTaskCreate(ccci_eint_task, "CC_E", 256, NULL, 2, NULL);
    CCCI_INF_MSG(-1, "INIT", "create EINT task %d %p\n", ret, ccci_eint_queue);

    ccci_mutex_to_ap = xSemaphoreCreateMutex();
    ccci_mutex_to_md = xSemaphoreCreateMutex();

    for (i = 0; i < MD_NUM; i++) {
        md =  &md_list[i];
        md->index = i;
        md->eint_num = eint_num_list[i];
        md->gpio_num = gpio_num_list[i];
        md->smem_ring_buff[i] = NULL;
    }

    //send message to AP
    ret = SCP_CCCI_STATE_BOOTING;
    ccci_send_msg_to_ap(MD_SYS1, CCCI_OP_SCP_STATE, &ret); // hardcode to MD1, as MD1 always exists
    CCCI_INF_MSG(MD_SYS1, "INIT", "report booting state to AP\n");
}

int ccci_register_rx(int host_id, int ch_id, CCCI_RX_FUNC_T callback)
{
    int i;

    for (i = 0; i < sizeof(rx_callbacks) / sizeof(struct ccci_rx_callback); i++) {
        if (rx_callbacks[i].callback == NULL) {
            rx_callbacks[i].host_id = host_id;
            rx_callbacks[i].ch_id = ch_id;
            rx_callbacks[i].callback = callback;
            return 0;
        }
    }
    return -1;
}

int ccci_geo_fence_send(int md_id, void *buf, int length)
{
    int i, qno;
    struct ccci_modem *md = NULL;

    for (i = 0; i < MD_NUM; i++) {
        if (md_list[i].index == md_id) {
            md = &md_list[i];
        }
    }
    if (!md) {
        CCCI_ERR_MSG(md->index, "HIF", "tx MD ID(%d) wrong\n", md_id);
        return -1;
    }
    if (length > CCCI_MTU) {
        CCCI_ERR_MSG(md->index, "HIF", "tx packet size(length) > MTU\n", length);
        return -2;
    }
    switch (md->index) {
        case MD_SYS1:
            qno = 0;
            break;
        case MD_SYS3:
            qno = 1;
            break;
        default:
            qno = 0;
            break;
    };
    return ccci_send_msg_to_md(md, qno, CCCI_CELLINFO_CHANNEL_TX, buf, length, 0);
}

