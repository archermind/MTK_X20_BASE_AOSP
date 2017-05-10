/*
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h> /* for context_mgmt.h */
#include <bl_common.h>
#include <bl31.h>
#include <context_mgmt.h>
#include <debug.h>
#include <interrupt_mgmt.h>
#include <plat_def.h>
#include <plat_private.h>
#include <platform.h>
#include <runtime_svc.h>
#include <string.h>

#include "smcall.h"
#include "sm_err.h"
#include "trusty_private.h"

#include <fiq_smp_call.h>

/* Store infos from preloader for Trusty */
static tee_v8_arg_t mt_trusty_boot_cfg;
static void mt_trusty_setup(entry_point_info_t *ep_info);

struct trusty_stack {
	uint8_t space[PLATFORM_STACK_SIZE / 4] __aligned(16);
};

struct trusty_cpu_ctx {
	cpu_context_t           cpu_ctx;
	void                   *saved_sp;
	uint32_t                saved_security_state;
#if 0
	int                     fiq_handler_active;
	uint64_t                fiq_handler_pc;
	uint64_t                fiq_handler_cpsr;
	uint64_t                fiq_handler_sp;
	uint64_t                fiq_pc;
	uint64_t                fiq_cpsr;
	uint64_t                fiq_sp_el1;
	gp_regs_t               fiq_gpregs;
#endif
	struct trusty_stack     secure_stack[1];
};
struct args {
	uint64_t r0;
	uint64_t r1;
	uint64_t r2;
	uint64_t r3;
};
struct trusty_cpu_ctx trusty_cpu_ctx[PLATFORM_CORE_COUNT];

struct args trusty_init_context_stack(void **sp, void *new_stack);
struct args trusty_context_switch_helper(void **sp, uint64_t r0, uint64_t r1,
					 uint64_t r2, uint64_t r3);

static struct trusty_cpu_ctx *get_trusty_ctx(void)
{
	return &trusty_cpu_ctx[platform_get_core_pos(read_mpidr())];
}

static struct args trusty_context_switch(uint32_t security_state, uint64_t r0,
					 uint64_t r1, uint64_t r2, uint64_t r3)
{
	struct args ret;
	struct trusty_cpu_ctx *ctx = get_trusty_ctx();

	assert(ctx->saved_security_state != security_state);

	cm_el1_sysregs_context_save(security_state);

	ctx->saved_security_state = security_state;
	ret = trusty_context_switch_helper(&ctx->saved_sp, r0, r1, r2, r3);

	assert(ctx->saved_security_state == !security_state);

	cm_el1_sysregs_context_restore(security_state);
	cm_set_next_eret_context(security_state);

	return ret;
}

static uint64_t trusty_fiq_handler(uint32_t id,
				   uint32_t flags,
				   void *handle,
				   void *cookie)
{
	//struct trusty_cpu_ctx *ctx = get_trusty_ctx();

	assert(!is_caller_secure(flags));

	if (id >= 1022) {
		SMC_RET0(handle);
	}

	if ((id == WDT_IRQ_BIT_ID) || (id == FIQ_SMP_CALL_SGI)) {
		plat_ic_acknowledge_interrupt();
		if (id == WDT_IRQ_BIT_ID){

			uint64_t mpidr;
			uint32_t linear_id;

			/* send to all cpus except the current one */
			mpidr = read_mpidr();
			linear_id = platform_get_core_pos(mpidr);
			fiq_smp_call_function(0xFF & ~(1 << linear_id), aee_wdt_dump, 0, 0);

			aee_wdt_dump();
		}


		if (id == FIQ_SMP_CALL_SGI)
			fiq_icc_isr();

        	plat_ic_end_of_interrupt(id);
		SMC_RET0(handle);
	} else {
		/* route to S-EL1 */
		trusty_context_switch(NON_SECURE, SMC_SC_NOP, 0, 0, 0);
		SMC_RET0(handle);
	}

#if 0
	ret = trusty_context_switch(NON_SECURE, SMC_FC_FIQ_ENTER, 0, 0, 0);
	if (ret.r0) {
		SMC_RET0(handle);
	}

	if (ctx->fiq_handler_active) {
		NOTICE("%s: fiq handler already active\n", __func__);
		SMC_RET0(handle);
	}

	ctx->fiq_handler_active = 1;
	memcpy(&ctx->fiq_gpregs, get_gpregs_ctx(handle), sizeof(ctx->fiq_gpregs));
	ctx->fiq_pc = SMC_GET_EL3(handle, CTX_ELR_EL3);
	ctx->fiq_cpsr = SMC_GET_EL3(handle, CTX_SPSR_EL3);
	ctx->fiq_sp_el1 = read_ctx_reg(get_sysregs_ctx(handle), CTX_SP_EL1);

	write_ctx_reg(get_sysregs_ctx(handle), CTX_SP_EL1, ctx->fiq_handler_sp);
	cm_set_elr_spsr_el3(NON_SECURE, ctx->fiq_handler_pc, ctx->fiq_handler_cpsr);

	SMC_RET0(handle);
#endif

}

#if 0
static uint64_t trusty_set_fiq_handler(void *handle, uint64_t cpu, uint64_t handler, uint64_t stack)
{
	struct trusty_cpu_ctx *ctx;

	if (cpu >= PLATFORM_CORE_COUNT) {
		NOTICE("%s: cpu %d >= %d\n", __func__, cpu, PLATFORM_CORE_COUNT);
		return SM_ERR_INVALID_PARAMETERS;
	}

	ctx = &trusty_cpu_ctx[cpu];
	ctx->fiq_handler_pc = handler;
	ctx->fiq_handler_cpsr = SMC_GET_EL3(handle, CTX_SPSR_EL3)
	ctx->fiq_handler_sp = stack;

	SMC_RET1(handle, 0);
}

static uint64_t trusty_get_fiq_regs(void *handle)
{
	struct trusty_cpu_ctx *ctx = get_trusty_ctx();
	uint64_t sp_el0 = read_ctx_reg(&ctx->fiq_gpregs, CTX_GPREG_SP_EL0);

	SMC_RET4(handle, ctx->fiq_pc, ctx->fiq_cpsr, sp_el0, ctx->fiq_sp_el1);
}

static uint64_t trusty_fiq_exit(void *handle, uint64_t x1, uint64_t x2, uint64_t x3)
{
	struct args ret;
	struct trusty_cpu_ctx *ctx = get_trusty_ctx();

	if (!ctx->fiq_handler_active) {
		NOTICE("%s: fiq handler not active\n", __func__);
		SMC_RET1(handle, SM_ERR_INVALID_PARAMETERS);
	}

	ret = trusty_context_switch(NON_SECURE, SMC_FC_FIQ_EXIT, 0, 0, 0);
	if (ret.r0 != 1) {
		NOTICE("%s(0x%lx) SMC_FC_FIQ_EXIT returned unexpected value, %d\n",
		       __func__, handle, ret.r0);
	}

	/*
	 * Restore register state to state recorded on fiq entry.
	 *
	 * x0, sp_el1, pc and cpsr need to be restored because el1 cannot
	 * restore them.
	 *
	 * x1-x4 and x8-x17 need to be restored here because smc_handler64
	 * corrupts them (el1 code also restored them).
	 */
	memcpy(get_gpregs_ctx(handle), &ctx->fiq_gpregs, sizeof(ctx->fiq_gpregs));
	ctx->fiq_handler_active = 0;
	write_ctx_reg(get_sysregs_ctx(handle), CTX_SP_EL1, ctx->fiq_sp_el1);
	cm_set_elr_spsr_el3(NON_SECURE, ctx->fiq_pc, ctx->fiq_cpsr);

	SMC_RET0(handle);
}
#endif

static uint64_t trusty_smc_handler(uint32_t smc_fid,
			 uint64_t x1,
			 uint64_t x2,
			 uint64_t x3,
			 uint64_t x4,
			 void *cookie,
			 void *handle,
			 uint64_t flags)
{
	struct args ret;
	uint32_t cpu = platform_get_core_pos(read_mpidr());
	if (is_caller_secure(flags)) {
		if (smc_fid == SMC_SC_NS_RETURN) {
			ret = trusty_context_switch(SECURE, x1, 0, 0, 0);
			SMC_RET4(handle, ret.r0, ret.r1, ret.r2, ret.r3);
		}
		NOTICE("%s(0x%x, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx) cpu %d, unknown smc\n",
		       __func__, smc_fid, x1, x2, x3, x4, cookie, handle, flags, cpu);
		SMC_RET1(handle, SMC_UNK);
	} else {
		switch (smc_fid) {
#if 0
		case SMC_FC64_SET_FIQ_HANDLER:
			return trusty_set_fiq_handler(handle, x1, x2, x3);
		case SMC_FC64_GET_FIQ_REGS:
			return trusty_get_fiq_regs(handle);
		case SMC_FC_FIQ_EXIT:
			return trusty_fiq_exit(handle, x1, x2, x3);
#endif
		default:
			ret = trusty_context_switch(NON_SECURE, smc_fid, x1, x2, x3);
			SMC_RET1(handle, ret.r0);
		}
	}
}

static int32_t trusty_init(void)
{
	void el3_exit();
	entry_point_info_t *ep_info;
	struct trusty_cpu_ctx *ctx = get_trusty_ctx();
	uint32_t cpu = platform_get_core_pos(read_mpidr());

	ep_info = bl31_plat_get_next_image_ep_info(SECURE);

	cm_el1_sysregs_context_save(NON_SECURE);

	cm_set_context(&ctx->cpu_ctx, SECURE);
	cm_init_context(read_mpidr(), ep_info);

	/* Adjust secondary cpu entry point for 32 bit images to the end of exeption vectors */
	if (cpu != 0 && GET_RW(read_ctx_reg(get_el3state_ctx(&ctx->cpu_ctx), CTX_SPSR_EL3)) == MODE_RW_32) {
		INFO("trusty: cpu %d, adjust entry point to 0x%x\n", cpu, ep_info->pc + (1U << 5));
		cm_set_elr_el3(SECURE, ep_info->pc + (1U << 5));
	}

	cm_el1_sysregs_context_restore(SECURE);
	cm_set_next_eret_context(SECURE);

	ctx->saved_security_state = ~0; /* initial saved state is invalid */
	trusty_init_context_stack(&ctx->saved_sp, &ctx->secure_stack[1]);

	trusty_context_switch_helper(&ctx->saved_sp, 0, 0, 0, 0);

	cm_el1_sysregs_context_restore(NON_SECURE);
	cm_set_next_eret_context(NON_SECURE);

	return 0;
}

static void trusty_cpu_suspend(void)
{
	struct args ret;
	uint32_t linear_id = platform_get_core_pos(read_mpidr());

	ret = trusty_context_switch(NON_SECURE, SMC_FC_CPU_SUSPEND, 0, 0, 0);
	if (ret.r0 != 0) {
		NOTICE("%s: cpu %d, SMC_FC_CPU_SUSPEND returned unexpected value, %d\n",
		       __func__, linear_id, ret.r0);
	}
}

static void trusty_cpu_resume(void)
{
	struct args ret;
	uint32_t linear_id = platform_get_core_pos(read_mpidr());

	ret = trusty_context_switch(NON_SECURE, SMC_FC_CPU_RESUME, 0, 0, 0);
	if (ret.r0 != 0) {
		NOTICE("%s: cpu %d, SMC_FC_CPU_RESUME returned unexpected value, %d\n",
		       __func__, linear_id, ret.r0);
	}
}

static int32_t trusty_cpu_off_handler(uint64_t unused)
{
	trusty_cpu_suspend();

	return 0;
}

static void trusty_cpu_on_finish_handler(uint64_t unused)
{
	struct trusty_cpu_ctx *ctx = get_trusty_ctx();

	if (!ctx->saved_sp) {
		trusty_init();
	} else {
		trusty_cpu_resume();
	}
}

static void trusty_cpu_suspend_handler(uint64_t unused)
{
	trusty_cpu_suspend();
}

static void trusty_cpu_suspend_finish_handler(uint64_t unused)
{
	trusty_cpu_resume();
}

static const spd_pm_ops_t trusty_pm = {
	.svc_off = trusty_cpu_off_handler,
	.svc_suspend = trusty_cpu_suspend_handler,
	.svc_on_finish = trusty_cpu_on_finish_handler,
	.svc_suspend_finish = trusty_cpu_suspend_finish_handler,
};

static int32_t trusty_setup(void)
{
	entry_point_info_t *ep_info;
#if 0
	uint32_t instr;
#endif
	uint32_t flags;
	int ret;
#if 0
	int aarch32 = 0;
#endif

	ep_info = bl31_plat_get_next_image_ep_info(SECURE);
	if (!ep_info) {
		NOTICE("Trusty image missing.\n");
		return -1;
	}

/*
 * Preloader will pass trusty image type thru teeargs.aarch32
 */
#if 0
	instr = *(uint32_t *)ep_info->pc;

	if (instr >> 24 == 0xea) {
		INFO("trusty: Found 32 bit image\n");
		aarch32 = 1;
	} else if (instr >> 8 == 0xd53810) {
		INFO("trusty: Found 64 bit image\n");
	} else {
		NOTICE("trusty: Found unknown image, 0x%x\n", instr);
	}
#endif

	/* Clean up args before setup */
	memset(&ep_info->args, 0, sizeof(ep_info->args));

	/* Parse and setup ep_info from preloader */
	mt_trusty_setup(ep_info);

	SET_PARAM_HEAD(ep_info, PARAM_EP, VERSION_1, SECURE | EP_ST_ENABLE);
	if (!mt_trusty_boot_cfg.aarch32)
		ep_info->spsr = SPSR_64(MODE_EL1, MODE_SP_ELX,
						  DISABLE_ALL_EXCEPTIONS);
	else
		ep_info->spsr = SPSR_MODE32(MODE32_svc, SPSR_T_ARM,
						      SPSR_E_LITTLE,
						      DAIF_FIQ_BIT |
							DAIF_IRQ_BIT |
							DAIF_ABT_BIT);

	bl31_register_bl32_init(trusty_init);

	psci_register_spd_pm_hook(&trusty_pm);

	flags = 0;
	set_interrupt_rm_flag(flags, NON_SECURE);
	ret = register_interrupt_type_handler(INTR_TYPE_S_EL1,
					      trusty_fiq_handler,
					      flags);
	if (ret)
		ERROR("trusty: failed to register fiq handler, ret = %d\n", ret);

	return 0;
}

static void mt_trusty_setup(entry_point_info_t *ep_info)
{
	struct trusty_cpu_ctx *ctx = get_trusty_ctx();
#if (defined(MACH_TYPE_MT6735) || defined(MACH_TYPE_MT6735M) || \
     defined(MACH_TYPE_MT6753) || defined(MACH_TYPE_MT8173))
        atf_arg_t_ptr atfarg = (atf_arg_t_ptr)(uintptr_t)TEE_BOOT_INFO_ADDR;
#else
	atf_arg_t_ptr atfarg = &gteearg;
#endif
	tee_v8_arg_t *teearg = (tee_v8_arg_t *)(uintptr_t)atfarg->tee_boot_arg_addr;

	INFO("%s\n", __func__);

	mt_trusty_boot_cfg.magic                   = teearg->magic;
	mt_trusty_boot_cfg.version                 = teearg->version;
	mt_trusty_boot_cfg.dRamBase                = teearg->dRamBase;
	mt_trusty_boot_cfg.dRamSize                = teearg->dRamSize;
	mt_trusty_boot_cfg.secDRamBase             = teearg->secDRamBase;
	mt_trusty_boot_cfg.secDRamSize             = teearg->secDRamSize;
	mt_trusty_boot_cfg.sRamBase                = teearg->sRamBase;
	mt_trusty_boot_cfg.sRamSize                = teearg->sRamSize;
	mt_trusty_boot_cfg.secSRamBase             = teearg->secSRamBase;
	mt_trusty_boot_cfg.secSRamSize             = teearg->secSRamSize;
	mt_trusty_boot_cfg.log_port                = teearg->log_port;
	mt_trusty_boot_cfg.log_baudrate            = teearg->log_baudrate;
	mt_trusty_boot_cfg.gicd_base               = teearg->gicd_base;
	mt_trusty_boot_cfg.gicc_base               = teearg->gicc_base;
	mt_trusty_boot_cfg.aarch32  			   = teearg->aarch32;
	memcpy(mt_trusty_boot_cfg.hwuid, teearg->hwuid, 16);

#if 0
	INFO("mt_trusty_boot_cfg.dRamBase=0x%x\n", mt_trusty_boot_cfg.dRamBase);
	INFO("mt_trusty_boot_cfg.dRamSize=0x%x\n", mt_trusty_boot_cfg.dRamSize);
	INFO("mt_trusty_boot_cfg.secDRamBase=0x%x\n", mt_trusty_boot_cfg.secDRamBase);
	INFO("mt_trusty_boot_cfg.secDRamSize=0x%x\n", mt_trusty_boot_cfg.secDRamSize);
	INFO("mt_trusty_boot_cfg.log_port=0x%x\n", mt_trusty_boot_cfg.log_port);
	INFO("mt_trusty_boot_cfg.log_baudrate=0x%x\n", mt_trusty_boot_cfg.log_baudrate);
	INFO("mt_trusty_boot_cfg.gicd_base=0x%x\n", mt_trusty_boot_cfg.gicd_base);
	INFO("mt_trusty_boot_cfg.gicc_base=0x%x\n", mt_trusty_boot_cfg.gicc_base);
	INFO("mt_trusty_boot_cfg.aarch32=0x%x\n", mt_trusty_boot_cfg.aarch32);
#endif

#if 0
	{
		INFO("mt_trusty_boot_cfg.hwuid[0-3] : 0x%x 0x%x 0x%x 0x%x\n",
				mt_trusty_boot_cfg.hwuid[0], mt_trusty_boot_cfg.hwuid[1],
				mt_trusty_boot_cfg.hwuid[2], mt_trusty_boot_cfg.hwuid[3]);
	}
#endif


	// ************************************************************************
	// Set registers for Trusty initialization entry
	// r0/x0: size of memory allocated to TOS
	// r1/x1: physical address of a contiguous block of memory that contains platform
	//		  specific boot parameters
	// r2/x2: size of the above block of memory
	// r14/x30: return address to jump to (in Non-secure mode) after TOS initializes

	ep_info->args.arg0 = mt_trusty_boot_cfg.secDRamSize;
	ep_info->args.arg1 = (int64_t)&mt_trusty_boot_cfg;
	ep_info->args.arg2 = sizeof(tee_v8_arg_t);

	cm_set_context(&ctx->cpu_ctx, SECURE);
}

/* Define a SPD runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	trusty_fast,

	OEN_TOS_START,
	SMC_ENTITY_SECURE_MONITOR,
	SMC_TYPE_FAST,
	trusty_setup,
	trusty_smc_handler
);

/* Define a SPD runtime service descriptor for standard SMC calls */
DECLARE_RT_SVC(
	trusty_std,

	OEN_TOS_START,
	SMC_ENTITY_SECURE_MONITOR,
	SMC_TYPE_STD,
	NULL,
	trusty_smc_handler
);

