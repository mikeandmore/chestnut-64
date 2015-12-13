#include "libc/common.h"
#include "libc/string.h"
#include "terminal.h"
#include "console.h"
#include "multiboot2.h"
#include "mm/allocator.h"
#include "mm/uslab.h"

__link void kernel_main(struct multiboot_tag_mmap *boot_mem_map)
{
  InitializeGlobal<kernel::Terminal, kernel::Console, kernel::MemPages>();

  GlobalInstance<kernel::Console>().printf("Chestnut-64 Booting...\n");
  GlobalInstance<kernel::MemPages>().Init(boot_mem_map);
  kernel::InitSlab();
}
