/* Entry Point */
ENTRY(Reset_Handler)
_SCP_IPC_SHARE_BUFFER_ADDR = 0x7FE00;

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x0;      /* required amount of heap  */
_Min_Stack_Size = 0x800; /* required amount of stack for MSP*/

/* Specify the memory areas */
MEMORY
{
  LOADER (rwx)        : ORIGIN = 0x00000000, LENGTH =   2K /*loader for recovery use*/
  RTOS   (rwx)        : ORIGIN = 0x00000800, LENGTH = 510K
}
__RTOS_segment_start__ = ORIGIN( RTOS );
__RTOS_segment_end__ = __RTOS_segment_start__ + LENGTH( RTOS );

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into RTOS */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >RTOS

  /* The program code and other data goes into  RTOS*/
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
	*(.eh_frame)
	 __commands_start = .;
    KEEP (*(.commands))
    __commands_end = .;


    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
    _exit = .;
  } >RTOS


   .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >RTOS
    .ARM : {
    __exidx_start = .;
      *(.ARM.exidx*)
      __exidx_end = .;
    } >RTOS

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >RTOS
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >RTOS
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(.fini_array*))
    KEEP (*(SORT(.fini_array.*)))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >RTOS

  /* used by the startup to initialize data */
  _sidata = .;

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    __module_start = .;
    KEEP (*(.module_init*))
    __module_end = .;
    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RTOS

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RTOS

  /* kernel_heap_stack section, used to check that there is enough RAM left */
  ._kernel_heap_stack :
  {
    . = ALIGN(4);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
    _estack = .; /* Highest address of the MSP stack */
  } >RTOS

    . = ALIGN(4);
  .share (NOLOAD) :
  {
    _sshare = .;
    *(.share)
    . = ALIGN(4);
    _eshare = .;
  } >RTOS
  _s_ipc_share = _SCP_IPC_SHARE_BUFFER_ADDR;

  ASSERT(_eshare < _s_ipc_share, "The share buffer is over ipc_buffer!!")
}
