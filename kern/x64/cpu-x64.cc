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

#include "x64/cpu-x64.h"
#include "x64/io-x64.h"
#include "libc/string.h"
#include "libc/common.h"
#include "mm/allocator.h"

extern u8 _smp_start;
extern u8 _smp_boot_code_end;

// other
extern volatile u16 _smp_cpu_counter;

namespace kernel {

void CpuPlatform::SetupSMPBootCode()
{
  kassert(reinterpret_cast<uintptr_t>(&_smp_boot_code_end)
          > reinterpret_cast<uintptr_t>(&_smp_start));
  const void *startup_loc = &_smp_start;
  u16 startup_len = reinterpret_cast<uintptr_t>(&_smp_boot_code_end) -
    reinterpret_cast<uintptr_t>(&_smp_start);
  kassert(startup_loc);
  memcpy(PADDR_TO_KPTR(kLoadAddress), startup_loc, startup_len);
  kprintf("SMP Boot Code Embeded\n");
}

u16 CpuPlatform::ReceiveCpuCounter()
{
  return _smp_cpu_counter;
}

}

// export global vars
static kernel::SerialPortX64 def_serial_port;

template <>
kernel::SerialPortX64 &GlobalInstance()
{
  return def_serial_port;
}
