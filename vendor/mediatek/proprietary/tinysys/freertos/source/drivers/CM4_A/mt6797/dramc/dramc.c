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

#include "main.h"
/*   Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <driver_api.h>
#include <dramc.h>
#include <string.h>
#include <interrupt.h>
#include <platform.h>
#include <utils.h>
#include <vcore_dvfs.h>
#include <tinysys_config.h>

/*
 * global variables
 */

#if defined(CFG_DRAMC_MONITOR_SUPPORT) && defined(TINYSYS_DEBUG_BUILD)
static void vTaskDramMonitor( void *pvParameters );
uint32_t ddrphy_base[4] = { 0xA000f000, 0xA0012000, 0xA0018000, 0xA0019000 };
uint32_t dramc_nao_base[2] = { DRAMCNAO_CHA_BASE_ADDR, DRAMCNAO_CHB_BASE_ADDR };
#define SRAM_BUFFER_BASE	0x9012DC40
#define mainCHECK_DELAY     ( ( portTickType) 10 / portTICK_RATE_MS )
#define PI_CRITERIA		12
#define PRINT_LOG 0
#define SHOW_SRAM_CONTENT 0
#define TIME_PROFILE 0

/*
Tracking	2T_UI/0.5T_UI	PI
Rank0-B0	0x37c[7:0]		0x374[7:0]
Rank0-B1	0x37c[15:8]		0x374[15:8]
Rank0-B2	0x37c[23:16]	0x374[23:16]
Rank0-B3	0x37c[31:24]	0x374[31:24]
Rank1-B0	0x380[7:0]		0x378[7:0]
Rank1-B1	0x380[15:8]		0x378[15:8]
Rank1-B2	0x380[23:16]	0x378[23:16]
Rank1-B3	0x380[31:24]	0x378[31:24]
*/

void calculate_tracking_pi( uint32_t src_ui, uint32_t src_pi, uint32_t* dst_pi )
{
    dst_pi[0] = ((src_ui & 0x000000ff) >>  0) * 32 + ((src_pi & 0x000000ff) >>  0);
    dst_pi[1] =	((src_ui & 0x0000ff00) >>  8) * 32 + ((src_pi & 0x0000ff00) >>  8);
    dst_pi[2] = ((src_ui & 0x00ff0000) >> 16) * 32 + ((src_pi & 0x00ff0000) >> 16);
    dst_pi[3] =	((src_ui & 0xff000000) >> 24) * 32 + ((src_pi & 0xff000000) >> 24);
}

static void vTaskDramMonitor( void *pvParameters )
{
    portTickType xLastExecutionTime, xDelayTime;
    uint32_t i, j, k, save_log, save_first_log, loop_count, buffer_index, error_happen;
    uint16_t error_flag[4][4];			// 4 DDRPHY
    uint32_t tracking_value[2][2][2]; 	// 2 Channel / 2 Rank
    uint32_t tracking_pi[2][2][4];		// 2 Channel / 2 Rank / 4 Byte
    uint32_t first_pi[3][2][2][4];		// 3 frequency / 2 Channel / 2 Rank / 4 Byte
    DRAMC_DEBUG_INFO_T* debug_info_ptr;
    int8_t first_done[3];
    uint8_t freq_idx;
    uint16_t pi_diff, first_byte_pi, track_byte_pi;

#if TIME_PROFILE
    //volatile unsigned int *ITM_CONTROL = (unsigned int *)0xE0000E80;
    volatile unsigned int *DWT_CONTROL = (unsigned int *)0xE0001000;
    volatile unsigned int *DWT_CYCCNT = (unsigned int *)0xE0001004;
    volatile unsigned int *DEMCR = (unsigned int *)0xE000EDFC;

#define CPU_RESET_CYCLECOUNTER() \
do { \
*DEMCR = *DEMCR | 0x01000000; \
*DWT_CYCCNT = 0; \
*DWT_CONTROL = *DWT_CONTROL | 1 ; \
} while(0)

    uint32_t start_time, end_time;
#endif

    xLastExecutionTime = xTaskGetTickCount();
    xDelayTime = mainCHECK_DELAY;
    debug_info_ptr = (DRAMC_DEBUG_INFO_T*) SRAM_BUFFER_BASE;

    buffer_index = loop_count = 0;
    debug_info_ptr->id[0] = 'D';
    debug_info_ptr->id[1] = 'R';
    debug_info_ptr->id[2] = 'A';
    debug_info_ptr->id[3] = 'M';

    for (i=0; i<3; i++)
        first_done[i] = 0;

    error_happen = 0;

    do {

#if TIME_PROFILE
        CPU_RESET_CYCLECOUNTER();
        start_time = *DWT_CYCCNT;
#endif

        loop_count++;
        save_log = 0;
        save_first_log = 0;

        //if no infra clock, do nothing
        if(check_infra_state() == 0) {
            goto just_wait;
        }

        enable_clk_bus(RTOS_MEM_ID);

        //fetch error flag
        for(i = 0; i < 4; i++) {
            error_flag[i][0] = drv_reg32(ddrphy_base[i] + 0xfc0) & 0xffff;
            error_flag[i][1] = drv_reg32(ddrphy_base[i] + 0xfc4) & 0xffff;
            error_flag[i][2] = drv_reg32(ddrphy_base[i] + 0xfc8) & 0xffff;
            error_flag[i][3] = drv_reg32(ddrphy_base[i] + 0xfcc) & 0xffff;
            if ((error_flag[i][0] != 0) ||
                    (error_flag[i][1] != 0) ||
                    (error_flag[i][2] != 0) ||
                    (error_flag[i][3] != 0)) {
                error_happen = 1;
                save_log = 1;
            }
        }

        //get tracking value of each rank of two channels
        for(i = 0; i < 2; i++) {
            tracking_value[i][0][0] = drv_reg32(dramc_nao_base[i] + 0x374);  //rank0 PI
            tracking_value[i][0][1] = drv_reg32(dramc_nao_base[i] + 0x37c);  //rank0 UI (P0)
            tracking_value[i][1][0] = drv_reg32(dramc_nao_base[i] + 0x378);  //rank1 PI
            tracking_value[i][1][1] = drv_reg32(dramc_nao_base[i] + 0x380);  //rank1 UI (P0)
        }

        // 	0x10004028[13:12]
        //	00: 1600
        //	01: 1270
        //	10: 1066
        freq_idx = (drv_reg32(DRAMCAO_CHA_BASE_ADDR + 0x028) & 0x3000) >> 12;

        if(first_done[freq_idx] == 0) {
            //calculate first tacking pi value for each byte of each rank of each channel
            for(i = 0; i < 2; i++)
                for(j = 0; j < 2; j++)
                    calculate_tracking_pi( tracking_value[i][j][1], tracking_value[i][j][0], first_pi[freq_idx][i][j]);

            first_done[freq_idx] = 1;
            save_first_log = 1;
        } else {
            //calculate tracking pi value for each byte of each rank of each channel
            for(i = 0; i < 2; i++)	//for channel
                for(j = 0; j < 2; j++) {	//for rank
                    calculate_tracking_pi( tracking_value[i][j][1], tracking_value[i][j][0], tracking_pi[i][j]);

                    for(k = 0; k < 4; k++) {
                        first_byte_pi = first_pi[freq_idx][i][j][k];
                        track_byte_pi = tracking_pi[i][j][k];
                        if(first_byte_pi > track_byte_pi)
                            pi_diff = first_byte_pi - track_byte_pi;
                        else
                            pi_diff = track_byte_pi - first_byte_pi;
                        if(pi_diff > PI_CRITERIA) {
                            save_log = 1;
#if PRINT_LOG
                            PRINTF_D("[DRAMC]ch=%d, rank=%d, byte=%d, first_pi=%d, track_pi=%d\n", i, j, k, first_byte_pi, track_byte_pi);
#endif
                        }
                    }
                }
        }

        if((save_log == 1) || (save_first_log == 1)) {
            //store data to SRAM buffer
            if(error_happen == 1)
                for(i = 0; i < 4; i++)
                    for(j = 0; j < 4; j++)
                        debug_info_ptr->error_flag[i][j] = error_flag[i][j];

            if(save_first_log == 1) {
                debug_info_ptr->freq_idx[freq_idx] = freq_idx;
                debug_info_ptr->loop_count[freq_idx] = loop_count;

                for(i = 0; i < 2; i++)
                    for(j = 0; j < 2; j++)
                        for(k = 0; k < 2; k++)
                            debug_info_ptr->first_tracking_value[freq_idx][i][j][k] = tracking_value[i][j][k];
#if PRINT_LOG
                PRINTF_D("[DRAMC]loop_count = %d, first log for freq_idx = %d\n", loop_count, freq_idx);
#endif
            } else if(save_log == 1) {
                debug_info_ptr->buffer_index = buffer_index;
                debug_info_ptr->freq_idx[3+buffer_index] = freq_idx;
                debug_info_ptr->loop_count[3+buffer_index] = loop_count;

                for(i = 0; i < 2; i++)
                    for(j = 0; j < 2; j++)
                        for(k = 0; k < 2; k++)
                            debug_info_ptr->tracking_value[buffer_index][i][j][k] = tracking_value[i][j][k];
#if PRINT_LOG
                PRINTF_D("[DRAMC]loop_count = %d, buffer_index = %d, freq_idx = %d\n", loop_count, buffer_index, freq_idx);
#endif
                buffer_index++;
            }
#if PRINT_LOG
            //dump error flags
            if(error_happen == 1) {
#if !SHOW_SRAM_CONTENT
                PRINTF_D("[DRAMC] error_flag = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
                         error_flag[0][0], error_flag[0][1], error_flag[0][2], error_flag[0][3],
                         error_flag[1][0], error_flag[1][1], error_flag[1][2], error_flag[1][3],
                         error_flag[2][0], error_flag[2][1], error_flag[2][2], error_flag[2][3],
                         error_flag[3][0], error_flag[3][1], error_flag[3][2], error_flag[3][3]);
#else
                PRINTF_D("[DRAMC] error_flag = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
                         debug_info_ptr->error_flag[0][0], debug_info_ptr->error_flag[0][1], debug_info_ptr->error_flag[0][2], debug_info_ptr->error_flag[0][3],
                         debug_info_ptr->error_flag[1][0], debug_info_ptr->error_flag[1][1], debug_info_ptr->error_flag[1][2], debug_info_ptr->error_flag[1][3],
                         debug_info_ptr->error_flag[2][0], debug_info_ptr->error_flag[2][1], debug_info_ptr->error_flag[2][2], debug_info_ptr->error_flag[2][3],
                         debug_info_ptr->error_flag[3][0], debug_info_ptr->error_flag[3][1], debug_info_ptr->error_flag[3][2], debug_info_ptr->error_flag[3][3]);
#endif
            }

            //dump gating values
#if !SHOW_SRAM_CONTENT
            PRINTF_D("[DRAMC]tracking_value[0] = 0x%08x, 0x%08x, 0x%08x, 0x%08x\n [DRAMC]tracking_value[1] = 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
                     tracking_value[0][0][0], tracking_value[0][0][1], tracking_value[0][1][0], tracking_value[0][1][1],
                     tracking_value[1][0][0], tracking_value[1][0][1], tracking_value[1][1][0], tracking_value[1][1][1]);
#else
            if(save_first_log == 1) {
                PRINTF_D("[DRAMC]tracking_value[0] = 0x%08x, 0x%08x, 0x%08x, 0x%08x\n [DRAMC]tracking_value[1] = 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
                         debug_info_ptr->first_tracking_value[freq_idx][0][0][0],
                         debug_info_ptr->first_tracking_value[freq_idx][0][0][1],
                         debug_info_ptr->first_tracking_value[freq_idx][0][1][0],
                         debug_info_ptr->first_tracking_value[freq_idx][0][1][1],
                         debug_info_ptr->first_tracking_value[freq_idx][1][0][0],
                         debug_info_ptr->first_tracking_value[freq_idx][1][0][1],
                         debug_info_ptr->first_tracking_value[freq_idx][1][1][0],
                         debug_info_ptr->first_tracking_value[freq_idx][1][1][1]);
            } else if(save_log == 1) {
                PRINTF_D("[DRAMC]tracking_value[0] = 0x%08x, 0x%08x, 0x%08x, 0x%08x\n [DRAMC]tracking_value[1] = 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
                         debug_info_ptr->tracking_value[buffer_index-1][0][0][0],
                         debug_info_ptr->tracking_value[buffer_index-1][0][0][1],
                         debug_info_ptr->tracking_value[buffer_index-1][0][1][0],
                         debug_info_ptr->tracking_value[buffer_index-1][0][1][1],
                         debug_info_ptr->tracking_value[buffer_index-1][1][0][0],
                         debug_info_ptr->tracking_value[buffer_index-1][1][0][1],
                         debug_info_ptr->tracking_value[buffer_index-1][1][1][0],
                         debug_info_ptr->tracking_value[buffer_index-1][1][1][1]);
            }
#endif
#endif
            if(buffer_index >= (MAX_SIZE-3))
                buffer_index = 0;
        }

        disable_clk_bus(RTOS_MEM_ID);
        if(error_happen == 1)
            break;

#if TIME_PROFILE
        end_time = *DWT_CYCCNT;
        PRINTF_D("[DRAMC]loop_count = %d, start_time = 0x%x, end_time = 0x%x, time_period = %d (%d us)\n", loop_count, start_time, end_time, end_time - start_time, (end_time - start_time) / 112);
#endif

just_wait:
        vTaskDelayUntil( &xLastExecutionTime, xDelayTime );
    } while (1);
}
#endif

/*
 * mt_init_dramc.
 * Always return 0.
 */
int32_t mt_init_dramc(void)
{
#if defined(CFG_DRAMC_MONITOR_SUPPORT) && defined(TINYSYS_DEBUG_BUILD)
    xTaskCreate( vTaskDramMonitor, "DTMon", 384, NULL, 1, NULL );
#endif
    PRINTF("Init DRAMC OK\n");

    return 0;
}
