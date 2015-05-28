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
        // 15, 63, 64, 127, 255, 511, 1023, 2047, 4095
	void *ptr = kernel::kmalloc(15);
	kernel::kfree(ptr);

	ptr = kernel::kmalloc(63);
	kernel::kfree(ptr);
	ptr = kernel::kmalloc(64);
	kernel::kfree(ptr);
	ptr = kernel::kmalloc(127);
	kernel::kfree(ptr);

	ptr = kernel::kmalloc(255);
	kernel::kfree(ptr);

	ptr = kernel::kmalloc(511);
	kernel::kfree(ptr);

	ptr = kernel::kmalloc(1023);
	kernel::kfree(ptr);

	ptr = kernel::kmalloc(2047);
	kernel::kfree(ptr);

	ptr = kernel::kmalloc(4095);
	kernel::kfree(ptr);




}
