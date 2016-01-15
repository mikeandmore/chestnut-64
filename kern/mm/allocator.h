// -*- c++ -*-
#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "multiboot2.h"
#include "libc/common.h"

#define PAGESHIFT 12
#define PAGESIZE (1 << PAGESHIFT)
#define PAGEMASK (PAGESIZE - 1)

#define LARGEPAGESHIFT 21
#define LARGEPAGESIZE (1 << LARGEPAGESHIFT)
#define LARGEPAGEMASK (LARGEPAGESHIFT - 1)

#define HUGEPAGESHIFT 30
#define HUGEPAGESIZE (1 << HUGEPAGESHIFT)
#define HUGEPAGEMASK (HUGEPAGESIZE - 1)

extern long _virt_base;

#define KERN_OFFSET ((u64) &_virt_base)
#define KERN_MAX_MEMORY (512ULL << 30)

#define PGNUM(pg_addr) (pg_addr >> PAGESHIFT)
#define PG(addr) (PGNUM(addr) << PAGESHIFT)
#define PGALIGN(addr) (((addr - 1) | PAGEMASK) + 1)

typedef u64 paddr;

#define PADDR_TO_KPTR(paddr) ((void *) ((u64) paddr + KERN_OFFSET))
#define KPTR_TO_PADDR(ptr) ((u64) ptr - KERN_OFFSET)

namespace kernel {

class MemCacheBase;

class Page {
  // TODO: shrink the size of this class
public:
  paddr physical_address() const { return phyaddr; }
private:
friend class MemPages;
  paddr phyaddr;
  Page *alloc_next;
  Page *alloc_prev;

  bool is_free;
  bool is_in_list;
public:
  void *slab_ptr;
  int slab_obj_size;
};

class MemPages {
public:
  MemPages() : avail_size_(0), reserved_size_(0), tot_size_(0) {}
  MemPages(const MemPages &rhs) = delete;

  void Init(struct multiboot_tag_mmap *boot_mem_map);

  u64 available() const { return avail_size_; }
  u64 reserved() const { return reserved_size_; }
  u64 acpi_mem() const { return acpi_size_; }
  u64 total() const { return tot_size_; }
  u64 max_physical_addr() const { return max_phy_; }

  Page *AllocPage();
  void FreePage(Page *pg);
  Page *AllocPages(int n);
  void FreePages(Page *pg, int n);

  Page *page(paddr addr) { return &page_structs[PGNUM(addr)]; }
private:
  static const int kBootLoaderSkipPages = 48;
  void CollectAvailable(struct multiboot_mmap_entry *entries,
                        int nr_entries);

private:
  u64 avail_size_, reserved_size_, acpi_size_;
  u64 tot_size_;
  u64 avail_low_;
  u64 max_phy_;

  Page page_head; // free list (double linked_list)
  Page *page_structs; // array of pages
  u64 nr_page_structs;
};

void *Alloc(int obj_size);
void Free(void *ptr);

}

#endif /* ALLOCATOR_H */
