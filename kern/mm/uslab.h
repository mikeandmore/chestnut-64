// -*- c++ -*-
#ifndef USLAB_H
#define USLAB_H

#include "mm/allocator.h"
#include "libc/list.h"

namespace kernel {

class BaseSlab {
protected:
	ListNode node; // for the full/half/empty queue
	Page *data_page;
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

	bool is_empty();
	bool is_full();
};

struct MetaSlabPageHeader {
	ListNode node;
	struct BaseSlab *slab_next; // slab free list in this page
};

class MemCacheBase {
	struct {
		ListNode full, half, empty;
	} slab_queue;

	struct {
		ListNode full, half;
	} page_queue;

	struct {
		u64 allocated, pool_size;
	} stat;
};

template <class Slab>
class MemCache : public MemCacheBase {

};

}

#endif /* USLAB_H */
