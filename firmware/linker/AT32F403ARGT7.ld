ENTRY(entry)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 768K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        _end = .;
        end = .;
    } > RAM

    .rti_fn :
    {
        . = ALIGN(4);
        PROVIDE(__rt_init_rti_start = .);
        KEEP(*(SORT(.rti_fn.0)))
        PROVIDE(__rt_init_rti_board_start = .);
        KEEP(*(SORT(.rti_fn.0.end)))
        KEEP(*(SORT(.rti_fn.1)))
        KEEP(*(SORT(.rti_fn.1.*)))
        PROVIDE(__rt_init_rti_board_end = .);
        KEEP(*(SORT(.rti_fn.1.end)))
        KEEP(*(SORT(.rti_fn.2)))
        KEEP(*(SORT(.rti_fn.3)))
        KEEP(*(SORT(.rti_fn.4)))
        KEEP(*(SORT(.rti_fn.5)))
        KEEP(*(SORT(.rti_fn.6)))
        KEEP(*(SORT(.rti_fn.6.*)))
        PROVIDE(__rt_init_rti_end = .);
        KEEP(*(SORT(.rti_fn.6.end)))
        KEEP(*(SORT(.rti_fn.7)))
        . = ALIGN(4);
    } > FLASH

    FSymTab :
    {
        . = ALIGN(4);
        PROVIDE(__fsymtab_start = .);
        KEEP(*(FSymTab))
        PROVIDE(__fsymtab_end = .);
        . = ALIGN(4);
    } > FLASH

    /DISCARD/ :
    {
        *(.note*)
        *(.comment*)
    }
}

PROVIDE(_Min_Heap_Size = 0x400);
PROVIDE(_Min_Stack_Size = 0x400);
