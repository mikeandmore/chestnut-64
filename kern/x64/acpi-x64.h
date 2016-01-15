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

#ifndef ACPI_X64_H
#define ACPI_X64_H

#include "libc/common.h"
#include "libc/vector.h"
#include "x64/cpu-x64.h"
#include "x64/ioapic-x64.h"

namespace kernel {

struct AcpiHeader;
struct AcpiHeaderMADT;
class LocalApicX64;

struct AcpiCPU {
  u32 cpu_id;
  u32 local_apic_id;
  bool enabled;
};

class AcpiX64 {
public:
  AcpiX64()
    : local_apic_(nullptr), local_apic_address_(nullptr), cpu_count_(0) {
    Init();
    kassert(local_apic_);
  }

  AcpiX64(const AcpiX64 &rhs) = delete;

  LocalApicX64* local_apic() const { return local_apic_; }
  void* local_apic_address() const { return local_apic_address_; }
  u32 cpus_count() const { return cpu_count_; }

  void InitIoApics();
  void BootSmp();

private:
  LocalApicX64* local_apic_;
  void* local_apic_address_;
  Vector<AcpiCPU> cpus_;
  Vector<IoApicX64*> io_apics_;
  u32 cpu_count_;

  void Init();
  bool ParseRSDP(void* p);
  void ParseRSDT(AcpiHeader* ptr);
  void ParseDT(AcpiHeader* ptr);
  void ParseTableAPIC(AcpiHeaderMADT* header);
};

}

#endif /* ACPI_X64_H */
