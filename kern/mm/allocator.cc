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

	nr_page_structs = 0;
	Page *last_page = &page_head;
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
			Page *pg = &page_structs[nr_page_structs];

			pg->phyaddr = paddr; pg->alloc_next = &page_head;
			pg->is_free = true; pg->is_in_list = true;
			last_page->alloc_next = pg;
			pg->alloc_prev = last_page;
			last_page = pg;
			nr_page_structs++;
		}

	}
	console->printf("Allocator: total pages: %ld available for alloc: %ld\n",
			nr_pages, nr_page_structs);
}


Page *Allocator::AllocPage()
{
	if(page_head.alloc_next == page_head.alloc_prev)
		return NULL;
	Page *allocatored_page;
	allocatored_page = page_head.alloc_next;
	page_head.alloc_next = allocatored_page->alloc_next;
	page_head.alloc_next->alloc_prev = &page_head;
	allocatored_page->is_free = false;
	allocatored_page->is_in_list = false;
	return allocatored_page;
}

void Allocator::FreePage(Page *pg)
{
	pg->alloc_next = page_head.alloc_next;
	page_head.alloc_next->alloc_prev = pg;
	page_head.alloc_next = pg;
	pg->alloc_prev = &page_head;
	pg->is_free = true;
	pg->is_in_list = true;
}

Page *Allocator::AllocPages(int n)
{
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

void Allocator::FreePages(Page *pg, int n)
{
	for (int64 i = 0; i < n; i++) {
		pg[i].alloc_next = page_head.alloc_next;
		page_head.alloc_next->alloc_prev = &pg[i];
		page_head.alloc_next = &pg[i];
		pg[i].alloc_prev = &page_head;
		pg[i].is_free = true;
		pg[i].is_in_list = true;
	}

}

Allocator *alloc = 0;

}
