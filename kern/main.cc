#include "libc/common.h"
#include "terminal.h"
#include "console.h"
#include "multiboot2.h"

__link void kernel_main(struct multiboot_mmap_entry *mem_map)
{
	kernel::Terminal term;
	kernel::default_term = &term; // this stack will never destroy
	kernel::default_term->Reset();
	//kernel::default_term->DrawString("Chestnut-64 OS booting...");
	kernel::Console console;
	console.print('a');
}
