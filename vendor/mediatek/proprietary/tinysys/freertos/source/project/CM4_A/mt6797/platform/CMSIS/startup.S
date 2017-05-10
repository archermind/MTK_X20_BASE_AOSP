  .file "startup.s"
  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.global  g_pfnVectors
.global  Default_Handler

.word  _sidata
.word  _sdata
.word  _edata
.word  _sbss
.word  _ebss

    .section  .text.Reset_Handler
  .weak  Reset_Handler
  .type  Reset_Handler, %function
Reset_Handler:

  ldr  r2, =_sbss
  b  LoopFillZerobss
/* Zero fill the bss segment. */
FillZerobss:
  movs  r3, #0
  str  r3, [r2], #4

LoopFillZerobss:
  ldr  r3, = _ebss
  cmp  r2, r3
  bcc  FillZerobss

  ldr  r2, =_sshare
  b  LoopFillZeroshare
/* Zero fill the bss segment. */
FillZeroshare:
  movs  r3, #0
  str  r3, [r2], #4

LoopFillZeroshare:
  ldr  r3, = _eshare
  cmp  r2, r3
  bcc  FillZeroshare

/* Call the clock system intitialization function.*/
  bl  SystemInit
/* Call the application's entry point.*/
  bl  main
  bx  lr
.size  Reset_Handler, .-Reset_Handler

    .section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
  .size  Default_Handler, .-Default_Handler
  .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object
  .size  g_pfnVectors, .-g_pfnVectors

.macro ISR_TEMPLATE isr_no
.word IRQHandler_\isr_no
.weak      IRQHandler_\isr_no
.thumb_set IRQHandler_\isr_no,Default_Handler
.endm

g_pfnVectors:
  .word  _estack
  .word  Reset_Handler
  .word  NMI_Handler
  .word  HardFault_Handler
  .word  MemManage_Handler
  .word  BusFault_Handler
  .word  UsageFault_Handler
  .word  0
  .word  0
  .word  0
  .word  0
  .word  SVC_Handler
  .word  DebugMon_Handler
  .word  0
  .word  PendSV_Handler
  .word  SysTick_Handler

  /* External Interrupts */
.word hw_isr_dispatch /*IPC0_IRQHandler */
.word hw_isr_dispatch /* IPC1_IRQHandler */
.word hw_isr_dispatch /* IPC2_IRQHandler */
.word hw_isr_dispatch /* IPC3_IRQHandler */
.word hw_isr_dispatch /* SPM_IRQHandler  */
.word hw_isr_dispatch /* CIRQ_IRQHandler */
.word hw_isr_dispatch /* EINT_IRQHandler */
.word hw_isr_dispatch /* PMIC_IRQHandler */
.word hw_isr_dispatch /* UART0_IRQHandler*/
.word hw_isr_dispatch /* UART1_IRQHandler*/
.word hw_isr_dispatch /* I2C0_IRQHandler */
.word hw_isr_dispatch /* I2C1_IRQHandler */
.word hw_isr_dispatch /* I2C2_IRQHandler */
.word hw_isr_dispatch /* CLK_CTRL_IRQHandler*/
.word hw_isr_dispatch /* MAD_FIFO_IRQHandler*/
.word hw_isr_dispatch /* TIMER0_IRQHandler  */
.word hw_isr_dispatch /* TIMER1_IRQHandler  */
.word hw_isr_dispatch /* TIMER2_IRQHandler  */
.word hw_isr_dispatch /* TIMER3_IRQHandler  */
.word hw_isr_dispatch /* TIMER4_IRQHandler  */
.word hw_isr_dispatch /* TIMER5_IRQHandler  */
.word hw_isr_dispatch /* UART0_RX_IRQHandler */
.word hw_isr_dispatch /* UART1_RX_IRQHandler */
.word hw_isr_dispatch /* DMA_IRQHandler */
.word hw_isr_dispatch /* AUDIO_IRQHandler */
.word hw_isr_dispatch /* MD_IRQHandler */

   .weak      NMI_Handler
   .thumb_set NMI_Handler,Default_Handler

   .weak      HardFault_Handler
   .thumb_set HardFault_Handler,Default_Handler

   .weak      MemManage_Handler
   .thumb_set MemManage_Handler,Default_Handler

   .weak      BusFault_Handler
   .thumb_set BusFault_Handler,Default_Handler

   .weak      UsageFault_Handler
   .thumb_set UsageFault_Handler,Default_Handler

   .weak      SVC_Handler
   .thumb_set SVC_Handler,Default_Handler

   .weak      DebugMon_Handler
   .thumb_set DebugMon_Handler,Default_Handler

   .weak      PendSV_Handler
   .thumb_set PendSV_Handler,Default_Handler

   .weak      SysTick_Handler
   .thumb_set SysTick_Handler,Default_Handler
