#include "page-table.h"
#include "mm/allocator.h"

extern long Pd;

namespace kernel {

static PageTable kernel_pgt;

static void MapPagesToPageTable(Pml4Entry &kern_entry, u64 start_addr, u64 len, bool skip = true, bool write_through = false)
{
  for (u64 addr = start_addr; addr < start_addr + len; ) {
    u64 vaddr = KERN_OFFSET + addr;
    u64 paddr = addr;
    // kprintf("MapPages addr %lx\n", addr);
    if (skip && addr % HUGEPAGESIZE == 0 && addr + HUGEPAGESIZE <= start_addr + len) {
      auto entry = PdptEntry(paddr);
      entry.set_huge(true);
      entry.set_write_through(write_through);
      kern_entry
        .AllocateOrPresent()[vaddr] = entry;
      addr += HUGEPAGESIZE;
    } else if (skip && addr % LARGEPAGESIZE == 0 && addr + LARGEPAGESIZE <= start_addr + len) {
      auto entry = PdEntry(paddr);
      entry.set_huge(true);
      entry.set_write_through(write_through);
      kern_entry
        .AllocateOrPresent()[vaddr]
        .AllocateOrPresent()[vaddr] = entry;
      addr += LARGEPAGESIZE;
    } else {
      auto entry = PtEntry(paddr);
      entry.set_write_through(write_through);
      kern_entry
        .AllocateOrPresent()[vaddr]
        .AllocateOrPresent()[vaddr]
        .AllocateOrPresent()[vaddr] = entry;

      // kprintf("DC %lx PRT %lx VA %lx\n",
      //         kern_entry[vaddr][vaddr][vaddr].physical_address(),
      //         kern_entry[vaddr][vaddr].physical_address(),
      //         vaddr);

      addr += PAGESIZE;
    }
  }
}

static void DumpPageTableFromEntry(const CommonBaseEntry &entry, int level)
{
  u64 paddr = entry.physical_address();
  for (int i = 0; i < level; i++) kprintf(" ");
  if (!entry.is_present()) kprintf("INVALID ");
  if (entry.is_huge()) kprintf("HUGE ");
  kprintf("0x%lx\n", paddr);
  if (entry.is_huge() || level == 0 || !entry.is_present()) return;

  auto next_level_entries = (CommonBaseEntry *) PADDR_TO_KPTR(paddr);
  for (int i = 0; i < 512; i++) {
    DumpPageTableFromEntry(next_level_entries[i], level - 1);
  }
}

void InitKernelPageTable(struct multiboot_tag_mmap *mm)
{
  new (&kernel_pgt) PageTable();

  if (Global<MemPages>().max_physical_addr() > KERN_MAX_MEMORY)
    panic("Cannot support more than 512GB memory!");

  kprintf("Init Kernel Page Table\n");

  Pml4Entry kern_entry;

  int nr_entries = (mm->size - sizeof(multiboot_tag_mmap)) / mm->entry_size;

  for (int i = 0; i < nr_entries; i++) {
    auto entry = &mm->entries[i];
    if (entry->addr % PAGESIZE != 0 || entry->len % PAGESIZE != 0) continue; // unpaged memory

    if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
      MapPagesToPageTable(kern_entry, entry->addr, entry->len);
    } else {
      MapPagesToPageTable(kern_entry, entry->addr, entry->len, false, true);
    }
  }
  // DumpPageTableFromEntry(kern_entry, 3);
  // during boot, we mapped the low address => low address in our page table
  // because EIP is at low address. Now our EIP is at kernel .text section,
  // and we never need to jump back again to low address any more. Therefore,
  // we unmapped the low address => low address mapping for protection.
  kernel_pgt[0].Clear();
  kernel_pgt[KERN_OFFSET] = kern_entry;

  kernel_pgt.Install();
}

PageTable &GetKernelPageTable()
{
  return kernel_pgt;
}

void InitBootPageTable()
{
  PdEntry *entries = (PdEntry *) Pd;
  u64 base_addr = 0;
  for (int i = 0; i < 512; i++, base_addr += (2 << 20)) {
    entries[i] = PdEntry(base_addr);
    entries[i].set_huge(true);
    entries[i].set_present(true);
    entries[i].set_read_write(true);
  }
}

}
