// -*- c++ -*

#ifndef KVM_CLOCK_H
#define KVM_CLOCK_H

#include "mm/allocator.h"
#include "libc/common.h"

namespace kernel {

struct KvmWallClockInfo {
  u32 version;
  u32 sec;
  u32 nsec;
} __attribute__((packed));

struct KvmCpuTimeInfo {
  u32 version;
  u32 pad0;
  u64 tsc_ts;
  u64 sys_time;
  u32 tsc_to_sys_mul;
  int8 tsc_shift;
  u8 flags;
  u16 pad1;
} __attribute__((packed));

class KvmWallClock {
public:
  KvmWallClock();
  KvmWallClock(const KvmWallClock &rhs) = delete;

  u64 BootClock();
private:
  KvmWallClockInfo info;
};

// suppose to be per-core
class KvmSystemTime {
public:
  KvmSystemTime();
  KvmSystemTime(const KvmSystemTime &rhs) = delete;

  u64 BootTime(bool use_hw_stable = true);
  u64 ProcessorNano(u64 time);
private:
  KvmCpuTimeInfo time_info;
  u64 last_boot_time;
};

}

#endif /* KVM_CLOCK_H */
