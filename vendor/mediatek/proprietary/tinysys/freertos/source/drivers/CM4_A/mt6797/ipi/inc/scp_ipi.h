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

#ifndef SCP_IPI_H
#define SCP_IPI_H

enum ipi_id {
    IPI_WDT = 0,
    IPI_TEST1,
    IPI_TEST2,
    IPI_LOGGER,
    IPI_SWAP,
    IPI_VOW,
    IPI_AUDIO,
    IPI_THERMAL,
    IPI_SPM,
    IPI_DVT_TEST,
    IPI_I2C,
    IPI_TOUCH,
    IPI_SENSOR,
    IPI_TIME_SYNC,
    IPI_GPIO,
    IPI_BUF_FULL,
    IPI_DFS,
    IPI_SHF,
    IPI_CONSYS,
    IPI_OCD,
    IPI_APDMA,
    IPI_IRQ_AST,
    IPI_DFS4MD,
    IPI_SCP_READY,
    IPI_ETM_DUMP,
    IPI_APCCCI,
    IPI_RAM_DUMP,
    IPI_DVFS_DEBUG,
    IPI_DVFS_FIX_OPP_SET,
    IPI_DVFS_FIX_OPP_EN,
    IPI_DVFS_LIMIT_OPP_SET,
    IPI_DVFS_LIMIT_OPP_EN,
    IPI_DVFS_DISABLE,
    IPI_DVFS_SLEEP,
    IPI_DVFS_WAKE,
    IPI_DUMP_REG,
    IPI_SCP_STATE,
    IPI_DVFS_SET_FREQ,
    NR_IPI,
};

#define IPI_AS_WAKEUP_SRC

extern unsigned char _SCP_IPC_SHARE_BUFFER_ADDR;
#define SCP_IPC_SHARE_BUFFER     ((volatile unsigned int*)&_SCP_IPC_SHARE_BUFFER_ADDR)
#define SCP_TCM_REG      (*(volatile unsigned int *)0x0)
#define SCP_TO_HOST_REG  (*(volatile unsigned int *)0x400a001C)
#define SCP_TO_SPM_REG   (*(volatile unsigned int *)0x400a0020)
#define HOST_TO_SCP_REG  (*(volatile unsigned int *)0x400a0024)
#define SEM_REG           (*(volatile unsigned int *)0x400a0090)

#define SEMA_0_SCP       (1 << 0)
#define IPC_SCP2HOST       (1 << 0)
#define IPC_SCP2SPM        (1 << 0)
#define WDT_INT           (1 << 8)


enum ipi_status {
    ERROR =-1,
    DONE,
    BUSY,
};
enum ipi_dir {
    IPI_SCP2AP = 0,
    IPI_SCP2SPM,
    IPI_SCP2CONN,
    IPI_SCP2MD
};

typedef enum ipi_status ipi_status;

typedef void(*ipi_handler_t)(int id, void * data, uint len);

struct ipi_desc_t {
    ipi_handler_t handler;
    const char  *name;
    uint32_t irq_count;
    uint32_t last_handled;
    uint32_t is_wakeup_src;
};

#define SHARE_BUF_SIZE 64
struct share_obj {
    enum ipi_id id;
    unsigned int len;
    unsigned char reserve[8];
    unsigned char share_buf[SHARE_BUF_SIZE - 16];
};

void scp_ipi_init(void);

ipi_status scp_ipi_registration(enum ipi_id id, ipi_handler_t handler, const char *name);
ipi_status scp_ipi_send(enum ipi_id id, void* buf, uint32_t len, uint32_t wait,enum ipi_dir dir);
ipi_status scp_ipi_status(enum ipi_id id);
void scp_ipi_wakeup_ap_registration(enum ipi_id id);
void ipi_status_dump(void);
#endif
