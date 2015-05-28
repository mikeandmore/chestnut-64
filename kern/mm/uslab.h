// -*- c++ -*-
#ifndef USLAB_H
#define USLAB_H

#include "libc/common.h"
#include "mm/allocator.h"
#include "libc/list.h"
#include "console.h"

namespace kernel {

class BaseSlab {
protected:
	ListNode node;
	Page *data_page;
public:
	ListNode *list_node() { return &node; }
	u8 *mem() { return (u8 *) PADDR_TO_KPTR(data_page->physical_address()); }

	static BaseSlab *AllocSlab(int obj_size);
	static void FreeSlab(BaseSlab *slab);
};

class BaseBitmapSlab : public BaseSlab {
protected:
	int AllocFromBitmapArray(int n, u64 *bitmap_array);
	void FreeToBitmapArray(int n, u64 *bitmap_array, int obj_idx);
};

template <int NBitLine>
class BitmapSlab : public BaseBitmapSlab {
protected:
	// 1 means free slot, 0 means in use
	u64 bitmap[NBitLine];
public:
	void Init(int total) {
		for (int i = 0; i < NBitLine; i++) {
			bitmap[i] = -1LL;
		}

	}
	int Alloc() {
		return AllocFromBitmapArray(NBitLine, bitmap);
	}

	void Free(int obj_idx) {
		FreeToBitmapArray(NBitLine, bitmap, obj_idx);
	}

	bool is_empty() {
		u64 bitsum = bitmap[0];
		for (int i = 1; i < NBitLine; i++) {
			bitsum &= bitmap[i];
		}
		return bitsum == -1LL;

	}

	bool is_full() {
		u64 bitsum = bitmap[0];
		for (int i = 1; i < NBitLine; i++) {
			bitsum |= bitmap[i];
		}
		return bitsum == 0ULL;
	}
};

class FreeListSlab : public BaseSlab {
protected:
	u8 nr_free;
	u8 nr_total;
	u8 free_stack[38];
public:
	void Init(int total);
	int Alloc();
	void Free(int obj_idx);

	bool is_empty() { return nr_free == nr_total; }
	bool is_full() { return nr_free == 0; }
};

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

void *kmalloc(int obj_size);
void kfree(void *ptr);


template <class Slab, int ObjSize>
class MemCache : public MemCacheBase {
public:
	bool ReclaimEmptySlab(Slab *slab) {
		if (stat.slab_pg <= stat.max_pg) return false;
		stat.slab_pg--;
		Slab::FreeSlab(slab);
		return true;
	}
	void AdjustSlab(Slab *slab) {
		ListNode *node = slab->list_node();
		if (slab->is_full()) {
			node->InsertAfter(&slab_queue.full);
		} else if (slab->is_empty()) {
			if (!ReclaimEmptySlab(slab))
				node->InsertAfter(&slab_queue.empty);

		} else {
			node->InsertAfter(&slab_queue.half);
		}
	}

	virtual void *Allocate() {
		Slab *slab = NULL;
		if (slab_queue.half.is_empty()) {
			if (slab_queue.empty.is_empty()) {
				slab = (Slab *) Slab::AllocSlab(ObjSize);
				slab->Init(PAGESIZE / ObjSize);
				stat.slab_pg++;
			} else {
				ListNode *node = slab_queue.empty.next;
				node->Delete();
				slab = (Slab *) node;
			}
		} else {
			slab = (Slab *) slab_queue.half.next;
			slab->list_node()->Delete();
		}
		int idx = slab->Alloc();
		u8* mem = slab->mem();
		stat.allocated++;
		AdjustSlab(slab);

		return mem + idx * ObjSize;
	}

	virtual void Free(void *ptr) {
		// use the struct page to find its slab
		paddr addr = KPTR_TO_PADDR(ptr);
		Page *pg = alloc->page(addr);
		Slab *slab = (Slab *) pg->slab_ptr;

		slab->list_node()->Delete();
		int idx = (addr - PG(addr)) / ObjSize;
		slab->Free(idx);
		stat.allocated--;
		AdjustSlab(slab);
	}

	virtual MemCacheBase *CreateNew() {
		int this_size = sizeof(MemCache<Slab, ObjSize>);
		MemCacheBase *global_cache = FitGlobalMemCache(this_size);
		MemCache<Slab, ObjSize> *new_ins =
			(MemCache<Slab, ObjSize> *) global_cache->Allocate();
		// placement new, call constructor
		new (new_ins) MemCache<Slab, ObjSize>();
		return new_ins;
	}
};

void InitSlab();

}


#endif /* USLAB_H */
