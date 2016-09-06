ENTRY(_boot)
OUTPUT_FORMAT(elf64-x86-64)

_virt_base = 0xFFFFFF0000000000;

SECTIONS {
   . = 0x100000;

   .boot : {
      *(.mbhdr)
      _load_start = .;
      *(.boot)
      . = ALIGN(4096);
      *(.smp_boot)
      . = ALIGN(4096);
      . += 0x1000;
      Pml4 = .;
      . += 0x1000;
      Pdpt = .;
      . += 0x1000;
      Pd = .;
      . += 0x1000;
      . += 0x8000;
      _boot_stack = .;
   }

   . += _virt_base;

   .text ALIGN(0x1000) : AT(ADDR(.text) - _virt_base) {
      *(.text)
      *(.gnu.linkonce.t*)
   }

   .data ALIGN(0x1000) : AT(ADDR(.data) - _virt_base) {
      *(.data)
      *(.gnu.linkonce.d*)
   }

   .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - _virt_base) {
      *(.rodata*)
      *(.gnu.linkonce.r*)
   }

   _load_end = . - _virt_base;

   .bss ALIGN(0x1000) : AT(ADDR(.bss) - _virt_base) {
      *(.bss)
      *(COMMON)
      *(.gnu.linkonce.b*)
   }

   _bss_end = . - _virt_base;

   /DISCARD/ : {
      *(.comment)
      *(.eh_frame)
   }
}
