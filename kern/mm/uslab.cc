// -*- c++ -*-
#include "uslab.h"

namespace kernel {

int BaseBitmapSlab::AllocFromBitmapArray(int n, u64 *bitmap_array)
{
	//n represent the NBitsmap's N
	int idx = 64;
	int row = 0;
	for (int i = 0; i < n; i++) {
		int temp_idx = __builtin_ctzl(bitmap_array[i]);
		if (temp_idx < idx) {
			idx = temp_idx;
			row = i;
		}
	}
	bitmap_array[row] ^= (1 << idx);
	return idx + 64 * row;
}

void BaseBitmapSlab::FreeToBitmapArray(int n, u64 *bitmap_array, int obj_idx)
{
	int row = obj_idx / 64;
	int idx = obj_idx % 64;
	bitmap_array[row] ^= (1 << idx);
}


void FreeListSlab::Init(int total)
{
	//total: how many objects will be in this page
	nr_free = total;
	nr_total = total;
	for (int i = 0; i < total; i++) {
		free_stack[i] = i;
	}
}

int FreeListSlab::Alloc()
{
	return free_stack[--nr_free];
}

void FreeListSlab::Free(int obj_idx)
{
	free_stack[nr_free] = obj_idx;
	nr_free++;
}

class MetaSlab {
	ListNode node;
	ListNode free_slab;
	u8 nr_free_slab;
public:
	ListNode *list_node() { return &node; }
	u8 *mem() { return (u8 *) this; }

	bool is_empty() {
		return nr_free_slab == 63;
	}

	bool is_full() {
		return nr_free_slab == 0;
	}

	void Init(int total);
	int Alloc();
	void Free(int obj_idx);

	static MetaSlab *AllocSlab(int obj_size);
	static void FreeSlab(MetaSlab *slab);
};

void MetaSlab::Init(int total)
{
	free_slab.InitHead();
	nr_free_slab = 63;
	u8 *ptr = (u8 *) this;
	ptr += 64;
	for (int i = 0; i < 63; i++, ptr += 64) {
		BaseSlab *real_slab = (BaseSlab *) ptr;
		ListNode *real_slab_node = real_slab->list_node();
		real_slab_node->InsertAfter(&free_slab);
	}
}

int MetaSlab::Alloc()
{
	ListNode *node = free_slab.next;
	node->Delete();
	nr_free_slab--;
	return ((u8 *) node - mem()) / 64;
}

void MetaSlab::Free(int obj_idx)
{
	ListNode *node = (ListNode *) (mem() + obj_idx * 64);
	node->InsertAfter(&free_slab);
	nr_free_slab++;
}

MetaSlab *MetaSlab::AllocSlab(int obj_size)
{
	Page *pg = alloc->AllocPage();
	MetaSlab *slab = (MetaSlab *) PADDR_TO_KPTR(pg->physical_address());
	pg->slab_ptr = slab;
	pg->slab_obj_size = obj_size;
	return slab;
}

void MetaSlab::FreeSlab(MetaSlab *slab)
{
	Page *pg = alloc->page(KPTR_TO_PADDR(slab));
	alloc->FreePage(pg);
}

static MemCache<MetaSlab, 64> meta_slab_cache;

BaseSlab *BaseSlab::AllocSlab(int obj_size)
{
	BaseSlab *slab = (BaseSlab *) meta_slab_cache.Allocate();
	Page *pg = slab->data_page = alloc->AllocPage();
	pg->slab_ptr = slab;
	pg->slab_obj_size = obj_size;
	return slab;
}

void BaseSlab::FreeSlab(BaseSlab *slab)
{
	meta_slab_cache.Free(slab);
}

void InitSlab()
{
	meta_slab_cache.Init(0);
	// TODO: buddy allocation cache

	// testing...
	void *p[128];
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < 128; i++) {
			p[i] = meta_slab_cache.Allocate();
		}
		for (int i = 64; i < 128; i++) {
			meta_slab_cache.Free(p[i]);
		}
		for (int i = 64; i < 72; i++) {
			p[i] = meta_slab_cache.Allocate();
		}
		for (int i = 0; i < 72; i++) {
			meta_slab_cache.Free(p[i]);
		}
	}

	meta_slab_cache.PrintStat();
}

}
