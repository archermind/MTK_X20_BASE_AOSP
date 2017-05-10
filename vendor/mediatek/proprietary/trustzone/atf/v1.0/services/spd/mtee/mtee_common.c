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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <arch_helpers.h>
#include <platform.h>
#include <bl_common.h>
#include <runtime_svc.h>
#include <context_mgmt.h>
#include <bl31.h>
#include <mtee_private.h>

/*******************************************************************************
 * Given a secure payload entrypoint, register width, cpu id & pointer to a
 * context data structure, this function will create a secure context ready for
 * programming an entry into the secure payload.
 ******************************************************************************/
int32_t mtee_init_secure_context(uint64_t entrypoint,
				 uint32_t rw,
				 uint64_t mpidr,
				 mtee_context_t *mtee_ctx)
{
	uint32_t scr, sctlr;
	el1_sys_regs_t *el1_state;
        el3_state_t *el3_state;
	uint32_t spsr;

	/* Passing a NULL context is a critical programming error */
	assert(mtee_ctx);

	/*
	 * We support AArch64 MTEE for now.
	 * TODO: Add support for AArch32 MTEE
	 */
	assert(rw == MTEE_AARCH64);

	/*
	 * This might look redundant if the context was statically
	 * allocated but this function cannot make that assumption.
	 */
	memset(mtee_ctx, 0, sizeof(*mtee_ctx));

	/*
	 * Set the right security state, register width and enable access to
	 * the secure physical timer for the SP.
	 */
	scr = read_scr();
	scr &= ~SCR_NS_BIT;
	scr &= ~SCR_RW_BIT;
	scr |= SCR_ST_BIT;
	if (rw == MTEE_AARCH64)
		scr |= SCR_RW_BIT;

	/* Get a pointer to the S-EL1 context memory */
	el1_state = get_sysregs_ctx(&mtee_ctx->cpu_ctx);

	/*
	 * Program the SCTLR_EL1 such that upon entry in S-EL1, caches and MMU are
	 * disabled and exception endianess is set to be the same as EL3
	 */
	sctlr = read_sctlr_el3();
	sctlr &= SCTLR_EE_BIT;
	sctlr |= SCTLR_EL1_RES1;
	write_ctx_reg(el1_state, CTX_SCTLR_EL1, sctlr);

	/* Set this context as ready to be initialised i.e OFF */
	set_mtee_pstate(mtee_ctx->state, MTEE_PSTATE_OFF);

	/*
	 * This context has not been used yet. It will become valid
	 * when the MTEE is interrupted and wants the MTEE to preserve
	 * the context.
	 */
	clr_std_smc_active_flag(mtee_ctx->state);

	/* Associate this context with the cpu specified */
	mtee_ctx->mpidr = mpidr;

        el3_state = get_el3state_ctx(&mtee_ctx->cpu_ctx);
	spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
        write_ctx_reg(el3_state, CTX_SPSR_EL3, spsr);
        write_ctx_reg(el3_state, CTX_ELR_EL3, entrypoint);
        write_ctx_reg(el3_state, CTX_SCR_EL3, scr);

	cm_set_context(&mtee_ctx->cpu_ctx, SECURE);

	return 0;
}

/*******************************************************************************
 * This function takes an SP context pointer and:
 * 1. Applies the S-EL1 system register context from mtee_ctx->cpu_ctx.
 * 2. Saves the current C runtime state (callee saved registers) on the stack
 *    frame and saves a reference to this state.
 * 3. Calls el3_exit() so that the EL3 system and general purpose registers
 *    from the mtee_ctx->cpu_ctx are used to enter the secure payload image.
 ******************************************************************************/
uint64_t mtee_synchronous_sp_entry(mtee_context_t *mtee_ctx)
{
	uint64_t rc;

	assert(mtee_ctx->c_rt_ctx == 0);

	/* Apply the Secure EL1 system register context and switch to it */
	assert(cm_get_context(SECURE) == &mtee_ctx->cpu_ctx);
	cm_el1_sysregs_context_restore(SECURE);
	cm_set_next_eret_context(SECURE);

	rc = mtee_enter_sp(&mtee_ctx->c_rt_ctx);
#if DEBUG
	mtee_ctx->c_rt_ctx = 0;
#endif

	return rc;
}


/*******************************************************************************
 * This function takes an SP context pointer and:
 * 1. Saves the S-EL1 system register context tp mtee_ctx->cpu_ctx.
 * 2. Restores the current C runtime state (callee saved registers) from the
 *    stack frame using the reference to this state saved in tspd_enter_sp().
 * 3. It does not need to save any general purpose or EL3 system register state
 *    as the generic smc entry routine should have saved those.
 ******************************************************************************/
void mtee_synchronous_sp_exit(mtee_context_t *mtee_ctx, uint64_t ret)
{
	/* Save the Secure EL1 system register context */
	assert(cm_get_context(SECURE) == &mtee_ctx->cpu_ctx);
	cm_el1_sysregs_context_save(SECURE);

	assert(mtee_ctx->c_rt_ctx != 0);
	mtee_exit_sp(mtee_ctx->c_rt_ctx, ret);

	/* Should never reach here */
	assert(0);
}

