/*
 * Copyright (c) 2015
 * microtrust
 * All rights reserved
 * author: luocl@microtrust.com
 * author: steven meng
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
#include <teei_private.h>


/*******************************************************************************
 * This function takes an SP context pointer and:
 * 1. Applies the S-EL1 system register context from tsp_ctx->cpu_ctx.
 * 2. Saves the current C runtime state (callee saved registers) on the stack
 *    frame and saves a reference to this state.
 * 3. Calls el3_exit() so that the EL3 system and general purpose registers
 *    from the tsp_ctx->cpu_ctx are used to enter the secure payload image.
 ******************************************************************************/
uint64_t teei_synchronous_sp_entry(teei_context *teei_ctx)
{
	uint64_t rc;

	assert(teei_ctx->c_rt_ctx == 0);

  cpu_context_t *s_entry_context = &teei_ctx->cpu_ctx;
  gp_regs_t *s_entry_gpregs = get_gpregs_ctx(s_entry_context);

	/* Apply the Secure EL1 system register context and switch to it */
	assert(cm_get_context_by_mpidr(read_mpidr(), SECURE) == &teei_ctx->cpu_ctx);

	cm_el1_sysregs_context_restore(SECURE);
	cm_fpregs_context_restore(SECURE);

	cm_set_next_eret_context(SECURE);
	
	rc = teei_enter_sp(&teei_ctx->c_rt_ctx);
	
#if DEBUG
	teei_ctx->c_rt_ctx = 0;
#endif

	return rc;
}

/*******************************************************************************
 * This function takes an SP context pointer and:
 * 1. Saves the S-EL1 system register context tp tsp_ctx->cpu_ctx.
 * 2. Restores the current C runtime state (callee saved registers) from the
 *    stack frame using the reference to this state saved in tspd_enter_sp().
 * 3. It does not need to save any general purpose or EL3 system register state
 *    as the generic smc entry routine should have saved those.
 ******************************************************************************/
void teei_synchronous_sp_exit(teei_context *teei_ctx, uint64_t ret, uint32_t save_sysregs)
{
	/* Save the Secure EL1 system register context */
	assert(cm_get_context_by_mpidr(read_mpidr(), SECURE) == &teei_ctx->cpu_ctx);
	if (save_sysregs){
	  cm_el1_sysregs_context_save(SECURE);
	  cm_fpregs_context_save(SECURE);
    }

	assert(teei_ctx->c_rt_ctx != 0);
	teei_exit_sp(teei_ctx->c_rt_ctx, ret);

	/* Should never reach here */
	assert(0);
}

