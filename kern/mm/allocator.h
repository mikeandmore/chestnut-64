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

namespace kernel {

class Page {
	paddr physical_address() const { return phyaddr; }
private:
friend class Allocator;
	paddr phyaddr;
	Page *alloc_next;
	bool is_free;
	bool is_in_list;
};

class Allocator {
public:
	Allocator() : avail_size_(0), reserved_size_(0), tot_size_(0) {}

	void Init(struct multiboot_tag_mmap *boot_mem_map);

	u64 available() const { return avail_size_; }
	u64 reserved() const { return reserved_size_; }
	u64 acpi_mem() const { return acpi_size_; }
	u64 total() const { return tot_size_; }

	Page *AllocPage();
	void FreePage(Page *pg);
	Page *AllocPages(int n);
	void FreePages(Page *pg, int n);
private:
	static const int kBootLoaderSkipPages = 4;
	void CollectAvailable(struct multiboot_mmap_entry *entries,
			      int nr_entries);

private:
	u64 avail_size_, reserved_size_, acpi_size_;
	u64 tot_size_;
	u64 avail_low_;

	Page *page_head; // free list
	Page *page_structs; // array of pages

private:
	// TODO: for Alloc/Free()
};

class SlabAllocator
{

};

extern Allocator *alloc;
}

#endif /* ALLOCATOR_H */
