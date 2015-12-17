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

#ifndef HPET_X64_H
#define HPET_X64_H

#include "libc/common.h"

namespace kernel {

struct HPETRegisters {
  u32 general_capabilities;
  u32 counter_period;
  u64 reserved0;
  u64 general_configuration;
  u64 reserved1;
  u64 general_interrupt_status;
} __attribute__((packed));

struct HPETTimerConfig {
  u64 config;
  u64 comparator_value;
  u64 interrupt_route;
  u64 reserved0;
};

class HpetX64 {
public:
  /**
   * Create HPET instance using registers base address
   */
  HpetX64(void* address);

  /**
   * Read counter value converted to microseconds
   */
  u64 ReadMicroseconds() const {
    return ReadCounter() / us_div_;
  }

  /**
   * Read raw counter value
   */
  u64 ReadCounter() const {
    return *counter_;
  }

  /**
   * Reset counter (must be disabled)
   */
  void ResetCounter() {
    *counter_ = 0;
  }

  HPETTimerConfig* GetTimerConfig(u8 timer_index) {
    return reinterpret_cast<HPETTimerConfig*>(
      reinterpret_cast<u8*>(address_) + kTimerConfigsOffset
      + sizeof(HPETTimerConfig) * timer_index);
  }
private:
  static const int kCapabilitiesCounterSizeMask = (1 << 13);
  static const int kConfigurationEnable = (1 << 0);
  static const int kCounterOffset = 0x0f0;
  static const int kTimerConfigsOffset = 0x100;
  void* address_;
  HPETRegisters* registers_;
  volatile u64* counter_;
  u64 frequency_;
  u64 us_div_;
};

}


#endif /* HPET_X64_H */
