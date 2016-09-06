#include "apic.h"
#include "page-table.h"
#include "cpu.h"
#include "io-ports.h"
#include "irqs.h"

namespace kernel {

LocalApic::LocalApic(void* local_apic_addr)
  : local_apic_address(local_apic_addr),
    registers(local_apic_address),
    bus_freq(0)
{
  kprintf("-> LocalApic addr 0x%lx\n", local_apic_address);
}

void LocalApic::CpuSetAPICBase(uintptr_t apic)
{
  u32 edx = (apic >> 32) & 0x0f;
  u32 eax = (apic & 0xfffff100) | kApicBaseMSREnable;

  CpuMSRValue value(eax, edx);
  CpuPlatform::SetMSR(kApicBaseMSR, value);
}

uintptr_t LocalApic::CpuGetAPICBase() {
  auto value = CpuPlatform::GetMSR(kApicBaseMSR);
  return (value.lo & 0xfffff100) | ((uintptr_t)(value.hi & 0x0f) << 32);
}

void LocalApic::InitCpu(KvmSystemTime *systime)
{
  GetKernelPageTable().MapPage(KPTR_TO_PADDR(local_apic_address), false, true);
  CpuSetAPICBase(CpuGetAPICBase());

  // Clear task priority to enable all interrupts
  registers.Write(LocalApicRegister::TASK_PRIORITY, 0);
  // Set masks
  registers.Write(LocalApicRegister::LINT0, 1 << 16);
  registers.Write(LocalApicRegister::LINT1, 1 << 16);
  registers.Write(LocalApicRegister::TIMER, 1 << 16);

  // Perf
  registers.Write(LocalApicRegister::PERF, 4 << 8);

  // Flat mode
  registers.Write(LocalApicRegister::DESTINATION_FORMAT, 0xffffffff);

  // Logical Destination Mode, all cpus use logical id 1
  registers.Write(LocalApicRegister::LOGICAL_DESTINATION, 1);

  // Configure Spurious Interrupt Vector Register
  registers.Write(LocalApicRegister::SPURIOUS_INTERRUPT_VECTOR, 0x100 | 0xff);

  u8 timer_vector = 32;

  if (0 == bus_freq) {
    // Ensure we do this once on CPU 0
    kassert(0 == CpuPlatform::id());

    // Calibrate times
    registers.Write(LocalApicRegister::TIMER, timer_vector);
    registers.Write(LocalApicRegister::TIMER_DIVIDE_CONFIG, 0x03);

    u64 start = systime->BootTime();

    // Reset APIC timer (set counter to -1)
    registers.Write(LocalApicRegister::TIMER_INITIAL_COUNT, 0xffffffffU);

    // 1/100 of a second
    while (systime->BootTime() - start < 10000 * 1000) {
      CpuPlatform::WaitPause();
    }

    // Stop Apic timer
    registers.Write(LocalApicRegister::TIMER, 1 << 16);

    // Calculate bus frequency
    u32 curr_count = registers.Read(LocalApicRegister::TIMER_CURRENT_COUNT);
    u32 cpubusfreq = ((0xFFFFFFFF - curr_count) + 1) * 16 * 100;
    bus_freq = cpubusfreq;
  }

  kassert(bus_freq);
  u32 quantum = 32;
  u32 init_count = bus_freq / quantum / 16;

  // Set minimum initial count value to avoid QEMU
  // "I/O thread has spun for 1000 iterations" warning
  if (init_count < 0x1000) {
    init_count = 0x1000;
  }

  // Set periodic mode for APIC timer
  registers.Write(LocalApicRegister::TIMER_INITIAL_COUNT, init_count);
  registers.Write(LocalApicRegister::TIMER, timer_vector | 0x20000);
  registers.Write(LocalApicRegister::TIMER_DIVIDE_CONFIG, 0x03);
  registers.Read(LocalApicRegister::SPURIOUS_INTERRUPT_VECTOR);

  EOI();
}

IoApic::IoApic(u32 id, uintptr_t address, u32 intr_base)
  : id_(id),
    addr(address),
    interrupt_base(intr_base),
    registers(IoApicRegistersAccessor(addr))
{
  kprintf("-> IoApic addr 0x%lx interrupt_base 0x%lx\n", addr, interrupt_base);
}

void IoApic::Init()
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

  kassert(max_interrupts < IrqVector::kMaxInterrupt);

  const u32 kIntMasked = 1 << 16;
  const u32 kIntTrigger = 1 << 15;
  const u32 kIntActiveLow = 1 << 14;
  const u32 kIntDstLogical = 1 << 11;
  const u32 kIRQOffset = 32;

  // Disable NMI
  IoPorts::OutB(0x70, IoPorts::InB(0x70) | 0x80);

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
