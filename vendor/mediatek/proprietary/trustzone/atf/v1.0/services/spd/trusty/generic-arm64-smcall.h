/*
 * Copyright (c) 2015 Google Inc. All rights reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "smcall.h"

#define	SMC_ENTITY_PLATFORM_MONITOR	61

/*
 * SMC calls implemented by EL3 monitor
 */

/*
 * Write character in r1 to debug console
 */
#define SMC_FC_DEBUG_PUTC	SMC_FASTCALL_NR(SMC_ENTITY_PLATFORM_MONITOR, 0x0)

/*
 * Get register base address
 * r1: SMC_GET_GIC_BASE_GICD or SMC_GET_GIC_BASE_GICC
 */
#define SMC_GET_GIC_BASE_GICD	0
#define SMC_GET_GIC_BASE_GICC	1
#define SMC_FC_GET_REG_BASE	SMC_FASTCALL_NR(SMC_ENTITY_PLATFORM_MONITOR, 0x1)
#define SMC_FC64_GET_REG_BASE	SMC_FASTCALL64_NR(SMC_ENTITY_PLATFORM_MONITOR, 0x1)
