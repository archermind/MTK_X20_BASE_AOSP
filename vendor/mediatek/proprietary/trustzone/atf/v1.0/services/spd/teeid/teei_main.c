/*
 * Copyright (c) 2015
 * microtrust
 * All rights reserved
 * author: luocl@microtrust.com
 * author: steven meng
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <arch_helpers.h>
#include <console.h>
#include <platform.h>
#include <context_mgmt.h>
#include <runtime_svc.h>
#include <bl31.h>
#include <teei_private.h>
#include <plat_teei.h>
#include "utos_version.h"


// SPSR for teei setup
#define TEEI_INIT_SPSR 0x1d3
#define NWD_INIT_SPSR 0x1d3 

// SPSR for entry to teei
#define TEEI_ENTRY_SPSR 0x1d1

// Initilization parameters for teei.
static tee_arg_t_ptr teeiBootCfg;

uint64_t teeiBootCoreMpidr;


// ************************************************************************************
// Mask 32 bit software registers

uint32_t maskSWdRegister(uint64_t x)
{
    return (uint32_t)x;
}

//*******************************************************************************
// Create a secure context ready for programming an entry into the secure 
// payload.
// This does not modify actual EL1 or EL3 system registers, so no need to 
// save / restore them.
 
int32_t teei_init_secure_context(teei_context *teei_ctx)
{
  uint32_t sctlr = read_sctlr_el3();
  el1_sys_regs_t *el1_state;
  uint64_t mpidr = read_mpidr();
  
  /* Passing a NULL context is a critical programming error */
  assert(teei_ctx);
  
  //DBG_PRINTF("teei_init_secure_context\n\r");

  memset(teei_ctx, 0, sizeof(*teei_ctx));

  /* Get a pointer to the S-EL1 context memory */
  el1_state = get_sysregs_ctx(&teei_ctx->cpu_ctx);

  // Program the sctlr for S-EL1 execution with caches and mmu off
  sctlr &= SCTLR_EE_BIT;
  sctlr |= SCTLR_EL1_RES1;
  
  //DBG_PRINTF(" sctlr = 0x%x \n\r",sctlr);
  
  write_ctx_reg(el1_state, CTX_SCTLR_EL1, sctlr);
 write_ctx_reg(el1_state, CTX_SPSR_EL1, TEEI_INIT_SPSR);
  write_ctx_reg(el1_state, CTX_ELR_EL1, teeiBootCfg->secDRamBase);
  /* Set this context as ready to be initialised i.e OFF */
  //teei_ctx->state = TEEI_STATE_OFF;

  /* Associate this context with the cpu specified */
  teei_ctx->mpidr = mpidr;

  // Set up cm context for this core
  cm_set_context_by_mpidr(mpidr, &teei_ctx->cpu_ctx, SECURE); 
//  cm_init_exception_stack(mpidr, SECURE);

  return 0;
}
/*******************************************************************************
 * This function populates 'cpu_context' pertaining to the given security state
 * with the entrypoint, SPSR and SCR values so that an ERET from this security
 * state correctly restores corresponding values to drop the CPU to the next
 * exception level
 ******************************************************************************/
static void cm_set_el3_eret_context(uint32_t security_state, uint64_t entrypoint,
		uint32_t spsr, uint32_t scr)
{
	cpu_context_t *ctx;
	el3_state_t *state;

	ctx = cm_get_context_by_mpidr(read_mpidr(), security_state);
	assert(ctx);

	/* Program the interrupt routing model for this security state */
	scr &= ~SCR_FIQ_BIT;
	scr &= ~SCR_IRQ_BIT;
	scr |= get_scr_el3_from_routing_model(security_state);

	/* Populate EL3 state so that we've the right context before doing ERET */
	state = get_el3state_ctx(ctx);
	write_ctx_reg(state, CTX_SPSR_EL3, spsr);
	write_ctx_reg(state, CTX_ELR_EL3, entrypoint);
	write_ctx_reg(state, CTX_SCR_EL3, scr);
}


//*******************************************************************************
// Configure teei and EL3 registers for intial entry 
static void teei_init_eret( uint64_t entrypoint, uint32_t rw ) {
  uint32_t scr = read_scr();
  uint32_t spsr = TEEI_INIT_SPSR;
  
  assert(rw == TEEI_AARCH32);

  // Set the right security state and register width for the SP
  scr &= ~SCR_NS_BIT; 
  scr &= ~SCR_RW_BIT; 
  // Also IRQ and FIQ handled in secure state
  scr &= ~(SCR_FIQ_BIT|SCR_IRQ_BIT); 
  // No execution from Non-secure memory
  scr |= SCR_SIF_BIT; 
  
  cm_set_el3_eret_context(SECURE, entrypoint, spsr, scr);
  
  entry_point_info_t *image_info = bl31_plat_get_next_image_ep_info(SECURE);
  assert(image_info);
  image_info->spsr = spsr;

}


// ************************************************************************************
// Initialize teei system for first entry to teei
// This and initial entry should be done only once in cold boot.

static int32_t teei_init_entry()
{
	
  DBG_PRINTF("[ATF--uTos] version [%s]\n\r",UTOS_VERSION);

  // Save el1 registers in case non-secure world has already been set up.
  // this time non-secure does not run , there is no context for non-secure this time, why do here??   --steven
  cm_el1_sysregs_context_save(NON_SECURE);
  cm_fpregs_context_save(NON_SECURE);
  
  uint64_t mpidr = read_mpidr();
  uint32_t linear_id = platform_get_core_pos(mpidr);
  teei_context *teei_ctx = &secure_context[linear_id];
  
  // ************************************************************************************
  // Set registers for teei initialization entry
  cpu_context_t *s_entry_context = &teei_ctx->cpu_ctx;
  gp_regs_t *s_entry_gpregs = get_gpregs_ctx(s_entry_context);
  write_ctx_reg(s_entry_gpregs, CTX_GPREG_X1, 0);
  write_ctx_reg(s_entry_gpregs, CTX_GPREG_X1, (int64_t)TEEI_BOOT_PARAMS);
  
  // Start teei
   DBG_PRINTF("  mtee ST [%ld]\n\r",linear_id);
   teei_synchronous_sp_entry(teei_ctx);
   //teei_ctx->state = TEEI_STATE_ON;
  
//DBG_PRINTF("[microtrust]: return from secure payload \n\r");
#ifdef TEEI_PM_ENABLE
  // Register power managemnt hooks with PSCI
  psci_register_spd_pm_hook(&teei_pm);
#endif
	    // this time non-secure does not run , there is no context for non-secure this time, why do here??   --steven
		cm_el1_sysregs_context_restore(NON_SECURE);
	    cm_fpregs_context_restore(NON_SECURE);
		cm_set_next_eret_context(NON_SECURE);

  return 1;
}

// ************************************************************************************
// Setup teei SPD 

int32_t teei_fastcall_setup(void)
{
	
  entry_point_info_t *image_info;
  int i;
 
  //DBG_PRINTF("[microtrust]:  teei_fastcall_setup \n");

  image_info = bl31_plat_get_next_image_ep_info(SECURE);
  assert(image_info);

  uint32_t linear_id = platform_get_core_pos(read_mpidr());
  
  teeiBootCoreMpidr = read_mpidr();
  
  teeiBootCfg = (tee_arg_t_ptr)(uintptr_t)TEEI_BOOT_PARAMS;

  // TODO: We do not need this anymore, if PM is in use
  teei_init_secure_context(&secure_context[linear_id]);
 
  teei_init_eret(image_info->pc,TEEI_AARCH32);
  bl31_register_bl32_init(&teei_init_entry);

  return 0;
}



