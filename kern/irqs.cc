#include "irqs.h"
#include "acpi.h"
#include "mm/allocator.h"
#include "io-ports.h"

namespace kernel {

struct IDT {
  u16 len;
  u64 addr;
};

}

extern kernel::IDT IDT64;

namespace kernel {

static bool gIsInterruptException[32];
static int gExceptionIrq[] = {
  0x08, 0x10, 0x11, 0x12, 0x13, 0x14, 0x17, 0x30
};

// We should *never* compile with -fomit-frame-pointer!
template <int IrqNumber>
void InterruptWrapper()
{
  // save context to the stack
  u64 frame_ptr;
  u64 stack_ptr;
  asm volatile(
    "sub $0x200, %%rsp;"
    "fnstenv (%%rsp);"
    "stmxcsr 0x18(%%rsp);"

    "sub $0x08, %%rsp;" // RIP

    "push %%r15;"
    "push %%r14;"
    "push %%r13;"
    "push %%r12;"
    "push %%r11;"
    "push %%r10;"
    "push %%r9;"
    "push %%r8;"

    "sub $0x10, %%rsp;" // RBP and RSP
    "push %%rsi;"
    "push %%rdi;"
    "push %%rdx;"
    "push %%rcx;"
    "push %%rbx;"
    "push %%rax;"

    "movq %%rbp, %0;"
    "movq %%rsp, %1;":: "m" (frame_ptr), "m" (stack_ptr));

  InterruptFrame * f = nullptr;

  if (gIsInterruptException[IrqNumber]) {
    f = (InterruptFrame *) (frame_ptr + 8);
  } else {
    ExceptionFrame *ef = (ExceptionFrame *) (frame_ptr + 8);
    f = ef;
  }
  Context *ctx = (Context *) stack_ptr;
  kprintf("frame 0x%lx, $rip 0x%lx, $cs 0x%lx %eflags 0x%lx, $rsp 0x%lx\n", frame_ptr, f->rip,
          f->cs, f->eflags, f->rsp);
  kprintf("ctx_ptr 0x%lx\n", ctx);
  ctx->rbp = *(u64 *) frame_ptr;
  ctx->rsp = f->rsp;
  ctx->rip = f->rip;

  Global<kernel::IrqVector>().InvokeChainHandlers(IrqNumber, f, ctx);
  Global<kernel::Acpi>().local_apic()->EOI();

  // restore context from the stack, we're leaving
  asm volatile(
    "mov %0, %%rsp;"

    "pop %%rax;"
    "pop %%rbx;"
    "pop %%rcx;"
    "pop %%rdx;"
    "pop %%rdi;"
    "pop %%rsi;"
    "add $0x10, %%rsp;" // RSP and RBP

    "pop %%r8;"
    "pop %%r9;"
    "pop %%r10;"
    "pop %%r11;"
    "pop %%r12;"
    "pop %%r13;"
    "pop %%r14;"
    "pop %%r15;"

    "add $0x08, %%rsp;" // RIP

    "fldenv (%%rsp);"
    "ldmxcsr 0x18(%%rsp);"
    "add $0x200, %%rsp;"
    : "=m" (stack_ptr));

  asm volatile("leave; iretq");
}

template <int N>
struct HandlerInstaller : public HandlerInstaller<N - 1> {
  HandlerInstaller(IrqVector *vec) : HandlerInstaller<N - 1>(vec) {
    vec->InstallIrqFunction(N, &InterruptWrapper<N>);
  }
};

template <>
struct HandlerInstaller<0> {
  HandlerInstaller(IrqVector *vec) {
    vec->InstallIrqFunction(0, &InterruptWrapper<0>);
  }
};

void IrqVector::Setup(u64 base_addr)
{
  base_irq_addr = base_addr;

  IoPorts::OutB(0x20, 0x11); // 00010001b, begin PIC 1 initialization
  IoPorts::OutB(0xA0, 0x11); // 00010001b, begin PIC 2 initialization

  IoPorts::OutB(0x21, 0x20); // IRQ 0-7, interrupts 20h-27h
  IoPorts::OutB(0xA1, 0x28); // IRQ 8-15, interrupts 28h-2Fh

  IoPorts::OutB(0x21, 0x04);
  IoPorts::OutB(0xA1, 0x02);

  IoPorts::OutB(0x21, 0x01);
  IoPorts::OutB(0xA1, 0x01);

  // Mask all PIC interrupts
  IoPorts::OutB(0x21, 0xFF);
  IoPorts::OutB(0xA1, 0xFF);

  memset(handlers, 0, sizeof(IrqHandler *));

  for (int i = 0; i < 8; i++) {
    gIsInterruptException[gExceptionIrq[i]] = true;
  }

  HandlerInstaller<0xFF>(this);

  void *idt_ptr = PADDR_TO_KPTR(&IDT64);

  // u64 idt_ptr = (u64) &gIDT;
  asm volatile("mov %%rax, %0; lidt (%%rax)" ::"r" (idt_ptr) : "%rax");
}

void IrqVector::InstallIrqFunction(u8 irq_id, void (*func)(void), u8 type)
{
  u8 *idt_table = (u8 *) base_irq_addr;
  u8 b[16];

  b[0] = (u64) func & 0x00000000000000FF;
  b[1] = ((u64) func & 0x000000000000FF00) >> 8;
  b[2] = 0x08; // CS
  b[3] = 0;
  b[4] = 0;
  b[5] = type;
  b[6] = ((u64) func & 0x0000000000FF0000) >> 16;
  b[7] = ((u64) func & 0x00000000FF000000) >> 24;

  b[8] = ((u64) func & 0x000000FF00000000) >> 32;
  b[9] = ((u64) func & 0x0000FF0000000000) >> 40;
  b[10] = ((u64) func & 0x00FF000000000000) >> 48;
  b[11] = ((u64) func & 0xFF00000000000000) >> 56;
  b[12] = 0;
  b[13] = 0;
  b[14] = 0;
  b[15] = 0;

  for (u64 i = 0; i < 16; i++) {
    *(u8 *) PADDR_TO_KPTR(idt_table + irq_id * 16 + i) = b[i];
  }
}

void IrqVector::InvokeChainHandlers(int irq_number, InterruptFrame *f, Context *saved_ctx)
{
  auto p = handlers[irq_number];
  while (p) {
    p->Run(f, saved_ctx);
    p = p->next;
  }
}

}

static kernel::IrqVector vec;

template <>
kernel::IrqVector &Global()
{
  return vec;
}
