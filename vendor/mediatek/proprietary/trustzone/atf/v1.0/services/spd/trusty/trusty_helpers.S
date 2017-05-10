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

#include <asm_macros.S>

.macro push ra, rb, sp=sp
	stp \ra, \rb, [\sp,#-16]!
.endm

.macro pop ra, rb, sp=sp
	ldp \ra, \rb, [\sp], #16
.endm

	.global trusty_context_switch_helper
func trusty_context_switch_helper
	push	x8, xzr
	push	x19, x20
	push	x21, x22
	push	x23, x24
	push	x25, x26
	push	x27, x28
	push	x29, x30

	mov	x9, sp
	ldr	x10, [x0]
	mov	sp, x10
	str	x9, [x0]

	pop	x29, x30
	pop	x27, x28
	pop	x25, x26
	pop	x23, x24
	pop	x21, x22
	pop	x19, x20
	pop	x8, xzr
	stp	x1, x2, [x8]
	stp	x3, x4, [x8, #16]

	ret

	.global trusty_init_context_stack
func trusty_init_context_stack
	push	x8, xzr, x1
	push	xzr, xzr, x1
	push	xzr, xzr, x1
	push	xzr, xzr, x1
	push	xzr, xzr, x1
	push	xzr, xzr, x1
	adr	x9, el3_exit
	push	xzr, x9, x1
	str	x1, [x0]
	ret
