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
 * MediaTek Inc. (C) 2014. All rights reserved.
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

#include <arch.h>
#include <arch_helpers.h>
#include <assert.h>
#include <runtime_svc.h>
#include <debug.h>
#include <sip_svc.h>
#include <sip_error.h>
#include <platform.h>
#include <mmio.h>
#include <console.h> //set_uart_flag(), clear_uart_flag();
#include "plat_private.h"   //for atf_arg_t_ptr
#include "sip_private.h"
#include "scp.h"
#include "mt_cpuxgpt.h"

#include "mt_idvfs_api.h"
#include "mt_ocp_api.h"

#include <xlat_tables.h>
#include <emi_drv.h>
#include <log.h>
#include <cache/plat_cache.h>
#define ATF_OCP_DREQ 1  // PTP3 OCP + DREQ function

extern void dfd_disable(void);

/*******************************************************************************
 * SIP top level handler for servicing SMCs.
 ******************************************************************************/

static struct kernel_info k_info;

static void save_kernel_info(uint64_t pc, uint64_t r0, uint64_t r1,
                                                       uint64_t k32_64)
{
    k_info.k32_64 = k32_64;
    k_info.pc=pc;

    if ( LINUX_KERNEL_32 == k32_64 ) {
        /* for 32 bits kernel */
        k_info.r0=0;
        k_info.r1=r0;   /* machtype */
        k_info.r2=r1;   /* tags */
    } else {
        /* for 64 bits kernel */
        k_info.r0=r0;
        k_info.r1=r1;
    }
}

static void set_kernel_k32_64(uint64_t k32_64)
{
    k_info.k32_64 = k32_64;
}

uint64_t get_kernel_k32_64(void)
{
    return k_info.k32_64;
}

uint64_t get_kernel_info_pc(void)
{
    return k_info.pc;
}

uint64_t get_kernel_info_r0(void)
{
    return k_info.r0;
}

uint64_t get_kernel_info_r1(void)
{
    return k_info.r1;
}
uint64_t get_kernel_info_r2(void)
{
    return k_info.r2;
}

extern void bl31_prepare_kernel_entry(uint64_t k32_64);
extern void el3_exit(void);
extern uint64_t sip_write_md_regs(uint32_t cmd_type, uint32_t value1,uint32_t value2, uint32_t value3);
extern unsigned long g_dormant_log_base;

/*******************************************************************************
 * SMC Call for Kernel MCUSYS register write
 ******************************************************************************/

static uint64_t mcusys_write_count = 0;
static uint64_t sip_mcusys_write(unsigned int reg_addr, unsigned int reg_value)
{
    if((reg_addr & 0xFFFF0000) != (MCUCFG_BASE & 0xFFFF0000))
        return SIP_SVC_E_INVALID_Range;

    /* Perform range check */
    if(( MP0_MISC_CONFIG0 <= reg_addr && reg_addr <= MP0_MISC_CONFIG9 ) ||
       ( MP1_MISC_CONFIG0 <= reg_addr && reg_addr <= MP1_MISC_CONFIG9 )) {
        return SIP_SVC_E_PERMISSION_DENY;
    }

    if (check_cpuxgpt_write_permission(reg_addr, reg_value) == 0) {
		/* Not allow to clean enable bit[0], Force to set bit[0] as 1 */
		reg_value |= 0x1;
    }

    mmio_write_32(reg_addr, reg_value);
    dsb();

    mcusys_write_count++;

    return SIP_SVC_E_SUCCESS;
}
/*******************************************************************************
 * SIP top level handler for servicing SMCs.
 ******************************************************************************/
uint64_t sip_smc_handler(uint32_t smc_fid,
			  uint64_t x1,
			  uint64_t x2,
			  uint64_t x3,
			  uint64_t x4,
			  void *cookie,
			  void *handle,
			  uint64_t flags)
{
    uint64_t rc = 0;
    uint32_t ns;
    atf_arg_t_ptr teearg = &gteearg;

    /* Determine which security state this SMC originated from */
    ns = is_caller_non_secure(flags);

    //WARN("sip_smc_handler\n");
    //WARN("id=0x%llx\n", smc_fid);
    //WARN("x1=0x%llx, x2=0x%llx, x3=0x%llx, x4=0x%llx\n", x1, x2, x3, x4);

    switch (smc_fid) {
    case MTK_SIP_TBASE_HWUID_AARCH32:
        {
        if (ns)
            SMC_RET1(handle, SMC_UNK);
        SMC_RET4(handle, teearg->hwuid[0], teearg->hwuid[1],
            teearg->hwuid[2], teearg->hwuid[3]);
        break;
        }
    case MTK_SIP_KERNEL_MCUSYS_WRITE_AARCH32:
    case MTK_SIP_KERNEL_MCUSYS_WRITE_AARCH64:
        rc = sip_mcusys_write(x1, x2);
        break;
    case MTK_SIP_KERNEL_MCUSYS_ACCESS_COUNT_AARCH32:
    case MTK_SIP_KERNEL_MCUSYS_ACCESS_COUNT_AARCH64:
        rc = mcusys_write_count;
        break;
    case MTK_SIP_KERNEL_BOOT_AARCH32:
        wdt_kernel_cb_addr = 0;
        set_uart_flag();
        printf("save kernel info\n");
        save_kernel_info(x1, x2, x3, x4);
        bl31_prepare_kernel_entry(x4);
        printf("el3_exit\n");
        clear_uart_flag();
        SMC_RET0(handle);
        break;
    case MTK_SIP_LK_MD_REG_WRITE_AARCH32:
    case MTK_SIP_LK_MD_REG_WRITE_AARCH64:
	    sip_write_md_regs((uint32_t)x1,(uint32_t)x2,(uint32_t)x3,(uint32_t)x4);
	    break;
    case MTK_SIP_LK_WDT_AARCH32:
    case MTK_SIP_LK_WDT_AARCH64:
        set_kernel_k32_64(LINUX_KERNEL_32);
        wdt_kernel_cb_addr = x1;
        printf("MTK_SIP_LK_WDT : 0x%lx \n", wdt_kernel_cb_addr);
        rc = teearg->atf_aee_debug_buf_start;
        break;
    case MTK_SIP_KERNEL_EMIMPU_WRITE_AARCH32:
    case MTK_SIP_KERNEL_EMIMPU_WRITE_AARCH64:
	    rc = sip_emimpu_write(x1, x2);
        break;

    case MTK_SIP_KERNEL_EMIMPU_READ_AARCH32:
    case MTK_SIP_KERNEL_EMIMPU_READ_AARCH64:
	    rc = (uint64_t)sip_emimpu_read(x1);
        break;
	case MTK_SIP_KERNEL_EMIMPU_SET_AARCH32:
	case MTK_SIP_KERNEL_EMIMPU_SET_AARCH64:
		//set_uart_flag();
		//printf("Ahsin sip_emimpu_set_region_protection x1=%x x2=%x  x3=%x\n",x1, x2, x3);
		rc = sip_emimpu_set_region_protection(x1, x2, x3);
		//clear_uart_flag();
		break;
#if DEBUG
    case MTK_SIP_KERNEL_GIC_DUMP_AARCH32:
    case MTK_SIP_KERNEL_GIC_DUMP_AARCH64:
	rc = mt_irq_dump_status(x1);
        break;
#endif
#ifdef MTK_ATF_RAM_DUMP
	case MTK_SIP_RAM_DUMP_ADDR_AARCH32:
		atf_ram_dump_base = x1<<32| (x2&0xffffffff);
		atf_ram_dump_size = x3<<32 | (x4&0xffffffff);
		break;
	case MTK_SIP_RAM_DUMP_ADDR_AARCH64:
		atf_ram_dump_base = x1;
		atf_ram_dump_size = x2;
		break;
#endif
    case MTK_SIP_DISABLE_DFD_AAACH32:
    case MTK_SIP_DISABLE_DFD_AARCH64:
		dfd_disable();
		rc = 0;
		break;
    case MTK_SIP_KERNEL_WDT_AARCH32:
    case MTK_SIP_KERNEL_WDT_AARCH64:
        wdt_kernel_cb_addr = x1;
        printf("MTK_SIP_KERNEL_WDT : 0x%lx \n", wdt_kernel_cb_addr);
        printf("teearg->atf_aee_debug_buf_start : 0x%x \n",
               teearg->atf_aee_debug_buf_start);
        rc = teearg->atf_aee_debug_buf_start;
        break;
    case MTK_SIP_KERNEL_MSG_AARCH32:
    case MTK_SIP_KERNEL_MSG_AARCH64:
        if (x1 == 0x0) { //set

		if (x2 == 1) {
			// g_dormant_tslog_base = x3;
		}
		if (x2 == 2) {
			g_dormant_log_base = x3;
		}
	}
	else if (x1 == 0x01) { //get

	}
	rc = SIP_SVC_E_SUCCESS;
	break;
    case MTK_SIP_KERNEL_SCP_RESET_AARCH32:
    case MTK_SIP_KERNEL_SCP_RESET_AARCH64:
        sip_reset_scp(x1);
        rc = 0;
        break;
#if 1
    case MTK_SIP_KERNEL_UDI_JTAG_CLOCK_AARCH32:
    case MTK_SIP_KERNEL_UDI_JTAG_CLOCK_AARCH64:
        rc = UDIRead(x1, x2);
        break;
    case MTK_SIP_KERNEL_IDVFS_BIGIDVFSENABLE_AARCH32:
    case MTK_SIP_KERNEL_IDVFS_BIGIDVFSENABLE_AARCH64:
        /* Fmax = 500 ~ 3000, Vproc_mv_x100 = 50000~120000, VSram_mv_x100 = 50000~120000 */
        rc = API_BIGIDVFSENABLE(x1, x2, x3);
        break;
    case MTK_SIP_KERNEL_IDVFS_BIGIDVFSDISABLE_AARCH32:
    case MTK_SIP_KERNEL_IDVFS_BIGIDVFSDISABLE_AARCH64:
        rc = API_BIGIDVFSDISABLE();
        break;
    case MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETFREQ_AARCH32:
    case MTK_SIP_KERNEL_IDVFS_BIGIDVFSPLLSETFREQ_AARCH64:
        /* x1 range = 500 ~ 3000 */
        rc = API_BIGPLLSETFREQ(x1);
        break;
    case MTK_SIP_KERNEL_IDVFS_BIGIDVFSSRAMLDOSET_AARCH32:
    case MTK_SIP_KERNEL_IDVFS_BIGIDVFSSRAMLDOSET_AARCH64:
        /* x1 range = 60000 ~ 120000 (mv_x100) */
        rc = BigSRAMLDOSet(x1);
        break;
    case MTK_SIP_KERNEL_IDVFS_IGIDVFSSWREQ_AARCH32:
    case MTK_SIP_KERNEL_IDVFS_IGIDVFSSWREQ_AARCH64:
        /* x1 swreq_reg value */
        rc = API_BIGIDVFSSWREQ(x1);
        break;
#endif

    case MTK_SIP_KERNEL_OCP_WRITE_AARCH32:
    case MTK_SIP_KERNEL_OCP_WRITE_AARCH64:
        /* OCP_BASE_ADDR= 0x10220000, only for secure reg 0x10220000 ~ 0x10224000 */
        if((x1 & 0xFFFFC000) != (OCP_BASE_ADDR & 0xFFFFC000))
            return SIP_SVC_E_INVALID_Range;
        mmio_write_32(x1, x2);
        /* prinf("MTK_SIP_KERNEL_OCP_WRITE : addr(0x%x) value(0x%x)\n", x1,x2); */
        break;
    case MTK_SIP_KERNEL_OCP_READ_AARCH32:
    case MTK_SIP_KERNEL_OCP_READ_AARCH64:
        /* OCP_BASE_ADDR= 0x10220000, only for secure reg 0x10220000 ~ 0x10224000 */
        if((x1 & 0xFFFFC000) != (OCP_BASE_ADDR & 0xFFFFC000))
            return SIP_SVC_E_INVALID_Range;
        rc = mmio_read_32(x1);
        /* prinf("MTK_SIP_KERNEL_OCP_READ : addr(0x%x) value(0x%x)\n", x1, rc); */
        break;

#if ATF_OCP_DREQ //OCP+DREQ
	case MTK_SIP_KERNEL_BIGOCPCONFIG_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPCONFIG_AARCH64:
         rc = BigOCPConfig(x1, x2);
	    break;
    case MTK_SIP_KERNEL_BIGOCPSETTARGET_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPSETTARGET_AARCH64:
	     rc = BigOCPSetTarget(x1, x2);
		 break;
	case MTK_SIP_KERNEL_BIGOCPENABLE1_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPENABLE1_AARCH64:
		 rc = BigOCPEnable(x1, 1, x2, x3);
	     break;
	case MTK_SIP_KERNEL_BIGOCPENABLE0_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPENABLE0_AARCH64:
		 rc = BigOCPEnable(x1, 0, x2, x3);
	     break;
    case MTK_SIP_KERNEL_BIGOCPDISABLE_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPDISABLE_AARCH64:
	     BigOCPDisable();
	     break;
    case MTK_SIP_KERNEL_BIGOCPINTLIMIT_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPINTLIMIT_AARCH64:
	     rc = BigOCPIntLimit(x1, x2);
	     break;
    case MTK_SIP_KERNEL_BIGOCPINTENDIS_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPINTENDIS_AARCH64:
	     rc = BigOCPIntEnDis(x1, x2);
	     break;
    case MTK_SIP_KERNEL_BIGOCPINTCLR_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPINTCLR_AARCH64:
 		 rc = BigOCPIntClr(x1, x2);
	     break;
   case MTK_SIP_KERNEL_BIGOCPAVGPWRGET_AARCH32:
   case MTK_SIP_KERNEL_BIGOCPAVGPWRGET_AARCH64:
	     rc = BigOCPAvgPwrGet(x1);
	     break;
    case MTK_SIP_KERNEL_BIGOCPCAPTURE1_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPCAPTURE1_AARCH64:
	     rc = BigOCPCapture(1, x1, x2, x3);
	     break;
    case MTK_SIP_KERNEL_BIGOCPCAPTURE0_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPCAPTURE0_AARCH64:
	     rc = BigOCPCapture(0, x1, x2, x3);
	     break;
//  case MTK_SIP_KERNEL_BIGOCPCAPTURESTATUS_AARCH32:
//  case MTK_SIP_KERNEL_BIGOCPCAPTURESTATUS_AARCH64:
//       rc = BigOCPCaptureStatus(x1, x2, x3);
//	     break;
    case MTK_SIP_KERNEL_BIGOCPCLKAVG_AARCH32:
    case MTK_SIP_KERNEL_BIGOCPCLKAVG_AARCH64:
		 rc = BigOCPClkAvg(x1, x2);
	     break;
//   case MTK_SIP_KERNEL_BIGOCPCLKAVGSTATUS_AARCH32:
//   case MTK_SIP_KERNEL_BIGOCPCLKAVGSTATUS_AARCH64:
//	     rc = BigOCPClkAvgStatus(x1);
//	     break;
     case MTK_SIP_KERNEL_LITTLEOCPCONFIG_AARCH32:
     case MTK_SIP_KERNEL_LITTLEOCPCONFIG_AARCH64:
	     rc = LittleOCPConfig(x1, x2, x3);
	     break;
     case MTK_SIP_KERNEL_LITTLEOCPSETTARGET_AARCH32:
     case MTK_SIP_KERNEL_LITTLEOCPSETTARGET_AARCH64:
	     rc = LittleOCPSetTarget(x1, x2);
		 break;
     case MTK_SIP_KERNEL_LITTLEOCPENABLE_AARCH32:
     case MTK_SIP_KERNEL_LITTLEOCPENABLE_AARCH64:
		 rc = LittleOCPEnable(x1, x2, x3);
	     break;
     case MTK_SIP_KERNEL_LITTLEOCPDISABLE_AARCH32:
     case MTK_SIP_KERNEL_LITTLEOCPDISABLE_AARCH64:
 	     rc = LittleOCPDisable(x1);
	     break;
     case MTK_SIP_KERNEL_LITTLEOCPDVFSSET_AARCH32:
     case MTK_SIP_KERNEL_LITTLEOCPDVFSSET_AARCH64:
	     rc = LittleOCPDVFSSet(x1, x2, x3);
	     break;
     case MTK_SIP_KERNEL_LITTLEOCPINTLIMIT_AARCH32:
     case MTK_SIP_KERNEL_LITTLEOCPINTLIMIT_AARCH64:
         rc = LittleOCPIntLimit(x1, x2, x3);
	     break;
     case MTK_SIP_KERNEL_LITTLEOCPINTENDIS_AARCH32:
     case MTK_SIP_KERNEL_LITTLEOCPINTENDIS_AARCH64:
          rc = LittleOCPIntEnDis(x1, x2, x3);
          break;
     case MTK_SIP_KERNEL_LITTLEOCPINTCLR_AARCH32:
     case MTK_SIP_KERNEL_LITTLEOCPINTCLR_AARCH64:
         rc = LittleOCPIntClr(x1, x2, x3);
	     break;
	case MTK_SIP_KERNEL_LITTLEOCPAVGPWR_AARCH32:
	case MTK_SIP_KERNEL_LITTLEOCPAVGPWR_AARCH64:
		rc = LittleOCPAvgPwr(x1,x2,x3);
		break;
	case MTK_SIP_KERNEL_LITTLEOCPCAPTURE00_AARCH32:
    case MTK_SIP_KERNEL_LITTLEOCPCAPTURE00_AARCH64:
	     rc = LittleOCPCapture(x1, 0, 0, x2, x3);
	     break;
	case MTK_SIP_KERNEL_LITTLEOCPCAPTURE10_AARCH32:
    case MTK_SIP_KERNEL_LITTLEOCPCAPTURE10_AARCH64:
	     rc = LittleOCPCapture(x1, 1, 0, x2, x3);
	     break;
	case MTK_SIP_KERNEL_LITTLEOCPCAPTURE11_AARCH32:
    case MTK_SIP_KERNEL_LITTLEOCPCAPTURE11_AARCH64:
	     rc = LittleOCPCapture(x1, 1, 1, x2, x3);
	     break;
	case MTK_SIP_KERNEL_LITTLEOCPAVGPWRGET_AARCH32:
	case MTK_SIP_KERNEL_LITTLEOCPAVGPWRGET_AARCH64:
		 rc = LittleOCPAvgPwrGet(x1);
		 break;

// DREQ + SRAMLDO
	case MTK_SIP_KERNEL_BIGSRAMLDOENABLE_AARCH32:
    case MTK_SIP_KERNEL_BIGSRAMLDOENABLE_AARCH64:
 	     rc = BigSRAMLDOEnable(x1);
 	     break;
	case MTK_SIP_KERNEL_BIGDREQHWEN_AARCH32:
    case MTK_SIP_KERNEL_BIGDREQHWEN_AARCH64:
 	     rc = BigDREQHWEn(x1, x2);
 	     break;
    case MTK_SIP_KERNEL_BIGDREQSWEN_AARCH32:
    case MTK_SIP_KERNEL_BIGDREQSWEN_AARCH64:
 	     rc = BigDREQSWEn(x1);
 	     break;
    case MTK_SIP_KERNEL_BIGDREQGET_AARCH32:
    case MTK_SIP_KERNEL_BIGDREQGET_AARCH64:
         rc = BigDREQGet();
         break;
    case MTK_SIP_KERNEL_LITTLEDREQSWEN_AARCH32:
    case MTK_SIP_KERNEL_LITTLEDREQSWEN_AARCH64:
 	     rc = LittleDREQSWEn(x1);
 	     break;
    case MTK_SIP_KERNEL_LITTLEDREQGET_AARCH32:
    case MTK_SIP_KERNEL_LITTLEDREQGET_AARCH64:
 		 rc = LittleDREQGet();
 	     break;

    case MTK_SIP_KERNEL_ICACHE_DUMP_AARCH32:
    case MTK_SIP_KERNEL_ICACHE_DUMP_AARCH64:
	    rc = mt_icache_dump(x1, x2);
	    break;
#endif

  default:
        rc = SMC_UNK;
        WARN("Unimplemented SIP Call: 0x%x \n", smc_fid);
    }

    SMC_RET1(handle, rc);
}

