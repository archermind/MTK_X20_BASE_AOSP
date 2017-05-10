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

#include <FreeRTOS.h>
#include <platform.h>
#include <task.h>
#include <mpu.h>

extern UBaseType_t uxTaskGetEndOfStack(TaskHandle_t xTask);

static mpu_region_t mpu_region_table[8] = {
    {0x20000000, MPU_SIZE_4GB, MPU_AP_PRI_RW_ACCESS_UNPRI_RW_ACCESS, MPU_TEX, MPU_C_ENABLE, MPU_B_DISABLE, MPU_S_ENABLE, MPU_ENABLE},
    {0x0, MPU_SIZE_512MB, MPU_AP_PRI_RO_ACCESS_UNPRI_RO_ACCESS, MPU_TEX, MPU_C_ENABLE, MPU_B_DISABLE, MPU_S_ENABLE, MPU_ENABLE},
    {0x0, MPU_SIZE_512KB, MPU_AP_PRI_RW_ACCESS_UNPRI_RW_ACCESS, MPU_TEX, MPU_C_ENABLE, MPU_B_DISABLE, MPU_S_ENABLE, MPU_ENABLE},
    {0x0, 0, 0, 0, MPU_DISABLE},
    {0x0, 0, 0, 0, MPU_DISABLE},
    {0x0, 0, 0, 0, MPU_DISABLE},
    {0xE000ED80, MPU_SIZE_512KB, MPU_AP_PRI_RO_ACCESS_UNPRI_NO_ACCESS, MPU_TEX, MPU_C_ENABLE, MPU_B_DISABLE, MPU_S_ENABLE, MPU_ENABLE},
    {0x0, MPU_SIZE_2KB, MPU_AP_PRI_RO_ACCESS_UNPRI_RO_ACCESS, MPU_TEX, MPU_C_ENABLE, MPU_B_DISABLE, MPU_S_ENABLE, MPU_ENABLE},
};
void enable_mpu_region(uint32_t region_num)
{
    if (region_num < 0 || region_num >= MAX_NUM_REGIONS) {
        PRINTF_E("region_num:%d is wrong\n", region_num);
        return;
    }
    MPU->RNR = region_num;
    MPU->RASR |= MPU_RASR_ENABLE_Msk;
    mpu_region_table[region_num].enable = MPU_ENABLE;
    __ISB();
}
void disable_mpu_region(uint32_t region_num)
{
    if (region_num < 0 || region_num >= MAX_NUM_REGIONS) {
        PRINTF_E("region_num:%d is wrong\n", region_num);
        return;
    }
    MPU->RNR = region_num;
    MPU->RASR &= ~MPU_RASR_ENABLE_Msk;
    mpu_region_table[region_num].enable = MPU_DISABLE;
    __ISB();
}
void setup_mpu_region(uint32_t region_num,  mpu_region_t region_setting)
{
    uint32_t region_attribute;

    if (region_num < 0 || region_num >= MAX_NUM_REGIONS) {
        PRINTF_E("region_num:%d is wrong\n", region_num);
        return;
    }
    if (mpu_region_table[region_num].enable == MPU_ENABLE) {
        PRINTF_E("MPU-%d already enable, disable it first.\n", region_num);
        return;
    }
    MPU->RNR = region_num;
    MPU->RBAR = region_setting.base_address;
    region_attribute = ((region_setting.length << MPU_RASR_SIZE_Pos) & MPU_RASR_SIZE_Msk) | \
                       ((region_setting.access_permission << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk) | \
                       ((region_setting.TEX << MPU_RASR_TEX_Pos) & MPU_RASR_TEX_Msk) | \
                       ((region_setting.C << MPU_RASR_C_Pos) & MPU_RASR_C_Msk) | \
                       ((region_setting.B << MPU_RASR_B_Pos) & MPU_RASR_B_Msk) | \
                       ((region_setting.S << MPU_RASR_S_Pos) & MPU_RASR_S_Msk) ;

    MPU->RASR = region_attribute;
    PRINTF_D("MPU-%d region setting:0x%x, 0x%x\n", region_num, region_setting.base_address, region_attribute);
}
void dump_mpu_status(void)
{
    uint32_t regions;
    uint32_t ix;

    regions = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
    PRINTF_D("TYPE\tCTRL\t\n");
    PRINTF_D("0x%x\t0x%x\t\n", MPU->TYPE, MPU->CTRL);
    PRINTF_D("RNR\tRBAR\tRASR\n");
    for (ix = 0; ix < regions; ix++) {
        MPU->RNR = ix;
        PRINTF_D("0x%x\t0x%x\t0x%x\n", MPU->RNR, MPU->RBAR, MPU->RASR);
    }
}
int32_t request_mpu_region(void)
{
    uint32_t max_regions;
    uint32_t region_num = -1;
    uint32_t ix;

    max_regions = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
    for (ix = 0; ix < max_regions; ix++) {
        if (mpu_region_table[ix].enable == MPU_DISABLE) {
            region_num = ix;
            break;
        }
    }
    return region_num;
}
int32_t stack_protector(TaskHandle_t task_handler)
{
    uint32_t stack_end_address;
    uint32_t region_num;
    mpu_region_t region_setting;

    region_num = request_mpu_region();

    if (region_num < 0 || region_num >= MAX_NUM_REGIONS) {
        PRINTF_E("region_num:%d is wrong\n", region_num);
        return -1;
    }
    stack_end_address = uxTaskGetEndOfStack(task_handler);
    PRINTF_D("stack address:0x%x\n", stack_end_address);
    region_setting.base_address = stack_end_address & (~0x1fUL); /*MPU has minimum 32byte limitation*/
    region_setting.length = MPU_SIZE_32B;
    region_setting.access_permission = MPU_AP_PRI_RO_ACCESS_UNPRI_RO_ACCESS;
    region_setting.TEX = MPU_TEX;
    region_setting.C = MPU_C_ENABLE;
    region_setting.B = MPU_B_DISABLE;
    region_setting.S = MPU_S_ENABLE;
    setup_mpu_region(region_num, region_setting);
    enable_mpu_region(region_num);

    return 0;
}
/*
 * To keep dram access which match the expectation
 * */
#define DRAM_REGION_NR (2)
static uint32_t dram_region_num[DRAM_REGION_NR];
int32_t dram_protector_init(void)
{
    uint32_t dram_base_address[DRAM_REGION_NR] = {0x20000000, 0x70000000};
    uint32_t dram_size[DRAM_REGION_NR] = {0x20000000, 0x20000000};
    mpu_region_t region_setting[DRAM_REGION_NR];
    int ix;

    for (ix = 0; ix < DRAM_REGION_NR; ix++) {
        dram_region_num[ix] = request_mpu_region();
        if (dram_region_num[ix] < 0 || dram_region_num[ix] >= MAX_NUM_REGIONS) {
            PRINTF_E("dram_region_num:%d is wrong\n", dram_region_num[ix]);
            return -1;
        }
        PRINTF_D("dram[%d] start address:0x%x\n", ix, dram_base_address[ix]);
        PRINTF_D("dram[%d] size:0x%x\n", ix, dram_size[ix]);
        region_setting[ix].base_address = dram_base_address[ix] & (~0x1fUL); /*MPU has minimum 32byte limitation*/
        region_setting[ix].length = 31 - 2 - __CLZ(dram_size[ix]);
        region_setting[ix].access_permission = MPU_AP_PRI_RW_ACCESS_UNPRI_RW_ACCESS;
        region_setting[ix].TEX = MPU_TEX;
        region_setting[ix].C = MPU_C_ENABLE;
        region_setting[ix].B = MPU_B_DISABLE;
        region_setting[ix].S = MPU_S_ENABLE;
        setup_mpu_region(dram_region_num[ix], region_setting[ix]);
        enable_mpu_region(dram_region_num[ix]);
        MPU->RNR = dram_region_num[ix];
        PRINTF_D("[MPU] 0x%x\t0x%x\t0x%x\n", MPU->RNR, MPU->RBAR, MPU->RASR);
    }
    return 0;
}
void enable_dram_protector(void)
{
    int ix;

    for (ix = 0; ix < DRAM_REGION_NR; ix++) {
        enable_mpu_region(dram_region_num[ix]);
    }
    return ;
}
void disable_dram_protector(void)
{
    int ix;

    for (ix = 0; ix < DRAM_REGION_NR; ix++) {
        disable_mpu_region(dram_region_num[ix]);
    }
    return ;
}
