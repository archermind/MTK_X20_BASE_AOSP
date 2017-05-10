#ifndef PLAT_HELPERS_H
#define PLAT_HELPERS_H

unsigned long read_l2actlr(void);
unsigned long read_l2ectlr(void);

void write_l2actlr(unsigned long);
void write_l2ectlr(unsigned long);

unsigned long read_cpuactlr(void);
void write_cpuactlr(unsigned long);

unsigned long read_cpuectlr(void);
void write_cpuectlr(unsigned long);


#endif
