#ifndef IRQS_H
#define IRQS_H

#include "libc/common.h"
#include "thread.h"

namespace kernel {

struct InterruptFrame {
  u64 rip;
  u64 cs;
  u64 eflags;
  u64 rsp;
};

struct ExceptionFrame : public InterruptFrame {
  u64 error_code;
};

class IrqVector;

class IrqHandler {
public:
  virtual void Run(InterruptFrame *f, Context *saved_context);
private:
  IrqHandler *next;
friend IrqVector;
};

class IrqVector {
  u64 base_irq_addr;
public:
  void Setup(u64 base_addr);
  static const int kMaxInterrupt = 256;

  void InstallIrqFunction(u8 irq_id, void (*func)(void), u8 type = 0x8E);

  void InvokeChainHandlers(int irq_number, InterruptFrame *f, Context *saved_context);
  void InstallChainHandler(int irq_number, IrqHandler *p) {
    p->next = handlers[irq_number];
    handlers[irq_number] = p;
  }
private:
  IrqHandler *handlers[kMaxInterrupt];
};

}

#endif /* IRQS_H */
