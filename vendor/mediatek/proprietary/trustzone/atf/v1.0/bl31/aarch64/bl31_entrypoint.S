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

#include <arch.h>
#include <asm_macros.S>
#include <bl_common.h>
#if defined(MACH_TYPE_MT6797)
#include "../plat/mt6797/power.h"
#endif
.macro  branch_if_master, xreg1, xreg2, master_label
	mrs     \xreg1, mpidr_el1
	lsr     \xreg2, \xreg1, #32
	lsl     \xreg1, \xreg1, #40
	lsr     \xreg1, \xreg1, #40
	orr     \xreg1, \xreg1, \xreg2
	cbz     \xreg1, \master_label
.endm

.macro  branch_if_slave, xreg, slave_label
	mrs     \xreg, mpidr_el1
	tst     \xreg, #0xff            /* Test Affinity 0 */
	b.ne    \slave_label
	lsr     \xreg, \xreg, #8
	tst     \xreg, #0xff            /* Test Affinity 1 */
	b.ne    \slave_label
	lsr     \xreg, \xreg, #8
	tst     \xreg, #0xff            /* Test Affinity 2 */
	b.ne    \slave_label
	lsr     \xreg, \xreg, #16
	tst     \xreg, #0xff            /* Test Affinity 3 */
	b.ne    \slave_label
.endm

.macro  branch_if_cluster, xreg, cl0_label, cl1_label, cl2_label
	mrs	\xreg, mpidr_el1
	and	\xreg, \xreg, #0x0f00
	cmp	\xreg, #0
	b.eq	\cl0_label
	cmp	\xreg, #0x100
	b.eq	\cl1_label
	cmp	\xreg, #0x200
	b.eq	\cl2_label
.endm

	.globl	bl31_entrypoint
	.globl	bl31_on_entrypoint

	/* -----------------------------------------------------
	 * bl31_entrypoint() is the cold boot entrypoint,
	 * executed only by the primary cpu.
	 * -----------------------------------------------------
	 */

func bl31_entrypoint
	/*pre-loader to uboot argument Location*/
	ldr     x8, =BOOT_ARGUMENT_LOCATION
	str     w4, [x8]

	ldr     x8, =BOOT_ARGUMENT_SIZE
	str     w5, [x8]

	ldr     x8, =BL33_START_ADDRESS
	str     w6, [x8]

	/*pre-loader to TEE argument Location*/
	ldr     x8, =TEE_BOOT_INFO_ADDR
	str     w7, [x8]

	ldr	x9,	=DBG_CNT
	ldr	w10,	[x9]
	cmp	w10,	#0x5aa
	bne	bl31_on_entrypoint
	mov	w10,	#20
	str	w10,	[x9]
	beq	cold_boot

bl31_on_entrypoint:
	/* ---------------------------------------------------------------
	 * Preceding bootloader has populated x0 with a pointer to a
	 * 'bl31_params' structure & x1 with a pointer to platform
	 * specific structure
	 * ---------------------------------------------------------------
	 */

#ifdef CFG_MPCORE
	branch_if_master x0, x1, master_cpu

	/* Secondary CPUs */
slave_cpu:
	branch_if_cluster x0, cl0, cl1, cl2

cl0:
	/* wfe */
	ldr	x1, =0x10208008		/* CPU_RELEASE_ADDR */
	ldr	x0, [x1]
	mrs	x2, mpidr_el1
	and	x2, x2, #0xff
	cmp	x0, x2
	b.eq	master_cpu
	b.ne	cl0
	ldr	x0, =0x0
	str	x0, [x1]		/* restore 0 */
        
cl1:
	/* wfe */
	ldr	x1, =0x10208008		/* CPU_RELEASE_ADDR */
	ldr	x0, [x1]
        mrs	x2, mpidr_el1
        and	x2, x2, #0xff
        add	x2, x2, #4
        cmp	x0, x2
	b.eq	master_cpu
	b.ne	cl1
	ldr	x0, =0x0
	str	x0, [x1]		/* restore 0 */

cl2:
	/* wfe */
	ldr	x1, =0x10208008		/* CPU_RELEASE_ADDR */
	ldr	x0, [x1]
        mrs	x2, mpidr_el1
        and	x2, x2, #0x3
        add	x2, x2, #0x8
        cmp	x0, x2
	b.eq	cl2_c
	b.ne	cl2
cl2_c:
	ldr	x0, =0x0
	str	x0, [x1]		/* restore 0 */

master_cpu:
cold_boot:
#else
	mrs	x0,	mpidr_el1
	bl	platform_is_primary_cpu
	cbz	x0,	not_primary

	ldr	x9,	=DORMANT_LOG_BASE
	ldr	w9,	[x9]
	ldr	w9,	[x9]
	cbz	w9,	cold_boot

	ldr	x10,	=DORMANT_LOG_T0
	ldr	w10,	[x10]
	str	w10,	[x9]
	dsb	sy

not_primary:
cold_boot:
#endif

#if !RESET_TO_BL31
	mov	x20, x0
	mov	x21, x1
#else
	/* ---------------------------------------------
	 * Set the CPU endianness before doing anything
	 * that might involve memory reads or writes.
	 * ---------------------------------------------
	 */
	mrs	x0, sctlr_el3
	bic	x0, x0, #SCTLR_EE_BIT
	msr	sctlr_el3, x0
	isb

	/* -----------------------------------------------------
	 * Perform any processor specific actions upon reset
	 * e.g. cache, tlb invalidations etc. Override the
	 * Boot ROM(BL0) programming sequence
	 * -----------------------------------------------------
	 */
	bl	reset_handler
#endif
	/* ---------------------------------------------
	 * Enable the instruction cache, stack pointer
	 * and data access alignment checks
	 * ---------------------------------------------
	 */
	mov	x1, #(SCTLR_I_BIT | SCTLR_A_BIT | SCTLR_SA_BIT)
	mrs	x0, sctlr_el3
	orr	x0, x0, x1
	msr	sctlr_el3, x0
	isb

	/* ---------------------------------------------
	 * Initialise cpu_data early to enable crash
	 * reporting to have access to crash stack.
	 * Since crash reporting depends on cpu_data to
	 * report the unhandled exception, not
	 * doing so can lead to recursive exceptions due
	 * to a NULL TPIDR_EL3
	 * ---------------------------------------------
	 */
	bl	init_cpu_data_ptr

	/* ---------------------------------------------
	 * Set the exception vector.
	 * ---------------------------------------------
	 */
	adr	x1, runtime_exceptions
	msr	vbar_el3, x1
	isb

	/* ---------------------------------------------
	 * Enable the SError interrupt now that the
	 * exception vectors have been setup.
	 * ---------------------------------------------
	 */
	msr	daifclr, #DAIF_ABT_BIT

	/* ---------------------------------------------------------------------
	 * The initial state of the Architectural feature trap register
	 * (CPTR_EL3) is unknown and it must be set to a known state. All
	 * feature traps are disabled. Some bits in this register are marked as
	 * Reserved and should not be modified.
	 *
	 * CPTR_EL3.TCPAC: This causes a direct access to the CPACR_EL1 from EL1
	 *  or the CPTR_EL2 from EL2 to trap to EL3 unless it is trapped at EL2.
	 * CPTR_EL3.TTA: This causes access to the Trace functionality to trap
	 *  to EL3 when executed from EL0, EL1, EL2, or EL3. If system register
	 *  access to trace functionality is not supported, this bit is RES0.
	 * CPTR_EL3.TFP: This causes instructions that access the registers
	 *  associated with Floating Point and Advanced SIMD execution to trap
	 *  to EL3 when executed from any exception level, unless trapped to EL1
	 *  or EL2.
	 * ---------------------------------------------------------------------
	 */
	mrs	x1, cptr_el3
	bic	w1, w1, #TCPAC_BIT
	bic	w1, w1, #TTA_BIT
	bic	w1, w1, #TFP_BIT
	msr	cptr_el3, x1

#if RESET_TO_BL31
	/* -------------------------------------------------------
	 * Will not return from this macro if it is a warm boot.
	 * -------------------------------------------------------
	 */
	wait_for_entrypoint
	bl	platform_mem_init
#endif

	/* ---------------------------------------------
	 * Zero out NOBITS sections. There are 2 of them:
	 *   - the .bss section;
	 *   - the coherent memory section.
	 * ---------------------------------------------
	 */
	ldr	x0, =__BSS_START__
	ldr	x1, =__BSS_SIZE__
	bl	zeromem16

#if defined(CLEAN_BSS1)
	ldr	x0, =__BSS1_START__
	ldr	x1, =__BSS1_SIZE__
	bl	zeromem16
#endif

	ldr	x0, =__COHERENT_RAM_START__
	ldr	x1, =__COHERENT_RAM_UNALIGNED_SIZE__
	bl	zeromem16

	/* ---------------------------------------------
	 * Initialize the cpu_ops pointer.
	 * ---------------------------------------------
	 */
	bl	init_cpu_ops

	/* ---------------------------------------------
	 * Use SP_EL0 for the C runtime stack.
	 * ---------------------------------------------
	 */
	msr	spsel, #0

	/* --------------------------------------------
	 * Allocate a stack whose memory will be marked
	 * as Normal-IS-WBWA when the MMU is enabled.
	 * There is no risk of reading stale stack
	 * memory after enabling the MMU as only the
	 * primary cpu is running at the moment.
	 * --------------------------------------------
	 */
	mrs	x0, mpidr_el1
	bl	platform_set_stack

	/* ---------------------------------------------
	 * Perform platform specific early arch. setup
	 * ---------------------------------------------
	 */
#if RESET_TO_BL31
	mov	x0, 0
	mov	x1, 0
#else
	mov	x0, x20
	mov	x1, x21
#endif

	bl	bl31_early_platform_setup
	bl	bl31_plat_arch_setup

	/* ---------------------------------------------
	 * Jump to main function.
	 * ---------------------------------------------
	 */
	bl	bl31_main

	b	el3_exit

	/*pre-loader to uboot argument Location*/
.global BOOT_ARGUMENT_LOCATION
	.align	2
BOOT_ARGUMENT_LOCATION:
	.word 0x00000000

.global BOOT_ARGUMENT_SIZE
	.align	2
BOOT_ARGUMENT_SIZE:
	.word 0x00000000

.global BL33_START_ADDRESS
	.align	2
BL33_START_ADDRESS:
	.word 0x00000000

/*pre-loader to TEE argument Location*/
.global TEE_BOOT_INFO_ADDR
	.align	2
TEE_BOOT_INFO_ADDR:
    .word 0x00000000


.global DBG_CNT
	.align	2
DBG_CNT:
	.word 0x5aa



.global DORMANT_LOG_BASE
	/* for 8 bytes alignment */
	.align	3
DORMANT_LOG_BASE:
	.quad g_dormant_log_base

.global DORMANT_LOG_T0
	/* for 4 bytes alignment */
	.align	2
DORMANT_LOG_T0:
	.word 0xA7F00401

