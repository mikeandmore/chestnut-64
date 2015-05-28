#include "libc/common.h"
#include "libc/string.h"
#include "terminal.h"
#include "console.h"
#include "multiboot2.h"
#include "mm/allocator.h"
#include "mm/uslab.h"

__link void kernel_main(struct multiboot_tag_mmap *boot_mem_map)
{
	kernel::Terminal term;
	kernel::default_term = &term; // this stack will never destroy
	// kernel::default_term->Reset();
	// kernel::default_term->DrawString("Chestnut-64 OS booting...");
	kernel::Console console;
	kernel::console = &console;
	// console.putchar('a');
	// for (int i = 0; i < 100; i++)
	kernel::MemPages mem_pages;
	kernel::mem_pages = &mem_pages;

	console.printf("Chestnut-64 Booting...\n");

	kernel::mem_pages->Init(boot_mem_map);
	kernel::InitSlab();
        // 15, 63, 64, 127, 255, 511, 1023, 2047, 4095
	void *ptr = kernel::Alloc(15);
	kernel::Free(ptr);

	ptr = kernel::Alloc(63);
	kernel::Free(ptr);
	ptr = kernel::Alloc(64);
	kernel::Free(ptr);
	ptr = kernel::Alloc(127);
	kernel::Free(ptr);

	ptr = kernel::Alloc(255);
	kernel::Free(ptr);

	ptr = kernel::Alloc(511);
	kernel::Free(ptr);

	ptr = kernel::Alloc(1023);
	kernel::Free(ptr);

	ptr = kernel::Alloc(2047);
	kernel::Free(ptr);

	ptr = kernel::Alloc(4095);
	kernel::Free(ptr);
}
