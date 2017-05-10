#ifndef DRIVER_API_H
#define DRIVER_API_H


/// I/O    ////////////////////////////////////////////////////////////////////
#define OUTREG32(a,b)   (*(volatile unsigned int *)(a) = (unsigned int)(b))
#define INREG32(a)      (*(volatile unsigned int *)(a))

#ifndef DRV_WriteReg32
#define DRV_WriteReg32(addr,data)   ((*(volatile unsigned int *)(addr)) = (unsigned int)(data))
#endif
#ifndef DRV_Reg32
#define DRV_Reg32(addr)             (*(volatile unsigned int *)(addr))
#endif
#define DRV_WriteReg8(addr,data)    ((*(volatile char *)(addr)) = (char)(data))
#define DRV_Reg8(addr)              (*(volatile char *)(addr))
#define DRV_SetReg32(addr, data)    ((*(volatile unsigned int *)(addr)) |= (unsigned int)(data))
#define DRV_ClrReg32(addr, data)    ((*(volatile unsigned int *)(addr)) &= ~((unsigned int)(data)))
#define DRV_SetReg8(addr, data)    ((*(volatile char *)(addr)) |= (char)(data))
#define DRV_ClrReg8(addr, data)    ((*(volatile char *)(addr)) &= ~((char)(data)))

// lowcase version
#define outreg32(a,b)   (*(volatile unsigned int *)(a) = (unsigned int)(b))
#define inreg32(a)      (*(volatile unsigned int *)(a))

#ifndef drv_write_reg32
#define drv_write_reg32(addr,data)   ((*(volatile unsigned int *)(addr)) = (unsigned int)(data))
#endif
#ifndef drv_reg32
#define drv_reg32(addr)             (*(volatile unsigned int *)(addr))
#endif
#define drv_write_reg8(addr,data)    ((*(volatile char *)(addr)) = (char)(data))
#define drv_reg8(addr)              (*(volatile char *)(addr))
#define drv_set_reg32(addr, data)    ((*(volatile unsigned int *)(addr)) |= (unsigned int)(data))
#define drv_clr_reg32(addr, data)    ((*(volatile unsigned int *)(addr)) &= ~((unsigned int)(data)))
#define drv_set_reg8(addr, data)    ((*(volatile char *)(addr)) |= (char)(data))
#define drv_clr_reg8(addr, data)    ((*(volatile char *)(addr)) &= ~((char)(data)))

#ifndef drv_write_reg16
#define drv_write_reg16(addr,data)    ((*(volatile unsigned short *)(addr)) = (unsigned short)(data))
#endif

#ifndef drv_reg16
#define drv_reg16(addr)              (*(volatile unsigned short *)(addr))
#endif

#endif
