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


/*******************************************************************************
 * This is the Secure Payload Dispatcher (SPD). The dispatcher is meant to be a
 * plug-in component to the Secure Monitor, registered as a runtime service. The
 * SPD is expected to be a functional extension of the Secure Payload (SP) that
 * executes in Secure EL1. The Secure Monitor will delegate all SMCs targeting
 * the Trusted OS/Applications range to the dispatcher. The SPD will either
 * handle the request locally or delegate it to the Secure Payload. It is also
 * responsible for initialising and maintaining communication with the SP.
 ******************************************************************************/
#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <bl31.h>
#include <context_mgmt.h>
#include <debug.h>
#include <errno.h>
#include <platform.h>
#include <runtime_svc.h>
#include <stddef.h>
#include <mtee.h>
#include <uuid.h>
#include "mtee_private.h"
#include <platform_def.h>
#include <fiq_smp_call.h>

/*******************************************************************************
 * Address of the entrypoint vector table in the Secure Payload. It is
 * initialised once on the primary core after a cold boot.
 ******************************************************************************/
mtee_vectors_t *mtee_vectors;

typedef struct mtee_param_s
{
	void    *sp_el1;
	void    *param;
	uint32_t paramType;
	uint32_t ns_spsr_el3;
} mtee_param_t;
/*******************************************************************************
 * Array to keep track of per-cpu Secure Payload state
 ******************************************************************************/
mtee_context_t mtee_sp_context[MTEE_CORE_COUNT];
mtee_param_t mtee_param[MTEE_CORE_COUNT];


int32_t mtee_init(void);

/*******************************************************************************
 * This function is the handler registered for S-EL1 interrupts by the MTEE. It
 * validates the interrupt and upon success arranges entry into the MTEE at
 * 'mtee_fiq_entry()' for handling the interrupt.
 ******************************************************************************/
static uint64_t mtee_sel1_interrupt_handler(uint32_t id,
					    uint32_t flags,
					    void *handle,
					    void *cookie)
{
	unsigned int iar;
#if 0
	uint32_t linear_id;
	uint64_t mpidr;
	mtee_context_t *mtee_ctx;
#endif

	/* Check the security state when the exception was generated */
	assert(get_interrupt_src_ss(flags) == NON_SECURE);

#if IMF_READ_INTERRUPT_ID
	/* Check the security status of the interrupt */
	assert(plat_ic_get_interrupt_type(id) == INTR_TYPE_S_EL1);
#endif

	/* ack the interrupt and read the IAR */
	iar = plat_ic_acknowledge_interrupt();
	iar &= 0x3FF;	/* interrupt ID */

	if (iar >= 1022) {
		SMC_RET0(handle);
	}

	if(iar == WDT_IRQ_BIT_ID)
	{
               fiq_smp_call_function(0xFE, aee_wdt_dump, 0, 0);
               aee_wdt_dump();
	}
	else if(iar == FIQ_SMP_CALL_SGI)
	{
		fiq_icc_isr();
	}

	if(iar == WDT_IRQ_BIT_ID || iar == FIQ_SMP_CALL_SGI)
	{
		plat_ic_end_of_interrupt(iar);

		SMC_RET0(handle);
	}

#if 0
	/* Sanity check the pointer to this cpu's context */
	mpidr = read_mpidr();
	assert(handle == cm_get_context(NON_SECURE));

	/* Save the non-secure context before entering the MTEE */
	cm_el1_sysregs_context_save(NON_SECURE);

	/* Get a reference to this cpu's MTEE context */
	linear_id = platform_get_core_pos(mpidr);
	mtee_ctx = &mtee_sp_context[linear_id];
	assert(&mtee_ctx->cpu_ctx == cm_get_context(SECURE));

	/*
	 * Determine if the MTEE was previously preempted. Its last known
	 * context has to be preserved in this case.
	 * The MTEE should return control to the MTEE after handling this
	 * FIQ. Preserve essential EL3 context to allow entry into the
	 * MTEE at the FIQ entry point using the 'cpu_context' structure.
	 * There is no need to save the secure system register context
	 * since the mTEE is supposed to preserve it during S-EL1 interrupt
	 * handling.
	 */
	if (get_std_smc_active_flag(mtee_ctx->state)) {
		mtee_ctx->saved_spsr_el3 = SMC_GET_EL3(&mtee_ctx->cpu_ctx,
						      CTX_SPSR_EL3);
		mtee_ctx->saved_elr_el3 = SMC_GET_EL3(&mtee_ctx->cpu_ctx,
						     CTX_ELR_EL3);
	}

	SMC_SET_EL3(&mtee_ctx->cpu_ctx,
		    CTX_SPSR_EL3,
		    SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS));
	SMC_SET_EL3(&mtee_ctx->cpu_ctx,
		    CTX_ELR_EL3,
		    (uint64_t) &mtee_vectors->fiq_entry);
	cm_el1_sysregs_context_restore(SECURE);
	cm_set_next_eret_context(SECURE);

	/*
	 * Tell the MTEE that it has to handle an FIQ synchronously. Also the
	 * instruction in normal world where the interrupt was generated is
	 * passed for debugging purposes. It is safe to retrieve this address
	 * from ELR_EL3 as the secure context will not take effect until
	 * el3_exit().
	 */
	SMC_RET2(&mtee_ctx->cpu_ctx, MTEE_HANDLE_FIQ_AND_RETURN, read_elr_el3());
#else
	plat_ic_end_of_interrupt(iar);

	SMC_RET0(handle);
#endif
}

/*******************************************************************************
 * Secure Payload Dispatcher setup. The SPD finds out the SP entrypoint and type
 * (aarch32/aarch64) if not already known and initialises the context for entry
 * into the SP for its initialisation.
 ******************************************************************************/
int32_t mtee_setup(void)
{
	entry_point_info_t *image_info;
	int32_t rc;
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id;

	linear_id = platform_get_core_pos(mpidr);

	/*
	 * Get information about the Secure Payload (BL32) image. Its
	 * absence is a critical failure.  TODO: Add support to
	 * conditionally include the SPD service
	 */
	image_info = bl31_plat_get_next_image_ep_info(SECURE);
	assert(image_info);

	/*
	 * If there's no valid entry point for SP, we return a non-zero value
	 * signalling failure initializing the service. We bail out without
	 * registering any handlers
	 */
	if (!image_info->pc)
		return 1;

	/*
	 * We could inspect the SP image and determine it's execution
	 * state i.e whether AArch32 or AArch64. Assuming it's AArch64
	 * for the time being.
	 */
	rc = mtee_init_secure_context(image_info->pc,
				     MTEE_AARCH64,
				     mpidr,
				     &mtee_sp_context[linear_id]);
	assert(rc == 0);

	/*
	 * All MTEE initialization done. Now register our init function with
	 * BL31 for deferred invocation
	 */
	bl31_register_bl32_init(&mtee_init);

	return rc;
}

/*******************************************************************************
 * This function passes control to the Secure Payload image (BL32) for the first
 * time on the primary cpu after a cold boot. It assumes that a valid secure
 * context has already been created by mtee_setup() which can be directly used.
 * It also assumes that a valid non-secure context has been initialised by PSCI
 * so it does not need to save and restore any non-secure state. This function
 * performs a synchronous entry into the Secure payload. The SP passes control
 * back to this routine through a SMC.
 ******************************************************************************/
int32_t mtee_init(void)
{
	uint64_t mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr), flags;
	uint64_t rc;
	mtee_context_t *mtee_ctx = &mtee_sp_context[linear_id];

	cm_el1_sysregs_context_save(NON_SECURE);
	/*
	 * Arrange for an entry into the test secure payload. We expect an array
	 * of vectors in return
	 */
	rc = mtee_synchronous_sp_entry(mtee_ctx);
	assert(rc != 0);
	if (rc) {
		set_mtee_pstate(mtee_ctx->state, MTEE_PSTATE_ON);

		/*
		 * MTEE has been successfully initialized. Register power
		 * managemnt hooks with PSCI
		 */
		psci_register_spd_pm_hook(&mtee_pm);
	}
	/*
	 * Register an interrupt handler for S-EL1 interrupts when generated
	 * during code executing in the non-secure state.
	 */
	flags = 0;
	set_interrupt_rm_flag(flags, NON_SECURE);
	rc = register_interrupt_type_handler(INTR_TYPE_S_EL1,
					     mtee_sel1_interrupt_handler,
					     flags);
	if (rc)
		panic();

	cm_el1_sysregs_context_restore(NON_SECURE);
	return rc;
}


/*******************************************************************************
 * This function is responsible for handling all SMCs in the Trusted OS/App
 * range from the non-secure state as defined in the SMC Calling Convention
 * Document. It is also responsible for communicating with the Secure payload
 * to delegate work and return results back to the non-secure state. Lastly it
 * will also return any information that the secure payload needs to do the
 * work assigned to it.
 ******************************************************************************/
uint64_t mtee_smc_handler(uint32_t smc_fid,
			 uint64_t x1,
			 uint64_t x2,
			 uint64_t x3,
			 uint64_t x4,
			 void *cookie,
			 void *handle,
			 uint64_t flags)
{
	cpu_context_t *ns_cpu_context;
	unsigned long mpidr = read_mpidr();
	uint32_t linear_id = platform_get_core_pos(mpidr), ns;
	mtee_context_t *mtee_ctx = &mtee_sp_context[linear_id];

	/* Determine which security state this SMC originated from */
	ns = is_caller_non_secure(flags);

	switch (smc_fid) {

	/*
	 * This function ID is used only by the MTEE to indicate that it has
	 * finished handling a S-EL1 FIQ interrupt. Execution should resume
	 * in the normal world.
	 */
	case MTEE_HANDLED_S_EL1_FIQ:
		if (ns)
			SMC_RET1(handle, SMC_UNK);

		assert(handle == cm_get_context(SECURE));

		/*
		 * Restore the relevant EL3 state which saved to service
		 * this SMC.
		 */
		if (get_std_smc_active_flag(mtee_ctx->state)) {
			SMC_SET_EL3(&mtee_ctx->cpu_ctx,
				    CTX_SPSR_EL3,
				    mtee_ctx->saved_spsr_el3);
			SMC_SET_EL3(&mtee_ctx->cpu_ctx,
				    CTX_ELR_EL3,
				    mtee_ctx->saved_elr_el3);
		}

		/* Get a reference to the non-secure context */
		ns_cpu_context = cm_get_context(NON_SECURE);
		assert(ns_cpu_context);

		/*
		 * Restore non-secure state. There is no need to save the
		 * secure system register context since the MTEE was supposed
		 * to preserve it during S-EL1 interrupt handling.
		 */
		cm_el1_sysregs_context_restore(NON_SECURE);
		cm_set_next_eret_context(NON_SECURE);

		SMC_RET0((uint64_t) ns_cpu_context);


	/*
	 * This function ID is used only by the MTEE to indicate that it was
	 * interrupted due to a EL3 FIQ interrupt. Execution should resume
	 * in the normal world.
	 */
	case MTEE_EL3_FIQ:
		if (ns)
			SMC_RET1(handle, SMC_UNK);

		assert(handle == cm_get_context(SECURE));

		/* Assert that standard SMC execution has been preempted */
		assert(get_std_smc_active_flag(mtee_ctx->state));

		/* Save the secure system register state */
		cm_el1_sysregs_context_save(SECURE);

		/* Get a reference to the non-secure context */
		ns_cpu_context = cm_get_context(NON_SECURE);
		assert(ns_cpu_context);

		/* Restore non-secure state */
		cm_el1_sysregs_context_restore(NON_SECURE);
		cm_set_next_eret_context(NON_SECURE);

		SMC_RET1(ns_cpu_context, MTEE_EL3_FIQ);


	/*
	 * This function ID is used only by the SP to indicate it has
	 * finished initialising itself after a cold boot
	 */
	case MTEE_ENTRY_DONE:
		if (ns)
			SMC_RET1(handle, SMC_UNK);

		/*
		 * Stash the SP entry points information. This is done
		 * only once on the primary cpu
		 */
		assert(mtee_vectors == NULL);
		mtee_vectors = (mtee_vectors_t *) x1;
		cm_el1_sysregs_context_save(SECURE);

		/*
		 * SP reports completion. The SPD must have initiated
		 * the original request through a synchronous entry
		 * into the SP. Jump back to the original C runtime
		 * context.
		 */
		mtee_synchronous_sp_exit(mtee_ctx, x1);

	/*
	 * These function IDs is used only by the SP to indicate it has
	 * finished:
	 * 1. turning itself on in response to an earlier psci
	 *    cpu_on request
	 * 2. resuming itself after an earlier psci cpu_suspend
	 *    request.
	 */
	case MTEE_ON_DONE:
	case MTEE_RESUME_DONE:

	/*
	 * These function IDs is used only by the SP to indicate it has
	 * finished:
	 * 1. suspending itself after an earlier psci cpu_suspend
	 *    request.
	 * 2. turning itself off in response to an earlier psci
	 *    cpu_off request.
	 */
	case MTEE_OFF_DONE:
	case MTEE_SUSPEND_DONE:
		if (ns)
			SMC_RET1(handle, SMC_UNK);

		/*
		 * SP reports completion. The SPD must have initiated the
		 * original request through a synchronous entry into the SP.
		 * Jump back to the original C runtime context, and pass x1 as
		 * return value to the caller
		 */
		mtee_synchronous_sp_exit(mtee_ctx, x1);

		/*
		 * Request from non-secure client to perform an
		 * arithmetic operation or response from secure
		 * payload to an earlier request.
		 */
	case MTEE_STD_FID(MTEE_SERVICE):
	case MTEE_FAST_FID(MTEE_SERVICE):
	case MTEE32_STD_FID(MTEE_SERVICE):
	case MTEE32_FAST_FID(MTEE_SERVICE):
	case MTEE_STD_FID(MTEE_EARLY_SERVICE):
	case MTEE_FAST_FID(MTEE_EARLY_SERVICE):
	case MTEE32_STD_FID(MTEE_EARLY_SERVICE):
	case MTEE32_FAST_FID(MTEE_EARLY_SERVICE):

		if (ns) {
			cpu_context_t *ns_ctx = (cpu_context_t*)handle;
			/*
			 * This is a fresh request from the non-secure client.
			 * The parameters are in x1 and x2. Figure out which
			 * registers need to be preserved, save the non-secure
			 * state and send the request to the secure payload.
			 */
			assert(handle == cm_get_context(NON_SECURE));
			cm_el1_sysregs_context_save(NON_SECURE);

			/*
			 * We are done stashing the non-secure context. Ask the
			 * secure payload to do the work now.
			 */

			/*
			 * Verify if there is a valid context to use, copy the
			 * operation type and parameters to the secure context
			 * and jump to the fast smc entry point in the secure
			 * payload. Entry into S-EL1 will take place upon exit
			 * from this function.
			 */
			assert(&mtee_ctx->cpu_ctx == cm_get_context(SECURE));

			/* Set appropriate entry for SMC.
			 * We expect the MTEE to manage the PSTATE.I and PSTATE.F
			 * flags as appropriate.
			 */
			if (smc_fid == MTEE32_STD_FID(MTEE_EARLY_SERVICE) ||
			    smc_fid == MTEE32_FAST_FID(MTEE_EARLY_SERVICE) )
			{
				cm_set_elr_el3(SECURE, (uint64_t)
						&mtee_vectors->early_smc_entry);
			}
			else if (GET_SMC_TYPE(smc_fid) == SMC_TYPE_FAST) {
				cm_set_elr_el3(SECURE, (uint64_t)
						&mtee_vectors->fast_smc_entry);
			} else {
				cm_set_elr_el3(SECURE, (uint64_t)
						&mtee_vectors->std_smc_entry);
			}

			cm_el1_sysregs_context_restore(SECURE);
			cm_set_next_eret_context(SECURE);

			mtee_param[linear_id].paramType = (uint32_t)x3;
			mtee_param[linear_id].param = (void *)
				read_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X4);
			mtee_param[linear_id].sp_el1 = (void *)
				read_ctx_reg(get_sysregs_ctx(ns_ctx), CTX_SP_EL1);
			mtee_param[linear_id].ns_spsr_el3 = (uint32_t)
				read_ctx_reg(get_el3state_ctx(ns_ctx), CTX_SPSR_EL3);

			SMC_RET4(&mtee_ctx->cpu_ctx, x1, x2,
				(uint64_t)&mtee_param[linear_id],
				read_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X5));
		} else {
			/*
			 * This is the result from the secure client of an
			 * earlier request. The results are in x1-x3. Copy it
			 * into the non-secure context, save the secure state
			 * and return to the non-secure state.
			 */
			assert(handle == cm_get_context(SECURE));
			cm_el1_sysregs_context_save(SECURE);

			/* Get a reference to the non-secure context */
			ns_cpu_context = cm_get_context(NON_SECURE);
			assert(ns_cpu_context);

			/* Restore non-secure state */
			cm_el1_sysregs_context_restore(NON_SECURE);
			cm_set_next_eret_context(NON_SECURE);

			SMC_RET4(ns_cpu_context, smc_fid, x1, x2, x3);
		}

		break;

	case TOS_CALL_COUNT:
		/*
		 * Return the number of service function IDs implemented to
		 * provide service to non-secure
		 */
		SMC_RET1(handle, MTEE_NUM_FID);

	case TOS_CALL_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, MTEE_VERSION_MAJOR, MTEE_VERSION_MINOR);

	default:
		break;
	}

	SMC_RET1(handle, SMC_UNK);
}

/* Define a SPD runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	mtee_fast,

	OEN_TOS_START,
	OEN_TOS_END,
	SMC_TYPE_FAST,
	mtee_setup,
	mtee_smc_handler
);

/* Define a SPD runtime service descriptor for standard SMC calls */
DECLARE_RT_SVC(
	mtee_std,

	OEN_TOS_START,
	OEN_TOS_END,
	SMC_TYPE_STD,
	NULL,
	mtee_smc_handler
);
