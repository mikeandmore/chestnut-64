#include "x64/kvm-clock.h"
#include "x64/cpu-x64.h"
#include "libc/common.h"
#include "libc/string.h"
#include "mm/allocator.h"

namespace kernel {

// this is the newer version of kvm clock source
static const u32 kKvmClockMSR = 0x4b564d00;
static const u32 kKvmSystemTimeMSR = 0x4b564d01;

KvmWallClock::KvmWallClock()
{
  memset(&info, 0, sizeof(KvmWallClockInfo));
  CpuPlatform::SetMSR(kKvmClockMSR, CpuMSRValue(KPTR_TO_PADDR(&info)));
}

KvmSystemTime::KvmSystemTime()
  : last_boot_time(0)
{
  memset(&time_info, 0, sizeof(KvmSystemTime));
  CpuPlatform::SetMSR(kKvmSystemTimeMSR, CpuMSRValue(KPTR_TO_PADDR(&time_info) | 0x01));
}

u64 KvmWallClock::BootClock()
{
  u32 v1, v2;
  u64 res;
  do {
    v1 = info.version;
    soft_barrier();
    res = info.sec;
    res *= 1000000000;
    res += info.nsec;
    soft_barrier();
    v2 = info.version;
  } while ((info.version & 1) || v1 != v2);

  return res;
}

u64 KvmSystemTime::ProcessorNano(u64 time)
{
  if (time_info.tsc_shift > 0) {
    time <<= time_info.tsc_shift;
  } else if (time_info.tsc_shift < 0) {
    time >>= -time_info.tsc_shift;
  }

  // we're doing mul in ASM because compiler will generate a call to runtime
  // function for u64 * u64
  //
  // copied from osv.
  // -Mike
  asm("mulq %1; shrd $32, %%rdx, %0"
      : "+a"(time)
      : "rm"((u64) time_info.tsc_to_sys_mul)
      : "rdx");
  return time;
}

static const u8 kTscStableBit = (1 << 0);

u64 KvmSystemTime::BootTime(bool use_hw_stable)
{
  u8 flags;
  u32 v1, v2;
  u64 time;
  do {
    v1 = time_info.version;
    lfence_barrier();
    time = time_info.sys_time
      + ProcessorNano(CpuPlatform::ReadTimeStamp() - time_info.tsc_ts);
    flags = time_info.flags;
    soft_barrier();
    v2 = time_info.version;
  } while ((time_info.version & 0x01) || v1 != v2);

  if (use_hw_stable && (flags & kTscStableBit) != 0) {
    return time;
  }

  // TODO: this is the algorithm in osv, not sure if it's a good idea
  // I suggest we should juse use the stable bit. Chestnut is design to run on
  // stable and modern hypervisor.
  // -Mike
  u64 last = 0;
  do {
    last = last_boot_time;
    if (time <= last) {
      return last;
    }
  } while (__sync_val_compare_and_swap(&last_boot_time, last, time) != last);

  return time;
}

}
