/*
 * Copyright (c) 2013-2014, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <context_mgmt.h>
#include <string.h>
#include "fiqd_private.h"

/*******************************************************************************
 * Given a secure payload entrypoint info pointer, entry point PC, register
 * width, cpu id & pointer to a context data structure, this function will
 * initialize tsp context and entry point info for the secure payload
 ******************************************************************************/
void fiqd_init_tsp_ep_state(struct entry_point_info *tsp_entry_point,
				uint32_t rw,
				uint64_t pc,
				tsp_context_t *tsp_ctx)
{
	uint32_t ep_attr;

	/* Passing a NULL context is a critical programming error */
	assert(tsp_ctx);
	assert(tsp_entry_point);
#if 0    
	assert(pc);
#endif

	/*
	 * We support AArch64 TSP for now.
	 * TODO: Add support for AArch32 TSP
	 */
	assert(rw == TSP_AARCH64);

	/* Associate this context with the cpu specified */
	tsp_ctx->mpidr = read_mpidr_el1();
	tsp_ctx->state = 0;
	set_tsp_pstate(tsp_ctx->state, TSP_PSTATE_OFF);
	clr_std_smc_active_flag(tsp_ctx->state);

	cm_set_context(&tsp_ctx->cpu_ctx, SECURE);

	/* initialise an entrypoint to set up the CPU context */
	ep_attr = SECURE | EP_ST_ENABLE;
	if (read_sctlr_el3() & SCTLR_EE_BIT)
		ep_attr |= EP_EE_BIG;
	SET_PARAM_HEAD(tsp_entry_point, PARAM_EP, VERSION_1, ep_attr);

	tsp_entry_point->pc = pc;
	tsp_entry_point->spsr = SPSR_64(MODE_EL1,
					MODE_SP_ELX,
					DISABLE_ALL_EXCEPTIONS);
	memset(&tsp_entry_point->args, 0, sizeof(tsp_entry_point->args));
}

