#include "libc/common.h"
#include "libc/string.h"
#include "terminal.h"
#include "console.h"
#include "multiboot2.h"
#include "mm/allocator.h"
#include "mm/uslab.h"
#include "libc/vector.h"

#include "x64/io-x64.h"

__link void kernel_main(struct multiboot_tag_mmap *boot_mem_map)
{
  InitializeGlobal<kernel::SerialPortX64>();
  InitializeGlobal<kernel::Terminal, kernel::Console, kernel::MemPages>();

  kprintf("Chestnut-64 Booting...\n");
  GlobalInstance<kernel::MemPages>().Init(boot_mem_map);
  kernel::InitSlab();
}
