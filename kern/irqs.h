#ifndef IRQS_H
#define IRQS_H

#include "libc/common.h"

namespace kernel {

class IrqHandler {
public:
  virtual void Run();
};

class IrqVector {
  u64 base_irq_addr;
public:
  void Setup(u64 base_addr);
  static const int kMaxInterrupt = 256;
private:
  void InstallHandler(u8 irq_id, void (*func)(void), u8 type = 0x8E);

  IrqHandler *handlers[kMaxInterrupt];
};

}

#endif /* IRQS_H */
