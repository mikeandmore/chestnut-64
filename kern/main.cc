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
	kernel::Allocator default_alloc;
	kernel::alloc = &default_alloc;

	console.printf("Chestnut-64 Booting...\n");

	kernel::alloc->Init(boot_mem_map);
	kernel::InitSlab();
}
