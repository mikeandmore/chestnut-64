#include "libc/common.h"
#include "libc/string.h"
#include "allocator.h"
#include "console.h"

extern long _boot_stack;
extern long _load_start;
extern long _load_end;
extern long _bss_end;

namespace kernel {

void MemPages::Init(struct multiboot_tag_mmap *mm)
{
#if 1
  kprintf("Kernel Stack Top: 0x%lx Start: 0x%lx End: 0x%lx BSSEnd: 0x%lx\n",
          &_boot_stack, &_load_start, &_load_end, &_bss_end);
#endif

  int nr_entries = (mm->size - sizeof(multiboot_tag_mmap)) / mm->entry_size;
  kprintf("Physical Memory Map:\n");
  u64 max_kern_mem = 0;
  for (int i = 0; i < nr_entries; i++) {
    struct multiboot_mmap_entry *entry = &mm->entries[i];
    if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
      if (entry->addr == 0) {
        avail_low_ = entry->len;
      } else {
        avail_size_ += entry->len;
        u64 kern_mem = entry->addr + entry->len;
        max_kern_mem = max_kern_mem > kern_mem ? max_kern_mem : kern_mem;
      }
    } else if (entry->type == MULTIBOOT_MEMORY_RESERVED) {
      reserved_size_ += entry->len;
    } else if (entry->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
      acpi_size_ += entry->len;
    }
    tot_size_ += entry->len;
    nr_page_structs = max_kern_mem / PAGESIZE;

    kprintf("Addr: %lx Len: %lx Type: %d\n",
                   entry->addr, entry->len, entry->type);
  }
  kprintf("Max Kern Mem: %ld KB\n", max_kern_mem / 1024);

  kprintf("Total Memory: %ld KB\n"
          "  Available: %ld KB\n"
          "  Reserved: %ld KB\n"
          "  ACPI: %ld KB\n\n",
          total() / 1024,
          available() / 1024,
          reserved() / 1024,
          acpi_mem() / 1024);

  CollectAvailable(mm->entries, nr_entries);
}

void MemPages::CollectAvailable(struct multiboot_mmap_entry *entries,
                                int nr_entries)
{
  // we are on page [kstart_pg, kend_pg)
  u64 kstart_pg = PG((u64) &_load_start);
  u64 kend_pg = PGALIGN((u64) &_bss_end) + kBootLoaderSkipPages * PAGESIZE;

  u64 struct_tot_sz = sizeof(Page) * nr_page_structs;
  // we store them right after the kernel, which is starting from
  // [kend_pg, kend_pg + nr_pages_structs * PAGESIZE)
  page_structs = (Page *) PADDR_TO_KPTR(kend_pg);
  memset(page_structs, 0, struct_tot_sz);

  Page *last_page = &page_head;
  for (int i = 0; i < nr_entries; i++) {
    struct multiboot_mmap_entry *ent = &entries[i];
    if (ent->type != MULTIBOOT_MEMORY_AVAILABLE || ent->addr == 0)
      continue;
    for (u64 paddr = ent->addr; paddr < ent->addr + ent->len;
         paddr += PAGESIZE) {
      kassert(PGNUM(paddr) < nr_page_structs);
      Page *pg = page(paddr);
      pg->phyaddr = PG(paddr);
      // if this address is in kernel image, bss, bootloader
      // data or page of pages, then we ignore them, and
      // therefore they won't be allocated
      if (paddr >= kstart_pg && paddr < kend_pg + struct_tot_sz) {
        pg->is_free = false; pg->is_in_list = false;
      } else {
        pg->is_free = true; pg->is_in_list = true;
        pg->alloc_next = &page_head;
        pg->alloc_prev = last_page;
        page_head.alloc_prev = pg;
        last_page->alloc_next = pg;
        last_page = pg;
      }
    }
  }
  kprintf("Memory System Initialized\n");
}

Page *MemPages::AllocPage()
{
  if(page_head.alloc_next == page_head.alloc_prev)
    return NULL;
  Page *allocated_page = page_head.alloc_next;
  page_head.alloc_next = allocated_page->alloc_next;
  page_head.alloc_next->alloc_prev = &page_head;
  allocated_page->is_free = false;
  allocated_page->is_in_list = false;
  return allocated_page;
}

void MemPages::FreePage(Page *pg)
{
  pg->alloc_next = page_head.alloc_next;
  page_head.alloc_next->alloc_prev = pg;
  page_head.alloc_next = pg;
  pg->alloc_prev = &page_head;
  pg->is_free = true;
  pg->is_in_list = true;
}

Page *MemPages::AllocPages(int n)
{
  if (n == 1) return AllocPage();

  int64 start = -1;
  for (int64 i = 0; i < nr_page_structs; i++) {
    if (!page_structs[i].is_free){
      start = -1;
      continue;
    }
    if (start == -1 && i > nr_page_structs - n )
      break;
    else if (start == -1)
      start = i;
    if (i == start + n - 1)
      break;

  }
  if (start == -1)
    return NULL;
  for (int64 i = start; i < start + n; i++) {
    page_structs[i].alloc_prev->alloc_next = page_structs[i].alloc_next;
    page_structs[i].alloc_next->alloc_prev = page_structs[i].alloc_prev;
    page_structs[i].is_free = false;
    page_structs[i].is_in_list = false;
  }
  return &page_structs[start];
}

void MemPages::FreePages(Page *pg, int n)
{
  if (n == 1) {
    FreePage(pg);
    return;
  }
  for (int64 i = 0; i < n; i++) {
    pg[i].alloc_next = page_head.alloc_next;
    page_head.alloc_next->alloc_prev = &pg[i];
    page_head.alloc_next = &pg[i];
    pg[i].alloc_prev = &page_head;
    pg[i].is_free = true;
    pg[i].is_in_list = true;
  }

}

}

static kernel::MemPages mem_pages;

template <>
kernel::MemPages &GlobalInstance()
{
  return mem_pages;
}
