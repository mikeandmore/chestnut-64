#include "libc/common.h"
#include "libc/string.h"
#include "terminal.h"
#include "console.h"
#include "multiboot2.h"
#include "mm/allocator.h"
#include "mm/uslab.h"
#include "libc/vector.h"

#include "x64/io-x64.h"
#include "x64/cpu-x64.h"
#include "x64/acpi-x64.h"
#include "x64/page-table-x64.h"

__link void kernel_main(struct multiboot_tag_mmap *boot_mem_map)
{
  InitializeGlobal<kernel::SerialPortX64>();
  InitializeGlobal<kernel::Terminal, kernel::Console, kernel::MemPages>();

  kprintf("Chestnut-64 Booting...\n");
  GlobalInstance<kernel::MemPages>().Init(boot_mem_map);
  kernel::InitSlab();

  kernel::PageTableX64 pt;
  u64 t = KERN_OFFSET;
  kprintf("huge page paddr %x ->%x\n", t, pt[t][t][t].physical_address());

  // kprintf("Booting From Cpu %d...\n", kernel::CpuPlatform::id());
  // kernel::AcpiX64 acpi;
  // kprintf("ACPI Initialized\n");

}
