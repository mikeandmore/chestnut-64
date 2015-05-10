// -*- c++ -*-
#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "multiboot2.h"
#include "libc/common.h"

namespace kernel {

class Allocator {
public:
	Allocator() : avail_size_(0), reserved_size_(0), tot_size_(0) {}

	void Init(struct multiboot_tag_mmap *boot_mem_map);

	u64 available() const { return avail_size_; }
	u64 reserved() const { return reserved_size_; }
	u64 total() const { return tot_size_; }

private:
	u64 avail_size_, reserved_size_;
	u64 tot_size_;
};

extern Allocator *alloc;
}

#endif /* ALLOCATOR_H */
