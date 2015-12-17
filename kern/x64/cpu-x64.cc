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

// located at 64 bit code location
extern "C" u8 _ap_startup_location;

// located at 16 bit start and finish of code
extern "C" u8 _ap_startup_start;
extern "C" u8 _ap_startup_finish;

// other
extern "C" volatile u16 _cpus_counter;


namespace kernel {

void CpuPlatform::SetupSMPBootCode()
{
  kassert(reinterpret_cast<uintptr_t>(&_ap_startup_finish)
          > reinterpret_cast<uintptr_t>(&_ap_startup_start));
  const void *startup_loc = &_ap_startup_location;
  u16 startup_len = reinterpret_cast<uintptr_t>(&_ap_startup_start) -
    reinterpret_cast<uintptr_t>(&_ap_startup_finish);
  kassert(startup_loc);
  memcpy((void *)kLoadAddress, startup_loc, startup_len);
  kprintf("SMP Boot Code Embeded\n");
}

u16 CpuPlatform::ReceiveCpuCounter()
{
  return _cpus_counter;
}

}

// export global vars
static kernel::SerialPortX64 def_serial_port;

template <>
kernel::SerialPortX64 &GlobalInstance()
{
  return def_serial_port;
}
