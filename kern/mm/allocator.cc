#include "allocator.h"
#include "console.h"

extern long Stack;
extern long _loadStart;
extern long _loadEnd;
extern long _bssEnd;

namespace kernel {

void Allocator::Init(struct multiboot_tag_mmap *mm)
{
#if 1
	console->printf("Kernel Stack Top: 0x%lx Start: 0x%lx "
			"End: 0x%lx BSSEnd: 0x%lx\n",
			&Stack, &_loadStart, &_loadEnd, &_bssEnd);
#endif

	int nr_entries = (mm->size - sizeof(multiboot_tag_mmap)) / mm->entry_size;
	console->printf("Physical Memory Map:\n");
	for (int i = 0; i < nr_entries; i++) {
		struct multiboot_mmap_entry *entry = &mm->entries[i];
		if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
			if (entry->addr == 0) {
				avail_low_ = entry->len;
			} else {
				avail_size_ += entry->len;
			}
		} else if (entry->type == MULTIBOOT_MEMORY_RESERVED) {
			reserved_size_ += entry->len;
		} else if (entry->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
			acpi_size_ += entry->len;
		}
		tot_size_ += entry->len;

		console->printf("Addr: %lx Len: %lx Type: %d\n",
				entry->addr, entry->len, entry->type);
	}

	console->printf("Total Memory: %ld KB\n"
			"  Available: %ld KB\n"
			"  Reserved: %ld KB\n"
			"  ACPI: %ld KB\n",
			total() / 1024,
			available() / 1024,
			reserved() / 1024,
			acpi_mem() / 1024);

	CollectAvailable(mm->entries, nr_entries);
}

void Allocator::CollectAvailable(struct multiboot_mmap_entry *entries,
				 int nr_entries)
{
	// we are on page [kstart_pg, kend_pg)
	u64 kstart_pg = PG((u64) &_loadStart);
	u64 kend_pg = PGALIGN((u64) &_bssEnd) + kBootLoaderSkipPages * PAGESIZE;

	// should be aligned to page size already
	u64 nr_pages = available() / PAGESIZE;
	u64 struct_tot_sz = sizeof(Page) * nr_pages;
	u64 nr_pages_struct = PGALIGN(struct_tot_sz) / PAGESIZE;
	// we store them right after the kernel, which is starting from
	// [kend_pg, kend_pg + nr_pages_struct * PAGESIZE)
	page_structs = (Page *) PADDR_TO_KPTR(kend_pg);

	u64 init_cur_page = 0;
	Page *last_page = NULL;
	for (int i = 0; i < nr_entries; i++) {
		struct multiboot_mmap_entry *ent = &entries[i];
		if (ent->type != MULTIBOOT_MEMORY_AVAILABLE) continue;
		if (ent->addr == 0) continue;

		for (u64 paddr = ent->addr; paddr < ent->addr + ent->len;
		     paddr += PAGESIZE) {
			// if this address is in kernel image, bss, bootloader
			// data or page of pages, then we ignore them, and
			// therefore they won't be allocated
			if (paddr >= kstart_pg && paddr < kend_pg + struct_tot_sz) {
			  	continue;
			}
			Page *pg = &page_structs[init_cur_page];

			pg->phyaddr = paddr; pg->alloc_next = NULL;
			pg->is_free = true; pg->is_in_list = true;
			if (last_page != NULL)
				last_page->alloc_next = pg;
			last_page = pg;
			init_cur_page++;
		}

	}
	console->printf("Allocator: total pages: %ld available for alloc: %ld\n",
			nr_pages, init_cur_page);
	page_head = page_structs;
}


Page *Allocator::AllocPage()
{
	return NULL;
}

void Allocator::FreePage(Page *pg)
{

}

Page *Allocator::AllocPages(int n)
{
	return NULL;
}

void Allocator::FreePages(Page *pg, int n)
{

}

Allocator *alloc = 0;

}
