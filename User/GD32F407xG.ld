/*!
    \file    GD32F407xG.ld
    \brief   GNU linker script for GD32F407ZG (1MB Flash, 112KB SRAM0 + 16KB SRAM1 + 64KB CCM)

    Memory map of GD32F407ZG:
        FLASH   : 0x0800 0000, 1024KB
        SRAM0   : 0x2000 0000, 112KB  (main RAM: .data / .bss / heap / stack)
        SRAM1   : 0x2001 C000,  16KB  (可单独放变量，如 DMA 缓冲)
        CCM RAM : 0x1000 0000,  64KB  (仅 CPU 可访问，速度快)

    命名对照：GD32 的 SRAM1 = STM32 的 SRAM2；TCMSRAM = CCM RAM。

    使用方式（把变量放进指定内存）：
        __attribute__((section(".sram1")))   放到 SRAM1
        __attribute__((section(".ccmram")))  放到 CCM RAM

    Entry point is Reset_Handler, see startup_gd32f407_427.S
*/

/* entry point */
ENTRY(Reset_Handler)

/* highest address of the user mode stack (stack grows downwards) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size  = 0x200;    /* required amount of heap  */
_Min_Stack_Size = 0x400;    /* required amount of stack */

/* specify the memory areas */
MEMORY
{
  RAM    (xrw) : ORIGIN = 0x20000000, LENGTH = 112K
  SRAM1  (xrw) : ORIGIN = 0x2001C000, LENGTH = 16K
  CCMRAM (rw)  : ORIGIN = 0x10000000, LENGTH = 64K
  FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
}

/* define output sections */
SECTIONS
{
  /* the startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* the program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab : 
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  .ARM : 
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  /* uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* user_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* SRAM1 section (16KB)，用 __attribute__((section(".sram1"))) 把变量放这里 */
  ._sram1 (NOLOAD) :
  {
    . = ALIGN(4);
    _ssram1 = .;
    *(.sram1)
    *(.sram1*)
    . = ALIGN(4);
    _esram1 = .;
  } >SRAM1

  /* CCM RAM section (64KB)，用 __attribute__((section(".ccmram"))) 把变量放这里 */
  ._ccmram (NOLOAD) :
  {
    . = ALIGN(4);
    _sccmram = .;
    *(.ccmram)
    *(.ccmram*)
    . = ALIGN(4);
    _eccmram = .;
  } >CCMRAM

  .ARM.attributes 0 : { *(.ARM.attributes) }

}