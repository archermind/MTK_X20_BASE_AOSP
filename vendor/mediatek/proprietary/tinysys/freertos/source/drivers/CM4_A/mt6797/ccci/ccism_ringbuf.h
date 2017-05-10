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

#ifndef __CCCI_RINGBUF_H__
#define __CCCI_RINGBUF_H__

#include "ccci.h"

typedef enum {
    CCCI_RINGBUF_OK = 0x100,
    CCCI_RINGBUF_PARAM_ERR,
    CCCI_RINGBUF_NOT_ENOUGH,
    CCCI_RINGBUF_BAD_HEADER,
    CCCI_RINGBUF_BAD_FOOTER,
    CCCI_RINGBUF_NOT_COMPLETE,
    CCCI_RINGBUF_EMPTY,
} ccci_ringbuf_error;

struct ccci_ringbuf {
    struct {
        unsigned int read;
        unsigned int write;
        unsigned int length;
    } rx_control, tx_control;
    unsigned char buffer[0];
};
#define CCCI_RINGBUF_CTL_LEN (8+sizeof(struct ccci_ringbuf)+8)

int ccci_ringbuf_readable(int md_id, struct ccci_ringbuf *ringbuf);
int ccci_ringbuf_writeable(int md_id, struct ccci_ringbuf *ringbuf, unsigned int write_size);
struct ccci_ringbuf *ccci_create_ringbuf(int md_id, unsigned char *buf, int buf_size, int rx_size, int tx_size);
int ccci_ringbuf_read(int md_id, struct ccci_ringbuf *ringbuf, unsigned char *buf, int read_size);
int ccci_ringbuf_write(int md_id, struct ccci_ringbuf *ringbuf, unsigned char *ccci_h,
                       unsigned char *data, int data_len);
void ccci_ringbuf_move_rpointer(int md_id, struct ccci_ringbuf *ringbuf, int read_size);
void ccci_ringbuf_reset(int md_id, struct ccci_ringbuf *ringbuf, int dir);
#endif              /* __CCCI_RINGBUF_H__ */
