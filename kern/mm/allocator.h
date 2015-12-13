// -*- c++ -*-
#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "multiboot2.h"
#include "libc/common.h"

#define PAGESHIFT 12
#define PAGESIZE (1 << PAGESHIFT)
#define PAGEMASK (PAGESIZE - 1)

#define PGNUM(pg_addr) (pg_addr >> PAGESHIFT)
#define PG(addr) (PGNUM(addr) << PAGESHIFT)
#define PGALIGN(addr) (((addr - 1) | PAGEMASK) + 1)

typedef u64 paddr;

extern long VIRT_BASE;

#define PADDR_TO_KPTR(paddr) ((void *) ((u64) paddr + (u64) &VIRT_BASE))
#define KPTR_TO_PADDR(ptr) ((u64) ptr - (u64) &VIRT_BASE)

namespace kernel {

class MemCacheBase;

class Page {
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

  void Init(struct multiboot_tag_mmap *boot_mem_map);

  u64 available() const { return avail_size_; }
  u64 reserved() const { return reserved_size_; }
  u64 acpi_mem() const { return acpi_size_; }
  u64 total() const { return tot_size_; }

  Page *AllocPage();
  void FreePage(Page *pg);
  Page *AllocPages(int n);
  void FreePages(Page *pg, int n);

  Page *page(paddr addr) { return &page_structs[PGNUM(addr)]; }
private:
  static const int kBootLoaderSkipPages = 16;
  void CollectAvailable(struct multiboot_mmap_entry *entries,
                        int nr_entries);

private:
  u64 avail_size_, reserved_size_, acpi_size_;
  u64 tot_size_;
  u64 avail_low_;

  Page page_head; // free list (double linked_list)
  Page *page_structs; // array of pages
  int64 nr_page_structs;
};

void *Alloc(int obj_size);
void Free(void *ptr);

}

#endif /* ALLOCATOR_H */
