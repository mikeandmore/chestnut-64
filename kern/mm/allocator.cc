#include "allocator.h"

namespace kernel {

void Allocator::Init(struct multiboot_tag_mmap *mm)
{
	int nr_entries = (mm->size - sizeof(multiboot_tag_mmap)) / mm->entry_size;
	for (int i = 0; i < nr_entries; i++) {
		struct multiboot_mmap_entry *entry = &mm->entries[i];
		if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
			avail_size_ += entry->len;
		} else if (entry->type == MULTIBOOT_MEMORY_RESERVED) {
			reserved_size_ += entry->len;
		}
		tot_size_ = MAX((u64) entry->addr + entry->len, tot_size_);
	}
}

Allocator *alloc = 0;

}
