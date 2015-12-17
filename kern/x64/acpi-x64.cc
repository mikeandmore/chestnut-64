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

#include "x64/acpi-x64.h"
#include "x64/local-apic-x64.h"
#include "x64/cpu-x64.h"
#include "libc/common.h"
#include "libc/string.h"
#include "mm/allocator.h"

namespace kernel {

struct AcpiHeader {
  u32 signature;
  u32 length;
  u8 revision;
  u8 checksum;
  u8 oem[6];
  u8 oemTableId[8];
  u32 oemRevision;
  u32 creatorId;
  u32 creatorRevision;
} __attribute__((packed));

constexpr static u32 TableUint32(const char* str) {
  return ((u32)str[0] <<  0) |
    ((u32)str[1] <<  8) |
    ((u32)str[2] << 16) |
    ((u32)str[3] << 24);
}

static_assert(TableUint32("ABCD") == 0x44434241,
              "Invalid table name convertion on current platform.");

struct AcpiHeaderMADT {
  AcpiHeader header;
  u32 localApicAddr;
  u32 flags;
} __attribute__((packed));

struct AcpiHPETAddress {
  u8 addressSpaceId;
  u8 registerBitWidth;
  u8 registerBitOffset;
  u8 reserved;
  u64 address;
} __attribute__((packed));

struct AcpiHeaderHPET {
  AcpiHeader header;
  u8 hardwareRevId;
  u8 comparatorCount : 5;
  u8 counterSize : 1;
  u8 reserved : 1;
  u8 legacyPlacement : 1;
  u16 pciVendorId;
  AcpiHPETAddress address;
  u8 hpetNumber;
  u16 minimumTick;
  u8 pageProtection;
} __attribute__((packed));

enum class ApicType : u8 {
  LOCAL_APIC = 0,
    IO_APIC = 1,
    INTERRUPT_OVERRIDE = 2
    };

struct ApicHeader {
  ApicType type;
  u8 length;
} __attribute__((packed));

struct ApicLocalApic {
  ApicHeader header;
  u8 acpiProcessorId;
  u8 apicId;
  u32 flags;
} __attribute__((packed));

struct ApicIoApic {
  ApicHeader header;
  u8 ioApicId;
  u8 reserved;
  u32 ioApicAddress;
  u32 globalSystemInterrupt;
} __attribute__((packed));

void AcpiX64::Init()
{
  for (u8* p = static_cast<u8*>(PADDR_TO_KPTR(0xe0000));
       p < static_cast<u8*>(PADDR_TO_KPTR(0x1000000)); p += 16) {
    u64 sig = *(u64*)p;
    if (0x2052545020445352 == sig) { // 'RSD PTR '
      if (ParseRSDP(p)) {
        break;
      }
    }
  }

  if (io_apics_.size() == 0) {
    panic("Unable to find IO APIC to setup interrupts.\n");
  }
}

bool AcpiX64::ParseRSDP(void* p)
{
  u8* pt = static_cast<u8*>(p);
  u8 sum = 0;
  for (u8 i = 0; i < 20; ++i) {
    sum += pt[i];
  }

  if (sum) {
    return false;
  }

  u8 rev = pt[15];
  u32 rsdt_addr = 0;

  switch (rev) {
  case 0:
    memcpy(&rsdt_addr, pt + 16, sizeof(u32));
    break;
  case 2:
    memcpy(&rsdt_addr, pt + 16, sizeof(u32));
    break;
  default:
    kprintf("ACPI unknown revision.\n");
    return false;
  }

  kassert(rsdt_addr);
  ParseRSDT(reinterpret_cast<AcpiHeader*>((u64)(rsdt_addr)));
  return true;
}

void AcpiX64::ParseRSDT(AcpiHeader* ptr)
{
  kassert(ptr);

  u32* p = reinterpret_cast<u32*>(ptr + 1);
  u32* end = reinterpret_cast<u32*>((u8*)ptr + ptr->length);

  while (p < end) {
    u64 addr = static_cast<u64>(*p++);
    ParseDT((AcpiHeader *)(uintptr_t)addr);
  }
}

void AcpiX64::ParseDT(AcpiHeader* ptr)
{
  kassert(ptr);
  u32 signature = ptr->signature;

  switch (signature) {
  case TableUint32("APIC"):
    ParseTableAPIC(reinterpret_cast<AcpiHeaderMADT*>(ptr));
    break;
  case TableUint32("HPET"):
    ParseTableHPET(reinterpret_cast<AcpiHeaderHPET*>(ptr));
    break;
  default:
    break;
  }
}

void AcpiX64::ParseTableHPET(AcpiHeaderHPET* header)
{
  kassert(header);
  if (nullptr != hpet_) {
    // Ignore other HPET devices except first one
    return;
  }

  u64 address = header->address.address;
  kassert(address);
  kassert(!hpet_);
  hpet_ = new HpetX64(reinterpret_cast<void*>(address));
  kassert(hpet_);
}

void AcpiX64::ParseTableAPIC(AcpiHeaderMADT* header)
{
  kassert(header);
  kassert(header->localApicAddr);

  void* local_apic_address = reinterpret_cast<void*>(
    static_cast<u64>(header->localApicAddr));

  local_apic_address_ = local_apic_address;
  local_apic_ = new LocalApicX64(local_apic_address);

  u8* p = (u8*)(header + 1);
  u8* end = (u8*)header + header->header.length;

  while (p < end) {
    ApicHeader* header = (ApicHeader*)p;
    ApicType type = header->type;
    u8 length = header->length;

    switch (type) {
    case ApicType::LOCAL_APIC: {
      ApicLocalApic* s = (ApicLocalApic*)p;
      AcpiCPU cpu;
      cpu.cpu_id = s->acpiProcessorId;
      cpu.local_apic_id = s->apicId;
      cpu.enabled = (1 == (s->flags & 1));
      cpus_.PushBack(move(cpu));
      ++cpu_count_;
    }
      break;
    case ApicType::IO_APIC: {
      ApicIoApic* s = (ApicIoApic*)p;
      io_apics_.PushBack(new IoApicX64(s->ioApicId,
                                       (uintptr_t)s->ioApicAddress, s->globalSystemInterrupt));
    }
      break;
    case ApicType::INTERRUPT_OVERRIDE:
      break;
    default:
      // TODO: parse others
      break;
    }

    p += length;
  }
}

void AcpiX64::StartCPUs()
{
  u8 startup_vec = 0x08;

  u32 bsp_apic_id = local_apic_->Id();
  u16 cpus_started = 0;
  for (const auto& cpu : cpus_) {
    if (cpu.local_apic_id == bsp_apic_id) {
      continue;
    }
    local_apic_->SendApicInit(cpu.local_apic_id);
  }

  for (const auto& cpu : cpus_) {
    if (cpu.local_apic_id == bsp_apic_id) {
      continue;
    }
    local_apic_->SendApicStartup(cpu.local_apic_id, startup_vec);
    ++cpus_started;

    kprintf("Starting #%d...\n", cpus_started);
    while (trampoline.cpus_counter_value() != cpus_started) {
      CpuPlatform::WaitPause();
    }
  }

  kprintf("Cpus: done.\n");
}

void AcpiX64::InitIoApics()
{
  for (IoApicX64* ioa : io_apics_) {
    ioa->Init();
  }
}

}
