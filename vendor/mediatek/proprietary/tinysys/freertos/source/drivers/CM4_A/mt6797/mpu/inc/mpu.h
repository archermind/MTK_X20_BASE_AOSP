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

#ifndef _MPU_H_
#define _MPU_H_

#define MAX_NUM_REGIONS (8)

typedef struct {
    uint32_t base_address;
    uint32_t length;
    uint32_t access_permission;
    uint32_t TEX;
    uint32_t C;
    uint32_t B;
    uint32_t S;
    uint32_t enable;
} mpu_region_t;
enum {
    MPU_AP_PRI_NO_ACCESS_UNPRI_NO_ACCESS = 0UL,
    MPU_AP_PRI_RW_ACCESS_UNPRI_NO_ACCESS = 1UL,
    MPU_AP_PRI_RW_ACCESS_UNPRI_RO_ACCESS = 2UL,
    MPU_AP_PRI_RW_ACCESS_UNPRI_RW_ACCESS = 3UL,
    MPU_AP_RESERVE			 = 4UL,
    MPU_AP_PRI_RO_ACCESS_UNPRI_NO_ACCESS = 5UL,
    MPU_AP_PRI_RO_ACCESS_UNPRI_RO_ACCESS = 6UL,
    MPU_AP_PRI_RO_ACCESS_UNPRI_RO_ACCESS_2 = 7UL,
};
enum {
    MPU_TEX = 0UL,
};
enum {
    MPU_C_DISABLE= 0UL,
    MPU_C_ENABLE= 1UL,
};
enum {
    MPU_B_DISABLE= 0UL,
    MPU_B_ENABLE= 1UL,
};
enum {
    MPU_S_DISABLE= 0UL,
    MPU_S_ENABLE= 1UL,
};
enum {
    MPU_SIZE_32B  = 4UL,
    MPU_SIZE_64B  = 5UL,
    MPU_SIZE_128B = 6UL,
    MPU_SIZE_1KB  = 9UL,
    MPU_SIZE_2KB  = 10UL,
    MPU_SIZE_512KB  = 18UL,
    MPU_SIZE_1MB  = 19UL,
    MPU_SIZE_512MB  = 28UL,
    MPU_SIZE_1GB  = 29UL,
    MPU_SIZE_4GB  = 31UL,
};
enum {
    MPU_DISABLE = 0UL,
    MPU_ENABLE = 1UL,
};

void dump_mpu_status(void);
void enable_mpu_region(uint32_t region_num);
void disable_mpu_region(uint32_t region_num);
void setup_mpu_region(uint32_t region_num,mpu_region_t region_setting);
int32_t stack_protector(TaskHandle_t task_handler);
int32_t dram_protector_init(void);
void enable_dram_protector(void);
void disable_dram_protector(void);
#endif


