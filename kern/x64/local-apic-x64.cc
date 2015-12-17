// Copyright 2014 Runtime.JS project authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "x64/local-apic-x64.h"
#include "x64/cpu-x64.h"
#include "x64/io-x64.h"

namespace kernel {

LocalApicX64::LocalApicX64(void* local_apic_address)
    :	local_apic_address(local_apic_address),
        registers(local_apic_address),
        bus_freq(0) {
    kassert(local_apic_address);
}

void LocalApicX64::CpuSetAPICBase(uintptr_t apic) {
    u32 edx = (apic >> 32) & 0x0f;
    u32 eax = (apic & 0xfffff100) | kApicBaseMSREnable;

    CpuMSRValue value(eax, edx);
    CpuPlatform::SetMSR(kApicBaseMSR, value);
}

uintptr_t LocalApicX64::CpuGetAPICBase() {
    auto value = CpuPlatform::GetMSR(kApicBaseMSR);
    return (value.lo & 0xfffff100) | ((uintptr_t)(value.hi & 0x0f) << 32);
}

void LocalApicX64::InitCpu(HpetX64 *hpet) {
    kassert(hpet);

    // TODO: need to exclude address local_apic_address once we have a working
    // userspace

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

        u64 start = hpet->ReadMicroseconds();

        // Reset APIC timer (set counter to -1)
        registers.Write(LocalApicRegister::TIMER_INITIAL_COUNT, 0xffffffffU);

        // 1/100 of a second
        while (hpet->ReadMicroseconds() - start < 10000) {
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

}
