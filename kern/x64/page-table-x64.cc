#include "page-table-x64.h"
#include "mm/allocator.h"

namespace kernel {

static PageTableX64 kernel_pgt;

void InitKernelPageTable()
{
  new (&kernel_pgt) PageTableX64();
  // fill out the left over 511GB address space, so that PADDR_TO_KPTR can
  // simply add an offset to PADDR
  if (GlobalInstance<MemPages>().max_physical_addr() > KERN_MAX_MEMORY)
    panic("Cannot support more than 512GB memory!");

  u64 vaddr_start = KERN_OFFSET + HUGEPAGESIZE; // we already have 1G mapped
  u64 vaddr_end = KERN_OFFSET + GlobalInstance<MemPages>().max_physical_addr();
  u64 phyaddr = HUGEPAGESIZE;

  for (u64 vaddr = vaddr_start; vaddr < vaddr_end;
       vaddr += HUGEPAGESIZE, phyaddr += HUGEPAGESIZE) {
    kernel_pgt[vaddr][vaddr] = PdptEntry(phyaddr);
  }
  // during boot, we mapped the low address => low address in our page table
  // because EIP is at low address. Now our EIP is at kernel .text section,
  // and we never need to jump back again to low address any more. Therefore,
  // we unmapped the low address => low address mapping for protection.
  kernel_pgt[0].Clear();
}

PageTableX64 &GetKernelPageTable()
{
  return kernel_pgt;
}

}
