_Min_Stack_Size = 0x100;    /* stack size = 256 bytes */
_Region_Info_Addr = 0x400;
_Image_Addr = 0x800;

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */

  .isr_vector :
  {
    *(.isr_vector) /* Startup code */
  }

  /* The program code and other data goes into FLASH */
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
  }

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
  }

  /* used by the startup to initialize data */
  _sidata = .;

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : AT ( _sidata )
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
    . = ABSOLUTE(_Region_Info_Addr);
  }

  .region_info (NOLOAD) :
  {
    *(.region_info)
  }

  . = ALIGN(4);
  . = . + _Min_Stack_Size;
  _estack = .;
  . = ABSOLUTE(_Image_Addr);
  _sImage = .;
}
