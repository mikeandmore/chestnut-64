#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "libc/common.h"
#include "libc/string.h"
#include "mm/allocator.h"
#include "multiboot2.h"

/**
 * PageDirectory Entry:
 * [31..PageTable 4k Aligned..12][11..Avail..9][8..G S 0 A D W U R P..0]
 * G: Global
 * S: Huge Page?
 * A: Accessed
 * D: Cache Disabled
 * W: Write Through
 * U: User/Kernel?
 * R: Read/Write
 * P: Present
 *
 * PageTable Entry:
 * [31..Physical Page Address..12][11..Avail..9][8..G 0 D A C W U R P..0]
 * G: Global
 * D: Dirty
 * A: Accessed
 * W: Write Through
 * U: User/Kernel?
 * R: Read/Write
 * P: Present
 *
 */

namespace kernel {

class CommonBaseEntry {
protected:
  u64 entry;
  void set_flag(bool b, int bit) {
    u64 flag = (0x01ULL << bit);
    if (b) entry |= flag;
    else entry &= ~flag;
  }
  bool get_flag(int bit) const {
    u64 flag = (0x01ULL << bit);
    return (entry & flag) != 0;
  }

public:
  CommonBaseEntry() : entry(0) {}

  void Allocate() {
    entry = Global<MemPages>().AllocPage()->physical_address();
    memset(PADDR_TO_KPTR(entry), 0, PAGESIZE);
    set_present(true);
    set_read_write(true);
    // kprintf("Allocate Page Table entry %lx\n", entry);
  }

  void Clear() {
    entry = 0;
  }

  CommonBaseEntry(u64 paddr) : entry(paddr) {
    set_present(true);
    set_read_write(true);
  }

  void set_present(bool present) { set_flag(present, 0); }
  bool is_present() const { return get_flag(0); }
  void set_read_write(bool ro) { set_flag(ro, 1); }
  bool is_read_write() const { return get_flag(1); }
  void set_kernel_mode(bool kern) { set_flag(kern, 2); }
  bool is_kernel_mode() const { return get_flag(2); }
  void set_write_through(bool wt) { set_flag(wt, 3); }
  bool is_write_through() const { return get_flag(3); }
  void set_cache_disabled(bool cd) { set_flag(cd, 4); }
  bool is_cache_disabled() const { return get_flag(4); }

  void set_huge(bool huge) { set_flag(huge, 7); }
  bool is_huge() const { return get_flag(7); }

  u64 physical_address() const { return entry & ~(0x01FF); }
};

template <unsigned int Level>
class CommonEntry : public CommonBaseEntry {
public:
  CommonEntry() : CommonBaseEntry() {}
  CommonEntry(u64 paddr) : CommonBaseEntry(paddr) {}

  CommonEntry<Level> &AllocateOrPresent() {
    if (!is_present()) Allocate();
    return *this;
  }

  CommonEntry<Level - 1> &operator[](u64 vaddr) {
    if (Level == 1 && is_huge()) {
      panic("Cannot access huge page dir entry");
    }
    auto next_level = static_cast<CommonEntry<Level - 1> *>(
      PADDR_TO_KPTR(physical_address()));
    u16 offset = (u16) ((vaddr >> (12 + (Level -1) * 9)) & 0x01FF);
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
    return Global<MemPages>().page(physical_address());
  }
};

typedef CommonEntry<3> Pml4Entry;
typedef CommonEntry<2> PdptEntry;
typedef CommonEntry<1> PdEntry;
typedef CommonEntry<0> PtEntry;

class PageTable : public CommonEntry<4> {
public:
  PageTable() : CommonEntry<4>() {
    asm volatile("mov %%cr3, %0" : "=r"(entry));
  }
  void Install() {
    asm volatile("mov %0, %%cr3":: "b"(entry));
  }

  void MapPage(u64 paddr, bool write_through = false, bool cache_disabled = false) {
    u64 vaddr = KERN_OFFSET + paddr;
    auto entry = PtEntry(paddr);
    entry.set_write_through(write_through);
    entry.set_cache_disabled(cache_disabled);

    this->operator[](vaddr)
      .AllocateOrPresent()[vaddr]
      .AllocateOrPresent()[vaddr]
      .AllocateOrPresent()[vaddr] = entry;
  }
};

static_assert(sizeof(CommonEntry<4>) == 8
              && sizeof(Pml4Entry) == 8
              && sizeof(PdptEntry) == 8
              && sizeof(PdEntry) == 8
              && sizeof(PtEntry) == 8, "Entry should be 8 bytes");

void InitBootPageTable();
void InitKernelPageTable(struct multiboot_tag_mmap *boot_mem_map);
PageTable &GetKernelPageTable();
void InitBootPageTable();

}

#endif /* PAGE_TABLE_H */
