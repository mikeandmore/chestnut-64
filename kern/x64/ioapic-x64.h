// Copyright 2014 Runtime.JS project authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef IOAPIC_X64_H
#define IOAPIC_X64_H

#include "libc/common.h"

namespace kernel {

enum class IoApicAccessorRegister {
  IOREGSEL    = 0x00,  // Register selector
  IOWIN       = 0x10   // Register data
};

enum class IoApicRegister {
  ID          = 0x00,  // IO APIC ID
  VER         = 0x01,  // IO APIC Version / Max Redirection Entry
  ARB         = 0x02,  // APIC Arbitration ID
  REDTBL      = 0x10   // Redirection Entries
};

class IoApicRegistersAccessor {
public:
  explicit IoApicRegistersAccessor(uintptr_t address)
    :	addr(address) {
    kassert(addr);
  }

  inline u32 Read(IoApicRegister reg) const {
    kassert(addr);
    *(volatile u32*)(addr + static_cast<u8>(IoApicAccessorRegister::IOREGSEL))
      = static_cast<u32>(reg);

    return *(volatile u32*)(addr + static_cast<u8>(IoApicAccessorRegister::IOWIN));
  }

  inline void Write(IoApicRegister reg, u8 reg_offset, u8 value) const {
    kassert(addr);
    *(volatile u32*)(addr + static_cast<u8>(IoApicAccessorRegister::IOREGSEL))
      = static_cast<u32>(reg) + reg_offset;

    *(volatile u32*)(addr + static_cast<u8>(IoApicAccessorRegister::IOWIN)) = value;
  }

  inline void SetEntry(u8 index, u64 data) const {
    Write(IoApicRegister::REDTBL, index * 2, static_cast<u32>(data));
    Write(IoApicRegister::REDTBL, index * 2 + 1, static_cast<u32>(data >> 32));
  }
private:
  uintptr_t addr;
};

class IoApicX64 {
public:
  IoApicX64(u32 id, uintptr_t address, u32 intr_base);

  void Init();

  void EnableIrq(u32 first_irq_offset, u32 irq) {
    kprintf("Enabling IRQ %d %d\n", first_irq_offset, irq);
    registers.SetEntry(irq, first_irq_offset + irq + interrupt_base);
  }

private:
  u32 id_;
  uintptr_t addr;
  u32 interrupt_base;
  IoApicRegistersAccessor registers;
};

}


#endif /* IOAPIC_X64_H */
