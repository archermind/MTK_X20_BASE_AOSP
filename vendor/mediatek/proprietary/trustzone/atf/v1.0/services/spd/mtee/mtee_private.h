

#ifndef __MTEE_PRIVATE_H__
#define __MTEE_PRIVATE_H__

#include <context.h>
#include <arch.h>
#include <psci.h>
#include <interrupt_mgmt.h>
#include <platform_def.h>

#include <mtee.h>

/*******************************************************************************
 * Secure Payload PM state information e.g. SP is suspended, uninitialised etc
 * and macros to access the state information in the per-cpu 'state' flags
 ******************************************************************************/
#define MTEE_PSTATE_OFF		0
#define MTEE_PSTATE_ON		1
#define MTEE_PSTATE_SUSPEND	2
#define MTEE_PSTATE_SHIFT	0
#define MTEE_PSTATE_MASK	0x3
#define get_mtee_pstate(state)	((state >> MTEE_PSTATE_SHIFT) & MTEE_PSTATE_MASK)
#define clr_mtee_pstate(state)	(state &= ~(MTEE_PSTATE_MASK \
					    << MTEE_PSTATE_SHIFT))
#define set_mtee_pstate(st, pst)	do {					       \
					clr_mtee_pstate(st);		       \
					st |= (pst & MTEE_PSTATE_MASK) <<       \
						MTEE_PSTATE_SHIFT;	       \
				} while (0);

/*
 * This flag is used by the MTEE to determine if the MTEE is servicing a standard
 * SMC request prior to programming the next entry into the MTEE e.g. if MTEE
 * execution is preempted by a non-secure interrupt and handed control to the
 * normal world. If another request which is distinct from what the MTEE was
 * previously doing arrives, then this flag will be help the MTEE to either
 * reject the new request or service it while ensuring that the previous context
 * is not corrupted.
 */
#define STD_SMC_ACTIVE_FLAG_SHIFT	2
#define STD_SMC_ACTIVE_FLAG_MASK	1
#define get_std_smc_active_flag(state)	((state >> STD_SMC_ACTIVE_FLAG_SHIFT) \
					 & STD_SMC_ACTIVE_FLAG_MASK)
#define set_std_smc_active_flag(state)	(state |=                             \
					 1 << STD_SMC_ACTIVE_FLAG_SHIFT)
#define clr_std_smc_active_flag(state)	(state &=                             \
					 ~(STD_SMC_ACTIVE_FLAG_MASK           \
					   << STD_SMC_ACTIVE_FLAG_SHIFT))

/*******************************************************************************
 * Secure Payload execution state information i.e. aarch32 or aarch64
 ******************************************************************************/
#define MTEE_AARCH32		MODE_RW_32
#define MTEE_AARCH64		MODE_RW_64

/*******************************************************************************
 * The SPD should know the type of Secure Payload.
 ******************************************************************************/
#define MTEE_TYPE_UP		PSCI_TOS_NOT_UP_MIG_CAP
#define MTEE_TYPE_UPM		PSCI_TOS_UP_MIG_CAP
#define MTEE_TYPE_MP		PSCI_TOS_NOT_PRESENT_MP

/*******************************************************************************
 * Secure Payload migrate type information as known to the SPD. We assume that
 * the SPD is dealing with an MP Secure Payload.
 ******************************************************************************/
#define MTEE_MIGRATE_INFO		MTEE_TYPE_MP

/*******************************************************************************
 * Number of cpus that the present on this platform. TODO: Rely on a topology
 * tree to determine this in the future to avoid assumptions about mpidr
 * allocation
 ******************************************************************************/
#define MTEE_CORE_COUNT		PLATFORM_CORE_COUNT

/*******************************************************************************
 * Constants that allow assembler code to preserve callee-saved registers of the
 * C runtime context while performing a security state switch.
 ******************************************************************************/
#define TSPD_C_RT_CTX_X19		0x0
#define TSPD_C_RT_CTX_X20		0x8
#define TSPD_C_RT_CTX_X21		0x10
#define TSPD_C_RT_CTX_X22		0x18
#define TSPD_C_RT_CTX_X23		0x20
#define TSPD_C_RT_CTX_X24		0x28
#define TSPD_C_RT_CTX_X25		0x30
#define TSPD_C_RT_CTX_X26		0x38
#define TSPD_C_RT_CTX_X27		0x40
#define TSPD_C_RT_CTX_X28		0x48
#define TSPD_C_RT_CTX_X29		0x50
#define TSPD_C_RT_CTX_X30		0x58
#define TSPD_C_RT_CTX_SIZE		0x60
#define TSPD_C_RT_CTX_ENTRIES		(TSPD_C_RT_CTX_SIZE >> DWORD_SHIFT)

/*
 * The number of arguments to save during a SMC call for MTEE.
 * Currently only x1 and x2 are used by TSP.
 */
#define MTEE_NUM_ARGS	0x2

#ifndef __ASSEMBLY__

/* AArch64 callee saved general purpose register context structure. */
DEFINE_REG_STRUCT(c_rt_regs, TSPD_C_RT_CTX_ENTRIES);

/*
 * Compile time assertion to ensure that both the compiler and linker
 * have the same double word aligned view of the size of the C runtime
 * register context.
 */
CASSERT(TSPD_C_RT_CTX_SIZE == sizeof(c_rt_regs_t),	\
	assert_spd_c_rt_regs_size_mismatch);

/*******************************************************************************
 * Structure which helps the SPD to maintain the per-cpu state of the SP.
 * 'state'    - collection of flags to track SP state e.g. on/off
 * 'mpidr'    - mpidr to associate a context with a cpu
 * 'c_rt_ctx' - stack address to restore C runtime context from after returning
 *              from a synchronous entry into the SP.
 * 'cpu_ctx'  - space to maintain SP architectural state
 * 'monitorCallRegs' - area for monitor to mtee call parameter passing.
 ******************************************************************************/

typedef struct mtee_context {
        uint64_t saved_elr_el3;
        uint32_t saved_spsr_el3;
        uint32_t state;
        uint64_t mpidr;
        uint64_t c_rt_ctx;
        cpu_context_t cpu_ctx;
} mtee_context_t;

extern mtee_context_t mtee_sp_context[MTEE_CORE_COUNT];

// fastcall origin IDs for mtee
#define MTEE_SMC_NWD 0
#define MTEE_SMC_MONITOR 2

// mtee entrypoint offsets
#define ENTRY_OFFSET_FASTCALL 0x2C
#define ENTRY_OFFSET_FIQ 0x28
#define ENTRY_OFFSET_SMC 0x24

/*******************************************************************************
 * Tbase specific SMC ids
 ******************************************************************************/

// Fastcall ids
#define MTEE_SMC_FASTCALL_RETURN    (0xB2000001)

#define MTEE_SMC_FASTCALL_CONFIG_OK (0xB2000002)
#define MTEE_SMC_FASTCALL_CONFIG_VECTOR 1
#define MTEE_SMC_FASTCALL_CONFIG_DEBUG 2

#define MTEE_SMC_FASTCALL_OUTPUT    (0xB2000003)
#define MTEE_SMC_FASTCALL_OUTPUT_PUTC 1

#define MTEE_SMC_FASTCALL_STATUS    (0xB2000004)
#define MTEE_SMC_FASTCALL_STATUS_EXECUTION 1
#define MTEE_STATUS_NORMAL_BIT      0x01
#define MTEE_STATUS_FASTCALL_OK_BIT 0x02
#define MTEE_STATUS_SMC_OK_BIT      0x04

#define MTEE_STATUS_UNINIT          0x00
#define MTEE_STATUS_NORMAL          (MTEE_STATUS_NORMAL_BIT|MTEE_STATUS_FASTCALL_OK_BIT|MTEE_STATUS_SMC_OK_BIT)

#define MTEE_SMC_FASTCALL_INPUT    (0xB2000005)
#define MTEE_INPUT_HWIDENTITY      0x01
#define MTEE_INPUT_HWKEY           0x02
#define MTEE_INPUT_RNG             0x03



// Context for each core. gp registers not used by SPD.
extern mtee_context_t *secure_context;

/* mtee power management handlers */
extern const spd_pm_ops_t mtee_pm;

// ************************************************************************************

// secure_context for secodary cores are initialized
// when primary core returns from initialization call.
#define MTEE_INIT_NONE 0
#define MTEE_INIT_CONFIG_OK 1
#define MTEE_INIT_SYSREGS_OK 2

// Current status of initialization
extern uint64_t mteeInitStatus;

// Current status of execution
extern uint64_t mteeExecutionStatus;

// Entry vector start address in mtee
extern uint64_t mteeEntryBase;
// Tbase SPSR for SMC and FIQ handling.
extern uint32_t mteeEntrySpsr;



// ************************************************************************************
// Shared mtee monitor memory
// ************************************************************************************

// Page aligned start addresses for memory windows between mtee and
#define REGISTER_FILE_COUNT 2
#define REGISTER_FILE_NWD 0
#define REGISTER_FILE_MONITOR 1

extern uint64_t registerFileStart[REGISTER_FILE_COUNT];
extern uint64_t registerFileEnd[REGISTER_FILE_COUNT];


/*******************************************************************************
 * Function & Data prototypes
 ******************************************************************************/
int32_t mtee_init_secure_context(uint64_t entrypoint,
				 uint32_t rw,
				 uint64_t mpidr,
				 mtee_context_t *mtee_ctx);
extern void mtee_setup_entry(cpu_context_t *ns_context, uint32_t call_offset, uint32_t regfileNro);
extern uint64_t mtee_fiq_handler(uint32_t id, uint32_t flags, void *handle, void *cookie);


extern uint64_t mtee_enter_sp(uint64_t *c_rt_ctx);
extern void __dead2 mtee_exit_sp(uint64_t c_rt_ctx, uint64_t ret);
extern uint64_t mtee_synchronous_sp_entry(mtee_context_t *mtee_ctx);
extern void __dead2 mtee_synchronous_sp_exit(mtee_context_t *mtee_ctx, uint64_t ret);

extern uint64_t maskSWdRegister(uint64_t x);

extern int32_t mtee_fastcall_setup(void);
extern void save_sysregs_allcore();
extern void mtee_init_core(uint64_t mpidr);
extern void configure_mtee(uint64_t x1, uint64_t x2);

#if DEBUG
  #define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
  #define DBG_PRINTF(...)
#endif

#endif /*__ASSEMBLY__*/

#endif /* __MTEE_PRIVATE_H__ */
