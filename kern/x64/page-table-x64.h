#ifndef PAGE_TABLE_X64_H
#define PAGE_TABLE_X64_H

#include "libc/common.h"
#include "mm/allocator.h"

namespace kernel {

class CommonBaseEntry {
protected:
  u64 entry;
  void set_flag(bool b, int bit) {
    u64 flag = (0x01ULL << bit);
    if (b) entry |= flag;
    else entry &= ~flag;
  }
  bool get_flag(int bit) {
    u64 flag = (0x01ULL << bit);
    return (entry & flag) != 0;
  }

public:
  CommonBaseEntry() : entry(0) {}

  void Allocate() {
    entry = GlobalInstance<MemPages>().AllocPage()->physical_address();
    set_present(true);
  }

  void Clear() {
    entry = 0;
  }

  CommonBaseEntry(u64 paddr) : entry(paddr) {
    set_present(true);
  }

  void set_present(bool present) { set_flag(present, 0); }
  bool is_present() { return get_flag(0); }
  void set_read_only(bool ro) { set_flag(ro, 1); }
  bool is_read_only() { return get_flag(1); }
  void set_kernel_mode(bool kern) { set_flag(kern, 2); }
  bool is_kernel_mode() { return get_flag(2); }
  void set_write_through(bool wt) { set_flag(wt, 3); }
  bool is_write_through() { return get_flag(3); }
  void set_cache_disabled(bool cd) { set_flag(cd, 4); }
  bool is_cache_disabled() { return get_flag(4); }

  void set_huge(bool huge) { set_flag(huge, 7); }
  bool is_huge() { return get_flag(7); }

  u64 physical_address() const { return entry & ~(0x01FF); }
};

template <unsigned int Level>
class CommonEntry : public CommonBaseEntry {
public:
  CommonEntry() : CommonBaseEntry() {}
  CommonEntry(u64 paddr) : CommonBaseEntry(paddr) {}

  CommonEntry<Level - 1> &operator[](u64 vaddr) {
    if (Level == 1 && is_huge()) {
      panic("Cannot access huge page dir entry");
    }
    auto next_level = static_cast<CommonEntry<Level - 1> *>(
      PADDR_TO_KPTR(physical_address()));
    u16 offset = (u16) ((vaddr >> (12 + Level * 9)) & 0x01FF);
    kassert(offset < 512);
    return next_level[offset];
  }
};

template <>
class CommonEntry<0> : public CommonBaseEntry {
public:
  CommonEntry() : CommonBaseEntry() {}
  CommonEntry(u64 paddr) : CommonBaseEntry(paddr) {}

  Page *get_page() {
    return GlobalInstance<MemPages>().page(physical_address());
  }
};

typedef CommonEntry<3> Pml4Entry;
typedef CommonEntry<2> PdptEntry;
typedef CommonEntry<1> PdEntry;
typedef CommonEntry<0> PtEntry;

class PageTableX64 : public CommonEntry<4> {
public:
  PageTableX64() : CommonEntry<4>() {
    asm volatile("mov %%cr3, %0" : "=r"(entry));
  }
  void Install() {
    asm volatile("mov %0, %%cr3":: "b"(entry));
  }
};

static_assert(sizeof(CommonEntry<4>) == 8
              && sizeof(Pml4Entry) == 8
              && sizeof(PdptEntry) == 8
              && sizeof(PdEntry) == 8
              && sizeof(PtEntry) == 8, "Entry should be 8 bytes");

void InitKernelPageTable();
PageTableX64 &GetKernelPageTable();

}

#endif /* PAGE_TABLE_X64_H */
