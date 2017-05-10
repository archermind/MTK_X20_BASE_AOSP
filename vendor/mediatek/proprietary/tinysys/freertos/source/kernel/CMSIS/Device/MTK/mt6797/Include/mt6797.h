#ifndef __MT6797_H
#define __MT6797_H
/**
  * @brief Configuration of the Cortex-M4 Processor and Core Peripherals
  */
#define __CM4_REV                 0x0001  /*!< Core revision r0p1                            */
#define __MPU_PRESENT             1       /*!< mt6797 provides an MPU                     */
#define __NVIC_PRIO_BITS          4       /*!< mt6797 uses 4 Bits for the Priority Levels */
#define __Vendor_SysTickConfig    0       /*!< Set to 1 if different SysTick Config is used  */
#define __FPU_PRESENT             1       /*!< FPU present                    */
#define ARM_MATH_CM4		  1       /*!< ARM_MATH_CM4 for building the library on Cortex-M4 target,
					    use in kernel/CMSIS/Include/arm_math.h*/

/**
 * @brief MT6797 Interrupt Number Definition, according to the selected device
 *        in @ref Library_configuration_section
 */
typedef enum
{
/******  Cortex-M4 Processor Exceptions Numbers ****************************************************************/
  NonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt                                          */
  MemoryManagement_IRQn       = -12,    /*!< 4 Cortex-M4 Memory Management Interrupt                           */
  BusFault_IRQn               = -11,    /*!< 5 Cortex-M4 Bus Fault Interrupt                                   */
  UsageFault_IRQn             = -10,    /*!< 6 Cortex-M4 Usage Fault Interrupt                                 */
  SVCall_IRQn                 = -5,     /*!< 11 Cortex-M4 SV Call Interrupt                                    */
  DebugMonitor_IRQn           = -4,     /*!< 12 Cortex-M4 Debug Monitor Interrupt                              */
  PendSV_IRQn                 = -2,     /*!< 14 Cortex-M4 Pend SV Interrupt                                    */
  SysTick_IRQn                = -1,     /*!< 15 Cortex-M4 System Tick Interrupt                                */
/******  MT6797 specific Interrupt Numbers **********************************************************************/
 IPC0_IRQn    = 0,
 IPC1_IRQn    = 1,
 IPC2_IRQn    = 2,
 IPC3_IRQn    = 3,
 SPM_IRQn    = 4,
 CIRQ_IRQn    = 5,
 EINT_IRQn    = 6,
 PMIC_IRQn    = 7,
 UART0_IRQn    = 8,
 UART1_IRQn    = 9,
 I2C0_IRQn    = 10,
 I2C1_IRQn    = 11,
 I2C2_IRQn    = 12,
 CLK_CTRL_IRQn    = 13,
 MAD_FIFO_IRQn    = 14,
 TIMER0_IRQn    = 15,
 TIMER1_IRQn    = 16,
 TIMER2_IRQn    = 17,
 TIMER3_IRQn    = 18,
 TIMER4_IRQn    = 19,
 TIMER5_IRQn    = 20,
 UART0_RX_IRQn    = 21,
 UART1_RX_IRQn    = 22,
 DMA_IRQn    = 23,
 AUDIO_IRQn    = 24,
 MD_IRQn     = 25,
 C2K_IRQn     = 26,
 IRQ_MAX_CHANNEL = 27
} IRQn_Type;


#include "core_cm4.h"             /*  Cortex-M4 processor and core peripherals */
#include <stdint.h>
#endif
