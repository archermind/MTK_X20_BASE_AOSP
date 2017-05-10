/*
 * Copyright (c) 2014, ARM Limited and Contributors. All rights reserved.
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

#ifndef __PLAT_PRIVATE_H__
#define __PLAT_PRIVATE_H__

#include <bl_common.h>
#include <platform_def.h>

#define DEVINFO_SIZE 4

#define LINUX_KERNEL_32 0
#define LINUX_KERNEL_64 1

typedef volatile struct mailbox {
	unsigned long value
	__attribute__((__aligned__(CACHE_WRITEBACK_GRANULE)));
} mailbox_t;

/*******************************************************************************
 * This structure represents the superset of information that is passed to
 * BL31 e.g. while passing control to it from BL2 which is bl31_params
 * and bl31_plat_params and its elements
 ******************************************************************************/
typedef struct bl2_to_bl31_params_mem {
	bl31_params_t bl31_params;
	image_info_t bl31_image_info;
	image_info_t bl32_image_info;
	image_info_t bl33_image_info;
	entry_point_info_t bl33_ep_info;
	entry_point_info_t bl32_ep_info;
	entry_point_info_t bl31_ep_info;
} bl2_to_bl31_params_mem_t;

typedef struct {
    unsigned int atf_magic;
    unsigned int tee_support;
    unsigned int tee_entry;
    unsigned int tee_boot_arg_addr;
    unsigned int hwuid[4];     // HW Unique id for t-base used
    unsigned int HRID[2];      // HW random id for t-base used
    unsigned int atf_log_port;
    unsigned int atf_log_baudrate;
    unsigned int atf_log_buf_start;
    unsigned int atf_log_buf_size;
    unsigned int atf_irq_num;
    unsigned int devinfo[DEVINFO_SIZE];
    unsigned int atf_aee_debug_buf_start;
    unsigned int atf_aee_debug_buf_size;
} atf_arg_t, *atf_arg_t_ptr;

struct kernel_info {
    uint64_t pc;
    uint64_t r0;
    uint64_t r1;
    uint64_t r2;
    uint64_t k32_64;
};

/*******************************************************************************
 * Forward declarations
 ******************************************************************************/
struct meminfo;

extern atf_arg_t gteearg;
/*******************************************************************************
 * Function and variable prototypes
 ******************************************************************************/
void plat_configure_mmu_el1(unsigned long total_base,
			   unsigned long total_size,
			   unsigned long,
			   unsigned long,
			   unsigned long,
			   unsigned long);
void plat_configure_mmu_el3(unsigned long total_base,
			   unsigned long total_size,
			   unsigned long,
			   unsigned long,
			   unsigned long,
			   unsigned long);
int plat_config_setup(void);

void plat_cci_init(void);
void plat_cci_enable(void);

/* Declarations for mt_gic.c */
void gic_cpuif_deactivate(unsigned int);
void gic_cpuif_setup(unsigned int);
void gic_pcpu_distif_setup(unsigned int);
void gic_setup(void);

/* Declarations for plat_topology.c */
int plat_setup_topology(void);

/* Declarations for plat_io_storage.c */
void plat_io_setup(void);

/* Declarations for plat_security.c */
void plat_security_setup(void);

/* Gets the SPR for BL32 entry */
uint32_t plat_get_spsr_for_bl32_entry(void);

/* Gets the SPSR for BL33 entry */
uint32_t plat_get_spsr_for_bl33_entry(void);

void enable_ns_access_to_cpuectlr(void);
//L2ACTLR must be written before MMU on and any ACE, CHI or ACP traffic
int workaround_836870(unsigned long mpidr);
int clear_cntvoff(unsigned long mpidr);

uint64_t get_kernel_k32_64(void);
uint64_t get_kernel_info_pc(void);
uint64_t get_kernel_info_r0(void);
uint64_t get_kernel_info_r1(void);
uint64_t get_kernel_info_r2(void);
int is_mp0_off(void);
#endif /* __PLAT_PRIVATE_H__ */
