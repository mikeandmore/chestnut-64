#include "thread.h"

namespace kernel {

bool Thread::SaveContext()
{
	__asm__ (
		"movq %%rax, 0%0\n\t"
		"movq %%rbx, 8%0\n\t"

		"movq %%r12, 24%0\n\t"
		"movq %%r13, 32%0\n\t"
		"movq %%r14, 40%0\n\t"
		"movq %%r15, 48%0\n\t"
		"movq %%rdi, 56%0\n\t"

		"movq %%rsi, 64%0\n\t"
		"movq %%rdx, 72%0\n\t"
		"movq %%rcx, 80%0\n\t"
		"movq %%r8, 88%0\n\t"
		"movq %%r9, 96%0\n\t"

		"movq (%%rsp), %%rcx\n\t"
		"movq %%rcx, 16%0\n\t" // frame pointer is on stack

		"movq 8(%%rsp), %%rcx\n\t"
		"movq %%rcx, 104%0\n\t" // for RIP, exclude frame pointer

		"leaq 16(%%rsp), %%rcx\n\t" // skip the frame pointer and RA
		"movq %%rcx, 112%0\n\t" // for RSP
		: "=m" (context.reg)
		:
		: "memory", "%rcx"
		);
	return true;
}

void Thread::RestoreContext()
{
	__asm__ (
		"movq 0, %%rax\n\t" // set %RAX value to zero
		"movq 8%0, %%rbx\n\t"
		"movq 16%0, %%rbp\n\t"
		"movq 24%0, %%r12\n\t"
		"movq 32%0, %%r13\n\t"
		"movq 40%0, %%r14\n\t"
		"movq 48%0, %%r15\n\t"
		"movq 56%0, %%rdi\n\t"

		"movq 64%0, %%rsi\n\t"
		"movq 72%0, %%rdx\n\t"
		"movq 80%0, %%rcx\n\t"
		"movq 88%0, %%r8\n\t"
		"movq 96%0, %%r9\n\t"

		"movq 112%0, %%rsp\n\t" // for RSP

		"push 104%0\n\t" // push on stack as RA
		"retq" // jump back
		:
		: "m" (context.reg)
		: "%rax", "%rbx", "%rbp", "%r12", "%r13", "%r14", "%r15",
		  "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9", "%rsp",
		  "memory"
		);
	__builtin_unreachable();
}

Thread::Thread()
{
	// TODO: allocate stack page, initialize all variables
}

void Thread::Start()
{
	// TODO: add this thread to your scheduler, tell the scheduler to
	// schedule this thread later.
}

void Thread::Stub()
{
	// if the scheduler want this thread to run, but find there is no
	// context: meaning, the thread have just been added to the scheduler
	// and it hasn't run at all.
	//
	// Then, the scheduler should call this function

	void *stack_ptr = PADDR_TO_KPTR(context->stack->physical_address());
	__asm__ (
		"movq %0, %%rsp\n\t"
		:
		: "m" (stack_ptr)
		: "%rsp"
		);
	Run();

	// TODO: safely exit! Tell the scheduler to reclaim this thread when
	// the context is no longer be used.
}

}
