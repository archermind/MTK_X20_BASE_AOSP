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

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "timers.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <mt_reg_base.h>
#include <driver_api.h>
#include <platform.h>
#include <interrupt.h>
#include <scp_ipi.h>

// .share section should be zero filled
#define SHARESEC	 __attribute__ ((section (".share")))
#define LOG_LEN	1024

static unsigned char scp_log_buf[LOG_LEN] SHARESEC;
static volatile unsigned int scp_log_start SHARESEC;	/* Index into log_buf: next char to be read by syslog() */
static volatile unsigned int scp_log_end SHARESEC;	/* Index into log_buf: most-recently-written-char + 1 */
static volatile unsigned int scp_log_lock SHARESEC; /* 0:can write log;  1:locked */
static volatile unsigned int enable_scp_mobile_log SHARESEC; /* 0:disable mobile log ; 1:enable mobile log */
static unsigned int scp_mobile_log_ready SHARESEC;

typedef struct {
    unsigned int scp_log_buf_addr;
    unsigned int scp_log_start_idx_addr;
    unsigned int scp_log_end_idx_addr;
    unsigned int scp_log_lock_addr;
    unsigned int enable_scp_mobile_log_addr;
    unsigned int scp_log_buf_maxlen;
} SCP_LOG_INFO;

void send_log_info(void)
{
    SCP_LOG_INFO log_info;

    log_info.scp_log_buf_addr = (unsigned int) scp_log_buf;
    log_info.scp_log_start_idx_addr = (unsigned int)&scp_log_start;
    log_info.scp_log_end_idx_addr = (unsigned int)&scp_log_end;
    log_info.scp_log_lock_addr = (unsigned int)&scp_log_lock;
    log_info.enable_scp_mobile_log_addr= (unsigned int)&enable_scp_mobile_log;
    log_info.scp_log_buf_maxlen = LOG_LEN;

    scp_ipi_send(IPI_LOGGER, (void *)&log_info, sizeof(log_info), 1, IPI_SCP2AP);

    scp_mobile_log_ready = 1;
    PRINTF_E("scp_log_buf address: 0x%x\n",scp_log_buf);
}

static int send_buf_full_ipi_no_wait(void)
{
    unsigned int magic = 0x5A5A5A5A;
    if (scp_ipi_send(IPI_BUF_FULL, &magic, sizeof(magic), 0, IPI_SCP2AP) == DONE)
        return 1;
    return 0;
}

void logger_putc(char c)
{
    unsigned int log_start, log_end, threshold, len, nosend;

#define LOG_BUF_MASK (LOG_LEN - 1)
#define LOG_BUF(idx) (scp_log_buf[(idx) & LOG_BUF_MASK])
#define UNSIGNED_INT_MAX	0xFFFFFFFF

    if (scp_mobile_log_ready == 0)
        return ;

    LOG_BUF(scp_log_end++) = c;

    log_start = scp_log_start;
    log_end = scp_log_end;

    len = log_end - log_start;

    if (log_end < log_start)
        len = UNSIGNED_INT_MAX - len + 1;

    threshold = LOG_LEN >> 1;
    nosend = 0;

    if (len >= threshold) {
        if (len >= LOG_LEN) {
            if (log_end < LOG_LEN)
                log_start = UNSIGNED_INT_MAX - LOG_LEN + log_end;
            else
                log_start = log_end - LOG_LEN;

            if (!scp_log_lock)
                scp_log_start = log_start;

            // avoid boundary case: when buffer is bull, each char will send an IPI to AP
            if (len > LOG_LEN && (log_end & 0x3F) != 0)
                nosend = 1;

            len = LOG_LEN;
        }

        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED &&
                xTaskGetSchedulerState() != taskSCHEDULER_SUSPENDED &&
                enable_scp_mobile_log == 1 &&
                scp_mobile_log_ready == 1 &&
                (len & (threshold - 1)) == 0 &&
                nosend == 0) {
            if (is_in_isr()) {
                /*in isr*/
                xTimerPendFunctionCallFromISR((PendedFunction_t) send_buf_full_ipi_no_wait,
                                              NULL,
                                              0,
                                              NULL );
            } else {
                /*not in isr*/
                xTimerPendFunctionCall((PendedFunction_t) send_buf_full_ipi_no_wait,
                                       NULL,
                                       0,
                                       10 );
            }
        }
    }
}
