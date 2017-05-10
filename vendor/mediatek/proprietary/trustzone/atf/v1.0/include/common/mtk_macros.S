#ifdef SPD_tbase
#include <tbase_private.h>
#endif
#ifdef SPD_trusty
#include <smcall.h>
#endif
	.macro	smc_id_log_pre
	#ifdef DEBUG_SMC_ID_LOG
		stp	x0, x1, [sp, #-16]!
	#ifdef SPD_tbase
		ldr	x19,=TBASE_SMC_FASTCALL_OUTPUT
		cmp x0, x19
		b.eq	1f
	#endif
	#ifdef SPD_trusty
		ldr	x19,=SMC_SC_NOP
		cmp x0, x19
		b.eq	1f
		ldr	x19,=SMC_SC_NS_RETURN
		cmp x0, x19
		b.eq	1f
	#endif
		stp	x2, x3, [sp, #-16]!
		stp	x4, x5, [sp, #-16]!
		stp	x6, x7, [sp, #-16]!
		stp	x15, x16, [sp, #-16]!
		mov	x2, x0
		mrs	x0, mpidr_el1
		bl	platform_get_core_pos
		mov x1, x0
		adr 	x0, dump_smc_id_in
		bl	tf_printf
		ldp	x15, x16, [sp],#16
		ldp	x6, x7, [sp],#16
		ldp	x4, x5, [sp],#16
		ldp	x2, x3, [sp],#16
		ldp	x0, x1, [sp],#16
		/* save smc id for smc handle exit print out smc id*/
		stp x0, x1, [sp, #-16]!
	1:
	#endif
	.endm

	.macro	smc_id_log_post
	#ifdef DEBUG_SMC_ID_LOG
		ldp	x0, x1, [sp],#16
	#ifdef SPD_tbase
		ldr	x19,=TBASE_SMC_FASTCALL_OUTPUT
		cmp x0, x19
		b.eq	2f
	#endif
	#ifdef SPD_trusty
		ldr	x19,=SMC_SC_NOP
		cmp x0, x19
		b.eq	2f
		ldr	x19,=SMC_SC_NS_RETURN
		cmp x0, x19
		b.eq	2f
	#endif
		mov	x2, x0
		mrs	x0, mpidr_el1
		bl	platform_get_core_pos
		mov	x1, x0
		adr x0, dump_smc_id_out
		bl  tf_printf
	2:
	#endif
	.endm
