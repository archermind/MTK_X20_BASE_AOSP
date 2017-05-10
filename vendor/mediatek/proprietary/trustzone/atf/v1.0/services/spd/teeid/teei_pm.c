/*
 * Copyright (c) 2015
 * microtrust
 * All rights reserved
 * author: luocl@microtrust.com
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
#include <debug.h>

#ifdef TEEI_PM_ENABLE

#include <psci.h>
extern uint64_t teeiBootCoreMpidr;

extern int32_t teei_init_secure_context(teei_context *teei_ctx);
 
static void save_sysregs_core(uint64_t fromCoreNro, uint32_t toCoreNro) {
  if (fromCoreNro != toCoreNro) {
    cpu_context_t *cpu_ctx = &secure_context[fromCoreNro].cpu_ctx;
    memcpy(&secure_context[toCoreNro].cpu_ctx, cpu_ctx, sizeof(cpu_context_t) );
  }
}
void teei_init_core(uint64_t mpidr) {
  uint32_t linear_id = platform_get_core_pos(mpidr);
  teei_context *teei_ctx = &secure_context[linear_id];

  if (mpidr == teeiBootCoreMpidr) {
    return;
  }

  teei_init_secure_context(teei_ctx);
  uint32_t boot_core_nro = platform_get_core_pos(teeiBootCoreMpidr);
  save_sysregs_core(boot_core_nro, linear_id);
}
/*******************************************************************************
 * This cpu has been turned on. 
 ******************************************************************************/
static void teei_cpu_on_finish_handler(uint64_t cookie)
{

    uint64_t mpidr = read_mpidr();
  uint32_t linear_id = platform_get_core_pos(mpidr);
  teei_context *teei_ctx = &secure_context[linear_id];

  //assert(teei_ctx->state == TEEI_STATE_OFF);

  // Core specific initialization;
  teei_init_core(mpidr);
  
  //DBG_PRINTF("\r\nteei_cpu_on_finish_handler %d\r\n", linear_id);
  // TODO
  //teei_ctx->state = TEEI_STATE_ON;
}

/*******************************************************************************
 * Structure populated by the TEEI Dispatcher to be given a chance to perform any
 * TEEI bookkeeping before PSCI executes a power mgmt. operation.
 ******************************************************************************/
const spd_pm_ops_t teei_pm = {
  NULL,
  NULL,
  NULL,
  teei_cpu_on_finish_handler,
  NULL,
  NULL,
  NULL
};

#endif

