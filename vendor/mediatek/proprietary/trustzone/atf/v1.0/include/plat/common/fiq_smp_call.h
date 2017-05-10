#ifndef _INTER_CPU_CALL_H
#define _INTER_CPU_CALL_H

typedef void (*inter_cpu_call_func_t)(void *info);

struct call_function_data
{
    inter_cpu_call_func_t func;
    void *info;
    int lock;
};

int fiq_smp_call_function(unsigned int map, inter_cpu_call_func_t func, void *info, int wait);
void fiq_icc_isr(void);
int mt_fiq_smp_call_function(inter_cpu_call_func_t func, void *info, int wait);
#endif

