#ifndef __MT_REG_BASE_H__
#define __MT_REG_BASE_H__

#define CFGREG_BASE     0x400A0000
#define MDAIF_BASE      0x400A1000
#define INTC_BASE       0x400A2000
#define TIMER_BASE      0x400A3000
#define CLK_CTRL_BASE   0x400A4000
#define I2C0_BASE       0x400A5000
#define I2C1_BASE       0x400A6000
#define I2C2_BASE       0x400A7000
#define GPIO_BASE       0x400A8000
#define UART0_BASE      0x400A9000
#define EINT_BASE       0x400AA000
#define PMICWP2P_BASE   0x400AB000
#define SPMP2P_BASE     0x400AC000
#define DMA_BASE        0x400AD000
#define UART1_BASE      0x400AE000

/* CFGREG BASE */
#define AP_RESOURCE                (CFGREG_BASE + 0x04)
#define TCM_CFG                    (CFGREG_BASE + 0x10)
#define REMAP_CFG0                 (CFGREG_BASE + 0x14)
#define REMAP_CFG1                 (CFGREG_BASE + 0x18)
#define SCP_TO_HOST_INT            (CFGREG_BASE + 0x1C)
#define SCP_TO_SPM_INT             (CFGREG_BASE + 0x20)
#define GENERAL_REG0               (CFGREG_BASE + 0x50)
#define GENERAL_REG1               (CFGREG_BASE + 0x54)
#define GENERAL_REG2               (CFGREG_BASE + 0x58)
#define GENERAL_REG3               (CFGREG_BASE + 0x5C)
#define GENERAL_REG4               (CFGREG_BASE + 0x60)
#define GENERAL_REG5               (CFGREG_BASE + 0x64)
#define SEMAPHORE                  (CFGREG_BASE + 0x90)
#define REG_SP                     (CFGREG_BASE + 0xA8)
#define REG_LR                     (CFGREG_BASE + 0xAC)
#define REG_PSP                    (CFGREG_BASE + 0xB0)
#define REG_PC                     (CFGREG_BASE + 0xB4)

#define EXPECTED_FREQ_REG GENERAL_REG3
#define CURRENT_FREQ_REG  GENERAL_REG4

/* INTC BASE */
#define INTC_IRQ_STATUS_ADDR       (INTC_BASE + 0x00)
#define INTC_IRQ_ENABLE_ADDR       (INTC_BASE + 0x04)
#define INTC_IRQ_OUT_ADDR          (INTC_BASE + 0x08)
#define INTC_IRQ_SLEEP_ENABL_ADDR  (INTC_BASE + 0x0C)

#ifndef BUILD_SLT
#define MT_LOG_BUF_LEN  4096
#else
#define MT_LOG_BUF_LEN  512
#endif

enum SEMAPHORE_FLAG{
  SEMAPHORE_CLK_CFG_5 = 0,
  SEMAPHORE_PTP,
  SEMAPHORE_I2C0,
  SEMAPHORE_I2C1,
  SEMAPHORE_TOUCH,
  SEMAPHORE_APDMA,
  SEMAPHORE_SENSOR,
  NR_FLAG = 8,
};
#endif
