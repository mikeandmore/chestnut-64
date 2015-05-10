#include "common.h"
#include "terminal.h"
#include "console.h"

__link void kernel_main(void)
{
	kernel::Terminal term;
	kernel::default_term = &term; // this stack will never destroy
	kernel::default_term->Reset();
	//kernel::default_term->DrawString("Chestnut-64 OS booting...");
	kernel::Console console;
	console.print('a');
}
