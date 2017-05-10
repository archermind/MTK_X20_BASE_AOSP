/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

/******************************************************************************
*
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2001
*
*******************************************************************************/

#ifndef ATF_LOG_DRV_H
#define ATF_LOG_DRV_H

#include <spinlock.h>
#include <stdint.h>
#include <platform_def.h>

#define ATF_LOG_CTRL_BUF_SIZE 256
#define ATF_LOG_SIGNAL_THRESHOLD_SIZE 1024

#define ATF_CRASH_MAGIC_NO	0xdead1abf
#define ATF_LAST_LOG_MAGIC_NO	0x41544641
/*
  ___________________________
 |                           |
 | ATF crash reserved buffer |
 |___________________________|

  Total reserved buffer size = ATF_CRASH_LAST_LOG_SIZE + ATF_EXCEPT_BUF_SIZE

 +--------------------------------+--------------------------------------------------------+
 |    ATF_CRASH_LAST_LOG_SIZE     |    ATF_EXCEPT_BUF_SIZE_PER_CPU * PLATFORM_CORE_COUNT   |
 +--------------------------------+--------------------------------------------------------+
 |    Last ATF log for crash      | CPU-0 | CPU-1| CPU-2 | CPU-3 |...PLATFORM_CORE_COUNT-1 |
 +--------------------------------+--------------------------------------------------------+
*/
#define ATF_CRASH_LAST_LOG_SIZE (32*1024)
#define ATF_EXCEPT_BUF_SIZE_PER_CPU (4*1024)
#define ATF_EXCEPT_BUF_SIZE (ATF_EXCEPT_BUF_SIZE_PER_CPU * PLATFORM_CORE_COUNT)

typedef union atf_log_ctrl
{
    struct
    {
        unsigned int atf_buf_addr;          /*  0x0 */
        unsigned int atf_buf_size;
        unsigned int atf_write_pos;
        unsigned int atf_read_pos;
        spinlock_t atf_buf_lock;            /* 0x10 */
        unsigned int atf_buf_unread_size;
        unsigned int atf_irq_count;
        unsigned int atf_reader_alive;
        uint64_t atf_total_write_count;     /* 0x20 */
        uint64_t atf_total_read_count;
        unsigned int atf_aee_dbg_buf_addr;  /* 0x30 */
        unsigned int atf_aee_dbg_buf_size;
        unsigned int atf_crash_log_addr;
        unsigned int atf_crash_log_size;
        unsigned int atf_crash_flag;        /* 0x40 */
        unsigned int padding;  /* padding for next 8 bytes alignment variable */
        uint64_t atf_except_write_pos_per_cpu[PLATFORM_CORE_COUNT]; /* 0x48 */
    } info;
    unsigned char data[ATF_LOG_CTRL_BUF_SIZE];
} atf_log_ctrl_t;

void mt_log_setup(unsigned int start, unsigned int size, unsigned int aee_buf_size);
int mt_log_lock_acquire();
int mt_log_write(unsigned char c);
int mt_log_lock_release();
void mt_log_suspend_flush();
void mt_log_secure_os_print(int c);

#ifdef MTK_ATF_RAM_DUMP
extern uint64_t	atf_ram_dump_base;
extern uint64_t	atf_ram_dump_size;
#endif

#endif
