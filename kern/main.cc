#include "libc/common.h"
#include "libc/string.h"
#include "page-table.h"
#include "terminal.h"
#include "console.h"
#include "multiboot2.h"
#include "kvm-clock.h"
#include "cpu.h"
#include "acpi.h"

#include "mm/allocator.h"
#include "mm/uslab.h"
#include "libc/vector.h"

struct MultibootInfo {
  struct multiboot_tag_mmap *boot_mem_map;
  struct multiboot_tag_vbe *boot_vbe;
  struct multiboot_tag_framebuffer *boot_framebuf;
};

MultibootInfo ParseMultibootInfo(u8 *mbi)
{
  MultibootInfo info;
  memset(&info, 0, sizeof(MultibootInfo));

  int info_size = *(int *) mbi;
  u8 *tag_ptr = mbi + 8;

  while (tag_ptr < mbi + info_size) {
    auto tag = (struct multiboot_tag *) tag_ptr;
    // kprintf("Found MB tag type %d %d\n", tag->type, tag->size);
    if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
      info.boot_mem_map = (struct multiboot_tag_mmap *) tag;
    } else if (tag->type == MULTIBOOT_TAG_TYPE_VBE) {
      info.boot_vbe = (struct multiboot_tag_vbe *) tag;
    } else if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
      info.boot_framebuf = (struct multiboot_tag_framebuffer *) tag;
#if 0
      kprintf("addr 0x%x pitch %d %dx%d\n",
              info.boot_framebuf->common.framebuffer_addr,
              info.boot_framebuf->common.framebuffer_pitch,
              info.boot_framebuf->common.framebuffer_width,
              info.boot_framebuf->common.framebuffer_height);
#endif
    }
    tag_ptr += (tag->size + 0x07) & (~0x07);
  }
  return info;
}

__link void KernelMain(u64 mbi_paddr)
{
  u8 *mbi = (u8 *) PADDR_TO_KPTR(mbi_paddr);
  InitializeGlobal<kernel::Terminal, kernel::Console, kernel::MemPages>();

  kernel::SerialPort port;
  Global<kernel::Console>().AddSource(&port);

  kernel::InitBootPageTable();

  auto info = ParseMultibootInfo(mbi);
  kprintf("Chestnut-64 Booting...\n");

  Global<kernel::MemPages>().Init(info.boot_mem_map);

  kernel::InitSlab();
  kernel::InitKernelPageTable(info.boot_mem_map);


  kprintf("Booting From Cpu %d...\n", kernel::CpuPlatform::id());
  kernel::Acpi acpi;
  kprintf("ACPI Initialized\n");
#if 0
  acpi.local_apic()->InitCpu(&systime);
  acpi.InitIoApics();
#endif

  kprintf("\n\nAll Done. Quitting Now\n");
  // kernel::CpuPlatform::HangSystem();
  // kernel::CpuPlatform::WaitPause();
  asm volatile ("int3");
}
