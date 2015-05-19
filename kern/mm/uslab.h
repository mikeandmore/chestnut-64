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
	ListNode node; // for the full/half/empty queue
	Page *data_page;
public:
	ListNode *list_node() { return &node; }
	u8 *mem() { return (u8 *) PADDR_TO_KPTR(data_page->physical_address()); }

	static BaseSlab *AllocSlab(int obj_size);
	static void FreeSlab(BaseSlab *slab);
};

class BaseBitmapSlab : public BaseSlab {
protected:
	// allocation can use __builtin_ctzl()
	int AllocFromBitmapArray(int n, u64 *bitmap_array); // TODO: implement this
	void FreeToBitmapArray(int n, u64 *bitmap_array, int obj_idx); // TODO:
};

template <int NBitLine>
class BitmapSlab : public BaseBitmapSlab {
protected:
	// 1 means free slot, 0 means in use
	u64 bitmap[NBitLine];
	// reserved, might be used for history buffer later
	u8 __padding[40 - NBitLine * sizeof(u64)];
public:
	void Init(int total) {
		for (int i = 0; i < NBitLine; i++) {
			bitmap[i] = -1LL;
		}

	}
	int Alloc() {
		// TODO: implement, right here
		return AllocFromBitmapArray(NBitLine, bitmap);
	}

	void Free(int obj_idx) {
		// TODO: implement, right here
		FreeToBitmapArray(NBitLine, bitmap, obj_idx);
	}

	bool is_empty() {
		// TODO: bitwise and all bitmap to see if it's 0xFFFF...
		u64 bitsum = bitmap[0];
		for (int i = 1; i < NBitLine; i++) {
			bitsum &= bitmap[i];
		}
		return bitsum == -1LL;

	}

	bool is_full() {
		// TODO: bitwise or all bitmap to see if it's zero
		u64 bitsum = bitmap[0];
		for (int i = 1; i < NBitLine; i++) {
			bitsum &= bitmap[i];
		}
		//0ULL unsigned long long
		return bitsum == 0ULL;
	}
};

class FreeListSlab : public BaseSlab {
protected:
	u8 nr_free;
	u8 nr_total;
	u8 free_stack[38];
public:
	// TODO:
	void Init(int total);
	int Alloc();
	void Free(int obj_idx);

	bool is_empty() { return nr_free == nr_total; }
	bool is_full() { return nr_free == 0; }
};

template <class Slab, int ObjSize>
class MemCache {
	struct {
		ListNode full, half, empty;
	} slab_queue;
	struct {
		u64 allocated, slab_pg, max_pg;
	} stat;
public:
	bool ReclaimEmptySlab(Slab *slab) {
		if (stat.slab_pg <= stat.max_pg) return false;
		stat.slab_pg--;
		Slab::FreeSlab(slab);
		return true;
	}
	void AdjustSlab(Slab *slab) {
		ListNode *node = slab->list_node();
		node->Delete();
		if (slab->is_full()) {
			node->InsertAfter(&slab_queue.full);
		} else if (slab->is_empty()) {
			if (!ReclaimEmptySlab(slab))
				node->InsertAfter(&slab_queue.empty);

		} else {
			node->InsertAfter(&slab_queue.half);
		}
	}

	void set_max_page(u64 max_page) { stat.max_pg = max_page; }
	u64 max_page() { return stat.max_pg; }

	void Init(u64 max_page) {
		stat.allocated = 0;
		set_max_page(max_page);
		slab_queue.full.InitHead();
		slab_queue.half.InitHead();
		slab_queue.empty.InitHead();
	}

	void *Allocate() {
		Slab *slab = NULL;
		if (slab_queue.half.is_empty()) {
			if (slab_queue.empty.is_empty()) {
				slab = Slab::AllocSlab(ObjSize);
				slab->Init(PAGESIZE / ObjSize);
				stat.slab_pg++;
			} else {
				ListNode *node = slab_queue.empty.next;
				node->Delete();
				slab = (Slab *) node;
			}
			slab->list_node()->InsertAfter(&slab_queue.half);
		}
		slab = (Slab *) slab_queue.half.next;
		int idx = slab->Alloc();
		u8* mem = slab->mem();
		stat.allocated++;
		AdjustSlab(slab);
		return mem + idx * ObjSize;
	}

	void Free(void *ptr) {
		// use the struct page to find its slab
		paddr addr = KPTR_TO_PADDR(ptr);
		Page *pg = alloc->page(addr);
		Slab *slab = (Slab *) pg->slab_ptr;
		int idx = (addr - PG(addr)) / ObjSize;
		slab->Free(idx);
		stat.allocated--;
		AdjustSlab(slab);
	}

	void PrintStat() {
		console->printf("allocated: %ld pg %ld max_pag %ld\n",
				stat.allocated, stat.slab_pg, stat.max_pg);
	}
};

void InitSlab();

}


#endif /* USLAB_H */
