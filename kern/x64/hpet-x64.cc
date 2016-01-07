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

#include "x64/hpet-x64.h"
#include "x64/cpu-x64.h"

namespace kernel {

HpetX64::HpetX64(void* address)
  :  address_(address),
     registers_(reinterpret_cast<HPETRegisters*>(address)),
     counter_(reinterpret_cast<u64*>(
                reinterpret_cast<u8*>(address) + kCounterOffset)),
     frequency_(0),
     us_div_(0) {
  kprintf("HpetX64::HpetX64(%lx) %lx %lx\n", address, registers_, counter_);
  kassert(registers_);

  u32 period = registers_->counter_period; // in femptoseconds (10^-15 seconds)

  kassert(0 != period);
  kassert(period <= 0x05F5E100);

  if (!registers_->general_capabilities & kCapabilitiesCounterSizeMask) {
    kprintf("[HPET] 32 bit counter (!)\n");
  }

  frequency_ = /* 10^15 */ 1000000000000000 / period;

  kassert(frequency_ >= 1000000);
  us_div_ = frequency_ / 1000000;
  kassert(us_div_ > 0);

  ResetCounter();
  CpuPlatform::WaitPause();
  registers_->general_configuration |= kConfigurationEnable;
  CpuPlatform::WaitPause();
}

}
