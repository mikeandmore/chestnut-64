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

#include "x64/ioapic-x64.h"
#include "x64/page-table-x64.h"
#include "libc/common.h"

namespace kernel {

IoApicX64::IoApicX64(u32 id, uintptr_t address, u32 intr_base)
  : id_(id),
    addr(address),
    interrupt_base(intr_base),
    registers(IoApicRegistersAccessor(addr))
{
  kprintf("IoApic addr 0x%lx\n", addr);
}

void IoApicX64::Init()
{
  GetKernelPageTable().MapPage(KPTR_TO_PADDR(addr), false, true);
  kassert(addr);
  u32 id_reg = (registers.Read(IoApicRegister::ID) >> 24) & 0xF;

  if (id_reg != id_) {
    registers.Write(IoApicRegister::ID, 0, id_ << 24);
    kprintf("Patch APICID = %d, was = %d\n", id_, id_reg);
  }

  u32 max_value = (registers.Read(IoApicRegister::VER) >> 16) & 0xFF;
  kassert(max_value);

  u32 max_interrupts = max_value + 1;

  const u32 kIntMasked = 1 << 16;
  const u32 kIntTrigger = 1 << 15;
  const u32 kIntActiveLow = 1 << 14;
  const u32 kIntDstLogical = 1 << 11;
  const u32 kIRQOffset = 32;

  // Enable all interrupts
  for (u32 i = 0; i < max_interrupts; ++i) {
    // Mask timer (timer doesn't go through IOAPIC)
    if (0 == interrupt_base && (0 == i)) {
      registers.SetEntry(i, kIntMasked | (kIRQOffset + i));
      continue;
    }

    EnableIrq(kIRQOffset, i);
  }
}

}
