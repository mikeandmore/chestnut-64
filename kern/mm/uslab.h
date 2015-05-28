// -*- c++ -*-
#ifndef USLAB_H
#define USLAB_H

#include "libc/common.h"
#include "mm/allocator.h"
#include "libc/list.h"
#include "console.h"

namespace kernel {

class MemCacheBase {
protected:
	struct {
		ListNode full, half, empty;
	} slab_queue;
	struct {
		u64 allocated, slab_pg, max_pg;
	} stat;
public:
	MemCacheBase();

	virtual void *Allocate() = 0;
	virtual void Free(void *ptr) = 0;

	virtual MemCacheBase *CreateNew() = 0;

	void set_max_page(u64 max_page) { stat.max_pg = max_page; }
	u64 max_page() { return stat.max_pg; }

	void PrintStat() {
		console->printf("allocated: %ld pg %ld max_pag %ld\n",
				stat.allocated, stat.slab_pg, stat.max_pg);
	}
};

// buddy allocation
MemCacheBase *FitGlobalMemCache(int obj_size);

void InitSlab();

}


#endif /* USLAB_H */
