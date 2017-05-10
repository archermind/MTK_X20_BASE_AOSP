#ifndef INTERRUPT_H
#define INTERRUPT_H
#include "FreeRTOS.h"
#include <platform.h>
#include <driver_api.h>
#include <mt_reg_base.h>
void mask_irq(IRQn_Type IRQn);
void unmask_irq(IRQn_Type IRQn);
void irq_status_dump(void);
void wakeup_source_count(uint32_t wakeup_src);

typedef void(*irq_handler_t)(void);
struct irq_desc_t {
    irq_handler_t handler;
    const char  *name;
    uint32_t priority;
    uint32_t irq_count;
    uint32_t wakeup_count;
    uint64_t last_enter;
    uint64_t last_exit;
    uint64_t max_duration;
};
struct irq_desc_t
    irq_desc[IRQ_MAX_CHANNEL];/*IRQ_MAX_CHANNEL is defined in ./kernel/CMSIS/Device/MTK/mt6797/Include/mt6797.h */
void request_irq(uint32_t irq, irq_handler_t handler, const char *name);
__STATIC_INLINE uint32_t is_in_isr(void)
{
    /*  Masks off all bits but the VECTACTIVE bits in the ICSR register. */
#define portVECTACTIVE_MASK                                     ( 0xFFUL )
    if ((portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK) != 0)
        return 1;/*in isr*/
    else
        return 0;
}

#define DEFAULT_IRQ_PRIORITY (0x8)


void set_max_cs_limit(uint64_t limit_time);
void disable_cs_limit(void);
uint64_t get_max_cs_duration_time(void);
uint64_t get_max_cs_limit(void);
#endif
