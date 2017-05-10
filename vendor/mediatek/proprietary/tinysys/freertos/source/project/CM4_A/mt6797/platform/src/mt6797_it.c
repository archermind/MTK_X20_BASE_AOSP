/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "platform.h"
#include "stdarg.h"
#include "string.h"
#include <scp_ipi.h>
#include <tinysys_config.h>
#include <mt_reg_base.h>
#include <driver_api.h>

enum { r0, r1, r2, r3, r12, lr, pc, psr};

typedef struct TaskContextType {
    unsigned int r0;
    unsigned int r1;
    unsigned int r2;
    unsigned int r3;
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
    unsigned int r8;
    unsigned int r9;
    unsigned int r10;
    unsigned int r11;
    unsigned int r12;
    unsigned int sp;              /* after pop r0-r3, lr, pc, xpsr                   */
    unsigned int lr;              /* lr before exception                             */
    unsigned int pc;              /* pc before exception                             */
    unsigned int psr;             /* xpsr before exeption                            */
    unsigned int control;         /* nPRIV bit & FPCA bit meaningful, SPSEL bit = 0  */
    unsigned int exc_return;      /* current lr                                      */
    unsigned int msp;             /* msp                                             */
} TaskContext;

#define CRASH_SUMMARY_LENGTH 12
#define CRASH_MEMORY_LENGTH  (512*1024/4)
#define EIGHT_BYTE_ALIGN_IND (1 << 9)

typedef struct MemoryDumpType {
    char crash_summary[CRASH_SUMMARY_LENGTH];
    TaskContext core;
    int flash_length;
    int sram_length;
    int memory[CRASH_MEMORY_LENGTH];
} MemoryDump;

typedef struct {
    int task_context_addr;
    int dump_start;
    int length;
} RAM_DUMP_INFO;

#define FULL_MEMORY_DUMP 0
#define MINI_MEMORY_DUMP 0
#define EXCEPTION_HALT 1

static TaskContext taskCtxDump __attribute__ ((section (".share")));
TaskContext *pTaskContext = &taskCtxDump;

#ifdef CFG_RAMDUMP_SUPPORT
#if (MINI_MEMORY_DUMP || FULL_MEMORY_DUMP)
static MemoryDump *pMemoryDump;
#endif

extern unsigned long __RTOS_segment_start__[];
extern unsigned long __RTOS_segment_end__[];

void mt_ram_dump_init(void)
{
    void* task_context_dump_addr = &taskCtxDump;

    memset(&taskCtxDump, 0, sizeof(taskCtxDump));
    PRINTF("send IPI to init ram dump,ramdump_addr:0x%x, size:%d\n\r",task_context_dump_addr,sizeof(task_context_dump_addr));
    while (scp_ipi_send(IPI_RAM_DUMP,(void *)&task_context_dump_addr, sizeof(task_context_dump_addr), 1, IPI_SCP2AP) != DONE);
}
#endif

void triggerException(void)
{
    SCB->CCR |= 0x10; /* DIV_0_TRP */
    __ASM volatile("mov r0, #0");
    __ASM volatile("UDIV r0, r0");
    for(;;);
}

#if FULL_MEMORY_DUMP
static void memoryDumpAll(void)
{
    uint32_t current, end;
    uint32_t offset;
    current = (uint32_t)__RTOS_segment_start__;
    end     = (uint32_t)__RTOS_segment_end__;
    pMemoryDump->flash_length = end - current;

    PRINTF("flash_length:0x%08x\n\r",pMemoryDump->flash_length);
    PRINTF("memory:0x%08x\n\r",pMemoryDump->memory);
    for (offset=0; current < end; current += 16,offset +=4) {

        if ((*(uint32_t*) current) == 0xFFFFFFFF &&
                (*(uint32_t*) (current+4)) == 0xFFFFFFFF &&
                (*(uint32_t*) (current+8)) == 0xFFFFFFFF &&
                (*(uint32_t*) (current+12)) == 0xFFFFFFFF )
            continue;

        if ((*(uint32_t*) current) == 0 &&
                (*(uint32_t*) (current+4)) == 0 &&
                (*(uint32_t*) (current+8)) == 0 &&
                (*(uint32_t*) (current+12)) == 0 )
            continue;

        pMemoryDump->memory[offset]    = *((unsigned int*)current);
        pMemoryDump->memory[offset+1]  = *((unsigned int*)current+ 1);
        pMemoryDump->memory[offset+2]  = *((unsigned int*)current+ 2);
        pMemoryDump->memory[offset+3]  = *((unsigned int*)current+ 3);
        PRINTF("0x%08x: %08x %08x %08x %08x\n\r", (unsigned int)current, *((unsigned int*)current), *((unsigned int*) (current+4)), *((unsigned int*)(current+8)), *((unsigned int*)(current+12)));
    }
}
#endif

#if MINI_MEMORY_DUMP

#define NEAR_OFFSET 0x100

static uint32_t isValidMemory(uint32_t addr)
{
    addr = addr - addr%4;

    if ((addr < (uint32_t)__RTOS_segment_start__ ) || (addr > (uint32_t)__RTOS_segment_end__))
        return 0;
    return 1;
}

void nearDump(uint32_t addr)
{
    uint32_t current, end;

    addr = addr - addr %16;
    current = addr - NEAR_OFFSET/2;
    end = addr + NEAR_OFFSET/2;

    if (current < (uint32_t)__RTOS_segment_start__)
        current = (uint32_t)__RTOS_segment_start__;

    if (end > (uint32_t)__RTOS_segment_end__)
        end = (uint32_t)__RTOS_segment_end__;

    for (; current < end; current += 16) {
        PRINTF("0x%08x: %08x %08x %08x %08x\n\r", (unsigned int)current, *((unsigned int*)current), *((unsigned int*) (current+4)), *((unsigned int*)(current+8)), *((unsigned int*)(current+12)));
    }
}

static void memoryDump(TaskContext *pTaskContext)
{
    if (isValidMemory(pTaskContext->r0)) {
        PRINTF("\n\rMemory near r0:\n\r");
        nearDump(pTaskContext->r0);
    }
    if (isValidMemory(pTaskContext->r1)) {
        PRINTF("\n\rMemory near r1:\n\r");
        nearDump(pTaskContext->r1);
    }
    if (isValidMemory(pTaskContext->r2)) {
        PRINTF("\n\rMemory near r2:\n\r");
        nearDump(pTaskContext->r2);
    }
    if (isValidMemory(pTaskContext->r3)) {
        PRINTF("\n\rMemory near r3:\n\r");
        nearDump(pTaskContext->r3);
    }
    if (isValidMemory(pTaskContext->r4)) {
        PRINTF("\n\rMemory near r4:\n\r");
        nearDump(pTaskContext->r4);
    }
    if (isValidMemory(pTaskContext->r5)) {
        PRINTF("\n\rMemory near r5:\n\r");
        nearDump(pTaskContext->r5);
    }
    if (isValidMemory(pTaskContext->r6)) {
        PRINTF("\n\rMemory near r6:\n\r");
        nearDump(pTaskContext->r6);
    }
    if (isValidMemory(pTaskContext->r7)) {
        PRINTF("\n\rMemory near r7:\n\r");
        nearDump(pTaskContext->r7);
    }
    if (isValidMemory(pTaskContext->r8)) {
        PRINTF("\n\rMemory near r8:\n\r");
        nearDump(pTaskContext->r8);
    }
    if (isValidMemory(pTaskContext->r9)) {
        PRINTF("\n\rMemory near r9:\n\r");
        nearDump(pTaskContext->r9);
    }
    if (isValidMemory(pTaskContext->r10)) {
        PRINTF("\n\rMemory near r10:\n\r");
        nearDump(pTaskContext->r10);
    }
    if (isValidMemory(pTaskContext->r11)) {
        PRINTF("\n\rMemory near r11:\n\r");
        nearDump(pTaskContext->r11);
    }
    if (isValidMemory(pTaskContext->r12)) {
        PRINTF( "\n\rMemory near r12:\n\r");
        nearDump(pTaskContext->r12);
    }
    if (isValidMemory(pTaskContext->lr)) {
        PRINTF( "\n\rMemory near lr:\n\r");
        nearDump(pTaskContext->lr);
    }
    if (isValidMemory(pTaskContext->pc)) {
        PRINTF( "\n\rMemory near pc:\n\r");
        nearDump(pTaskContext->pc);
    }
    if (isValidMemory(pTaskContext->sp)) {
        PRINTF( "\n\rMemory near sp:\n\r");
        nearDump(pTaskContext->sp);
        nearDump(pTaskContext->sp + NEAR_OFFSET);
    }
}
#endif

static void stackDump(uint32_t stack[])
{
#ifdef CFG_RAMDUMP_SUPPORT
    taskCtxDump.r0   = stack[r0];
    taskCtxDump.r1   = stack[r1];
    taskCtxDump.r2   = stack[r2];
    taskCtxDump.r3   = stack[r3];
    taskCtxDump.r12  = stack[r12];
    taskCtxDump.lr   = stack[lr];
    taskCtxDump.pc   = stack[pc];
    taskCtxDump.psr  = stack[psr];
    taskCtxDump.sp   = ((uint32_t)stack)+0x20;
    if (taskCtxDump.psr & EIGHT_BYTE_ALIGN_IND)
        taskCtxDump.sp += 4;

    PRINTF("Core reg dump before exception happened\n\r");
    PRINTF("r0  = 0x%08x\n\r", taskCtxDump.r0);
    PRINTF("r1  = 0x%08x\n\r", taskCtxDump.r1);
    PRINTF("r2  = 0x%08x\n\r", taskCtxDump.r2);
    PRINTF("r3  = 0x%08x\n\r", taskCtxDump.r3);
    PRINTF("r4  = 0x%08x\n\r", taskCtxDump.r4);
    PRINTF("r5  = 0x%08x\n\r", taskCtxDump.r5);
    PRINTF("r6  = 0x%08x\n\r", taskCtxDump.r6);
    PRINTF("r7  = 0x%08x\n\r", taskCtxDump.r7);
    PRINTF("r8  = 0x%08x\n\r", taskCtxDump.r8);
    PRINTF("r9  = 0x%08x\n\r", taskCtxDump.r9);
    PRINTF("r10 = 0x%08x\n\r", taskCtxDump.r10);
    PRINTF("r11 = 0x%08x\n\r", taskCtxDump.r11);
    PRINTF("r12 = 0x%08x\n\r", taskCtxDump.r12);
    PRINTF("lr  = 0x%08x\n\r", taskCtxDump.lr);
    PRINTF("pc  = 0x%08x\n\r", taskCtxDump.pc);
    PRINTF("psr = 0x%08x\n\r", taskCtxDump.psr);
    PRINTF("EXC_RET = 0x%08x\n\r", taskCtxDump.exc_return);

    /* update CONTROL.SPSEL if returning to thread mode */
    if (taskCtxDump.exc_return & 0x4)
        taskCtxDump.control |= 0x2;
    else /* update msp if returning to handler mode */
        taskCtxDump.msp = taskCtxDump.sp;

    PRINTF("CONTROL = 0x%08x\n\r", taskCtxDump.control);
    PRINTF("MSP     = 0x%08x\n\r", taskCtxDump.msp);
    PRINTF("sp      = 0x%08x\n\r", taskCtxDump.sp);
#endif
}

void crash_dump(uint32_t stack[])
{
    stackDump(stack);

#if MINI_MEMORY_DUMP
    memoryDump(&taskCtxDump);
#endif

#if FULL_MEMORY_DUMP
    memoryDumpAll();
#endif

#if EXCEPTION_HALT
#if DEBUGGER_ON
    __ASM volatile("BKPT #01");
#else
    while(1);
#endif
#else
    SCB->CFSR = SCB->CFSR;
#endif

}

void printUsageErrorMsg(uint32_t CFSRValue)
{
    PRINTF("Usage fault: ");
    CFSRValue >>= 16;                  // right shift to lsb
    if((CFSRValue & (1 << 9)) != 0) {
        PRINTF("Divide by zero\n\r");
    }
    if((CFSRValue & (1 << 8)) != 0) {
        PRINTF("Unaligned access\n\r");
    }
    if((CFSRValue & (1 << 0)) != 0) {
        PRINTF("Undefined instruction\n\r");
    }
}

void printBusFaultErrorMsg(uint32_t CFSRValue)
{
    PRINTF("Bus fault: ");
    CFSRValue &= 0x0000FF00; // mask just bus faults
    CFSRValue >>= 8;
    if((CFSRValue & (1<<5)) != 0) {
        PRINTF("A bus fault occurred during FP lazy state preservation\n\r");
    }
    if((CFSRValue & (1<<4)) != 0) {
        PRINTF("A derived bus fault has occurred on exception entry\n\r");
    }
    if((CFSRValue & (1<<3)) != 0) {
        PRINTF("A derived bus fault has occurred on exception return\n\r");
    }
    if((CFSRValue & (1<<2)) != 0) {
        PRINTF("Imprecise data access error has occurred\n\r");
    }
    if((CFSRValue & (1<<1)) != 0) { /* Need to check valid bit (bit 7 of CFSR)? */
        PRINTF("A precise data access error has occurred @x%08x\n\r", (unsigned int)SCB->BFAR);
    }
    if((CFSRValue & (1<<0)) != 0) {
        PRINTF("A bus fault on an instruction prefetch has occurred\n\r");
    }
    if((CFSRValue & (1<<7)) != 0) { /* To review: remove this if redundant */
        PRINTF("SCB->BFAR = 0x%08x\n\r", (unsigned int)SCB->BFAR );
    }
}

void printMemoryManagementErrorMsg(uint32_t CFSRValue)
{
    PRINTF("Memory Management fault: ");
    CFSRValue &= 0x000000FF; // mask just mem faults
    if((CFSRValue & (1<<5)) != 0) {
        PRINTF("A MemManage fault occurred during FP lazy state preservation\n\r");
    }
    if((CFSRValue & (1<<4)) != 0) {
        PRINTF("A derived MemManage fault occurred on exception entry\n\r");
    }
    if((CFSRValue & (1<<3)) != 0) {
        PRINTF("A derived MemManage fault occurred on exception return\n\r");
    }
    if((CFSRValue & (1<<1)) != 0) { /* Need to check valid bit (bit 7 of CFSR)? */
        PRINTF("Data access violation @0x%08x\n\r", (unsigned int)SCB->MMFAR);
    }
    if((CFSRValue & (1<<0)) != 0) {
        PRINTF("MPU or Execute Never (XN) default memory map access violation\n\r");
    }
    if((CFSRValue & (1<<7)) != 0) { /* To review: remove this if redundant */
        PRINTF("SCB->MMFAR = 0x%08x\n\r", (unsigned int)SCB->MMFAR );
    }
}

/**
 * @brief   This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
}

void Hard_Fault_Handler(uint32_t stack[])
{
    PRINTF("\n\rIn Hard Fault Handler\n\r");
    PRINTF("SCB->HFSR = 0x%08x\n\r", (unsigned int)SCB->HFSR);
    if ((SCB->HFSR & (1 << 30)) != 0) {
        PRINTF("Forced Hard Fault\n\r");
        PRINTF("SCB->CFSR = 0x%08x\n\r", (unsigned int)SCB->CFSR );
        if((SCB->CFSR & 0xFFFF0000) != 0) {
            printUsageErrorMsg(SCB->CFSR);
        }
        if((SCB->CFSR & 0x0000FF00) !=0 ) {
            printBusFaultErrorMsg(SCB->CFSR);
        }
        if((SCB->CFSR & 0x000000FF) !=0 ) {
            printMemoryManagementErrorMsg(SCB->CFSR);
        }
    }

    crash_dump(stack);
}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{
    __asm volatile
    (
        "ldr r0, =pTaskContext         \n"
        "ldr r0, [r0]                  \n"
        "add r0, r0, #16               \n"     /* point to context.r4          */
        "stmia r0!, {r4-r11}           \n"     /* store r4-r11                 */
        "add r0, #20                   \n"     /* point to context.control     */
        "mrs r1, control               \n"     /* move CONTROL to r1           */
        "str r1, [r0], #4              \n"     /* store CONTROL                */
        "str lr, [r0], #4              \n"     /* store EXC_RETURN             */
        "mrs r1, msp                   \n"     /* move MSP to r1               */
        "str r1, [r0]                  \n"     /* store MSP                    */
        "tst lr, #4                    \n"     /* thread or handler mode?      */
        "ite eq                        \n"
        "mrseq r0, msp                 \n"
        "mrsne r0, psp                 \n"
        "push {lr}                     \n"
        "bl Hard_Fault_Handler         \n"
        "pop {lr}                      \n"
        "bx lr                         \n"
    );
}

void Bus_Fault_Handler(uint32_t stack[])
{
    PRINTF("\n\rIn Bus Fault Handler\n\r");
    PRINTF("SCB->CFSR = 0x%08x\n\r", (unsigned int)SCB->CFSR );
    if((SCB->CFSR & 0xFF00) != 0) {
        printBusFaultErrorMsg(SCB->CFSR);
    }

    crash_dump(stack);
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
    __asm volatile
    (
        "ldr r0, =pTaskContext         \n"
        "ldr r0, [r0]                  \n"
        "add r0, r0, #16               \n"     /* point to context.r4          */
        "stmia r0!, {r4-r11}           \n"     /* store r4-r11                 */
        "add r0, #20                   \n"     /* point to context.control     */
        "mrs r1, control               \n"     /* move CONTROL to r1           */
        "str r1, [r0], #4              \n"     /* store CONTROL                */
        "str lr, [r0], #4              \n"     /* store EXC_RETURN             */
        "mrs r1, msp                   \n"     /* move MSP to r1               */
        "str r1, [r0]                  \n"     /* store MSP                    */
        "tst lr, #4                    \n"     /* thread or handler mode?      */
        "ite eq                        \n"
        "mrseq r0, msp                 \n"
        "mrsne r0, psp                 \n"
        "push {lr}                     \n"
        "bl Bus_Fault_Handler          \n"
        "pop {lr}                      \n"
        "bx lr                         \n"
    );
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Fault_Handler(uint32_t stack[])
{
    PRINTF("\n\rIn MemManage Fault Handler\n\r");
    PRINTF("SCB->CFSR = 0x%08x\n\r", (unsigned int)SCB->CFSR );
    if((SCB->CFSR & 0xFF) != 0) {
        printMemoryManagementErrorMsg(SCB->CFSR);
    }

    crash_dump(stack);
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
    __asm volatile
    (
        "ldr r0, =pTaskContext         \n"
        "ldr r0, [r0]                  \n"
        "add r0, r0, #16               \n"     /* point to context.r4          */
        "stmia r0!, {r4-r11}           \n"     /* store r4-r11                 */
        "add r0, #20                   \n"     /* point to context.control     */
        "mrs r1, control               \n"     /* move CONTROL to r1           */
        "str r1, [r0], #4              \n"     /* store CONTROL                */
        "str lr, [r0], #4              \n"     /* store EXC_RETURN             */
        "mrs r1, msp                   \n"     /* move MSP to r1               */
        "str r1, [r0]                  \n"     /* store MSP                    */
        "tst lr, #4                    \n"     /* thread or handler mode?      */
        "ite eq                        \n"
        "mrseq r0, msp                 \n"
        "mrsne r0, psp                 \n"
        "push {lr}                     \n"
        "bl MemManage_Fault_Handler    \n"
        "pop {lr}                      \n"
        "bx lr                         \n"
    );
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1) {
    }
}

void Debug_Mon_Handler(uint32_t stack[])
{
    PRINTF("\n\rIn Debug Mon Fault Handler\n\r");
    PRINTF("SCB->CFSR = 0x%08x\n\r", (unsigned int)SCB->CFSR );

    if (SCB->DFSR & SCB_DFSR_DWTTRAP_Msk) {
        PRINTF("A debug event generated by the DWT\n\r");
        /*
        		 PRINTF_D("COMP0: %8lx \t MASK0: %8lx \t FUNC0: %8lx \n\r", DWT->COMP0,DWT->MASK0,DWT->FUNCTION0);
        		 PRINTF_D("COMP1: %8lx \t MASK1: %8lx \t FUNC1: %8lx \n\r", DWT->COMP1,DWT->MASK1,DWT->FUNCTION1);
        		 PRINTF_D("COMP2: %8lx \t MASK2: %8lx \t FUNC2: %8lx \n\r", DWT->COMP2,DWT->MASK2,DWT->FUNCTION2);
        		 PRINTF_D("COMP3: %8lx \t MASK3: %8lx \t FUNC3: %8lx \n\r", DWT->COMP3,DWT->MASK3,DWT->FUNCTION3);
        */
    }

    crash_dump(stack);

    //PRINTF_D("LR:0x%x\n",drv_reg32(REG_LR));
    //PRINTF_D("PC:0x%x\n",drv_reg32(REG_PC));
    //PRINTF_D("PSP:0x%x\n",drv_reg32(REG_PSP));
    //PRINTF_D("SP:0x%x\n",drv_reg32(REG_SP));
}
/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void)
{
//TODO: implement here !!!
//    PRINTF("\n\rEnter Debug Monitor Handler\n\r");
//    PRINTF("SCB->DFSR = 0x%08x\n\r", (unsigned int)SCB->DFSR );
    __asm volatile
    (
        "ldr r0, =pTaskContext         \n"
        "ldr r0, [r0]                  \n"
        "add r0, r0, #16               \n"     /* point to context.r4          */
        "stmia r0!, {r4-r11}           \n"     /* store r4-r11                 */
        "add r0, #20                   \n"     /* point to context.control     */
        "mrs r1, control               \n"     /* move CONTROL to r1           */
        "str r1, [r0], #4              \n"     /* store CONTROL                */
        "str lr, [r0], #4              \n"     /* store EXC_RETURN             */
        "mrs r1, msp                   \n"     /* move MSP to r1               */
        "str r1, [r0]                  \n"     /* store MSP                    */
        "tst lr, #4                    \n"     /* thread or handler mode?      */
        "ite eq                        \n"
        "mrseq r0, msp                 \n"
        "mrsne r0, psp                 \n"
        "push {lr}                     \n"
        "bl Debug_Mon_Handler         \n"
        "pop {lr}                      \n"
        "bx lr                         \n"
    );
}
