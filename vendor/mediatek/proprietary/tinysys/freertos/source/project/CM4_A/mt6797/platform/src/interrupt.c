#include "FreeRTOS.h"
#include <interrupt.h>
#include <task.h>
#include <mt_gpt.h>
#include <limits.h>

void __real_vPortEnterCritical();
void __real_vPortExitCritical();
static uint64_t start_cs_time = 0;
static uint64_t end_cs_time = 0;
static uint64_t max_cs_duration_time = 0;
static uint64_t cs_duration_time = 0;
static uint64_t max_cs_limit = 0;
extern UBaseType_t uxCriticalNesting;
/** get irq mask status
*
*  @param irq interrupt number
*
*  @returns
*    irq mask status
*/
uint32_t get_mask_irq(IRQn_Type irq)
{
    return NVIC->ISER[((uint32_t)((int32_t)irq) >> 5)] & (1 << ((uint32_t)((int32_t)irq) & 0x1F));
}

/** enable irq
*
*  @param irq interrupt number
*
*  @returns
*    no return
*/
void unmask_irq(IRQn_Type irq)
{
    if (irq < IRQ_MAX_CHANNEL && irq_desc[irq].handler != NULL) {
        NVIC_EnableIRQ(irq);
    } else {
        PRINTF("ERROR Interrupt ID %lu\n\r", irq);
    }
}

/** disable irq
*
*  @param irq interrupt number
*
*  @returns
*    no return
*/
void mask_irq(IRQn_Type irq)
{
    if (irq < IRQ_MAX_CHANNEL && irq_desc[irq].handler != NULL) {
        NVIC_DisableIRQ(irq);
    } else {
        PRINTF("ERROR Interrupt ID %lu\n\r", irq);
    }
}

void wakeup_source_count(uint32_t wakeup_src)
{
    int32_t id;
    for (id = 0; id < IRQ_MAX_CHANNEL; id++) {
        if (wakeup_src & (1 << id))
            irq_desc[id].wakeup_count ++;
    }

}

/** register a irq handler
*
*  @param irq interrupt number
*  @param irq interrupt handler
*  @param irq interrupt name
*
*  @returns
*    no return
*/
void request_irq(uint32_t irq, irq_handler_t handler, const char *name)
{
    if (irq < IRQ_MAX_CHANNEL) {
        NVIC_DisableIRQ(irq);
        irq_desc[irq].name = name;
        irq_desc[irq].handler = handler;
        irq_desc[irq].priority = DEFAULT_IRQ_PRIORITY;
        irq_desc[irq].irq_count = 0;
        irq_desc[irq].wakeup_count = 0;
        irq_desc[irq].last_enter = 0;
        irq_desc[irq].last_exit = 0;
        irq_desc[irq].max_duration = 0;
        NVIC_SetPriority(irq, irq_desc[irq].priority);
        NVIC_EnableIRQ(irq);
    } else {
        PRINTF("ERROR Interrupt ID %lu\n\r", irq);
    }
}

/** interrupt handler entry & dispatcher
*
*  @param
*
*  @returns
*    no return
*/

void hw_isr_dispatch(void)
{
    uint32_t ulCurrentInterrupt;
    uint64_t duration;

    /* Obtain the number of the currently executing interrupt. */
    __asm volatile("mrs %0, ipsr" : "=r"(ulCurrentInterrupt));
    /* skip the CM4 built-in interrupts:16 */
    ulCurrentInterrupt = ulCurrentInterrupt - 16;
    //PRINTF("Interrupt ID %lu\n", ulCurrentInterrupt);
    if (ulCurrentInterrupt < IRQ_MAX_CHANNEL) {
        if (irq_desc[ulCurrentInterrupt].handler) {
            irq_desc[ulCurrentInterrupt].irq_count ++;
            /* record the last handled interrupt duration, unit: (ns)*/
            irq_desc[ulCurrentInterrupt].last_enter = read_xgpt_stamp_ns();
            irq_desc[ulCurrentInterrupt].handler();
            irq_desc[ulCurrentInterrupt].last_exit = read_xgpt_stamp_ns();
            duration = irq_desc[ulCurrentInterrupt].last_exit - irq_desc[ulCurrentInterrupt].last_enter;
            /* handle the xgpt overflow case
            * discard the duration time when exit time > enter time
            * */
            if (irq_desc[ulCurrentInterrupt].last_exit > irq_desc[ulCurrentInterrupt].last_enter)
                irq_desc[ulCurrentInterrupt].max_duration = duration;
        }
    } else {
        PRINTF("ERROR Interrupt ID %lu\n\r", ulCurrentInterrupt);
    }
}

/** interrupt status query function
*
*  @param
*
*  @returns
*    no return
*/
void irq_status_dump(void)
{
    int32_t id;

    PRINTF_E("id\tname\tpriority(HW)\tcount\tlast\tenable\tpending\tactive\n\r");
    for (id = NonMaskableInt_IRQn; id < 0; id++) {
        PRINTF_E("%d\t%s\t%d(%d)\t\t%d\t%d\t%s\t%s\t%s\n\r",
                 id,
                 "n/a",
                 0,
                 NVIC_GetPriority(id),
                 0,
                 0,
                 get_mask_irq(id) ? "enable" : "disable",
                 "n/a",
                 "n/a");
    }
    for (id = 0; id < IRQ_MAX_CHANNEL; id++) {
        PRINTF_E("%d\t%s\t%d(%d)\t\t%d\t%llu\t%s\t%s\t%s\n\r",
                 id,
                 (irq_desc[id].name) ? irq_desc[id].name : "n/a",
                 irq_desc[id].priority,
                 NVIC_GetPriority(id),
                 irq_desc[id].irq_count,
                 irq_desc[id].last_exit,
                 get_mask_irq(id) ? "enable" : "disable",
                 NVIC_GetPendingIRQ(id) ? "enable" : "no",
                 NVIC_GetActive(id) ? "enable" : "inactive");
    }
}
void set_max_cs_limit(uint64_t limit_time)
{
    if (max_cs_limit == 0)
        max_cs_limit = limit_time;
    else {
        PRINTF_D("set_max_cs_limit(%llu) failed\n\r", limit_time);
        PRINTF_D("Already set max context switch limit:%llu,\n\rPlease run disable_cs_limit() to disiable it first\n",
                 max_cs_limit);
    }
}

void disable_cs_limit(void)
{
    max_cs_limit = 0;
}

void __wrap_vPortEnterCritical()
{
    __real_vPortEnterCritical();
    if (uxCriticalNesting == 1 && max_cs_limit != 0)
        start_cs_time = read_xgpt_stamp_ns() ;
}

void __wrap_vPortExitCritical()
{
    if ((uxCriticalNesting - 1) == 0 && max_cs_limit != 0) {
        end_cs_time = read_xgpt_stamp_ns() ;
        if (end_cs_time > start_cs_time) {
            cs_duration_time = end_cs_time - start_cs_time;
            if (cs_duration_time > max_cs_duration_time)
                max_cs_duration_time = cs_duration_time;
            if (max_cs_duration_time > max_cs_limit) {
                PRINTF_D("violate the critical section time limit:%llu>%llu\n", max_cs_duration_time, max_cs_limit);
                configASSERT(0);
            }
        }

    }
    __real_vPortExitCritical();
}
uint64_t get_max_cs_duration_time(void)
{
    return max_cs_duration_time;
}
uint64_t get_max_cs_limit(void)
{
    return max_cs_limit;
}

