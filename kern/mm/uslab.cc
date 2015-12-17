// -*- c++ -*-
#include "libc/common.h"
#include "uslab.h"

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

int BaseBitmapSlab::AllocFromBitmapArray(int n, u64 *bitmap_array)
{
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
  return idx * n + row;
}

void BaseBitmapSlab::FreeToBitmapArray(int n, u64 *bitmap_array, int obj_idx)
{
  int row = obj_idx % n;
  int idx = obj_idx / n;
  bitmap_array[row] ^= (1 << idx);
}

void FreeListSlab::Init(int total)
{
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
  kassert(total == 64);
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
  kassert(nr_free_slab > 0);
  nr_free_slab--;
  int idx = ((u8 *) node - mem()) / 64;
  kassert(idx > 0 && idx < 64);
  return idx;
}

void MetaSlab::Free(int obj_idx)
{
  kassert(obj_idx > 0 && obj_idx < 64);
  ListNode *node = (ListNode *) (mem() + obj_idx * 64);
  node->InsertAfter(&free_slab);
  nr_free_slab++;
}

MetaSlab *MetaSlab::AllocSlab(int obj_size)
{
  auto &mem_pages = GlobalInstance<MemPages>();
  Page *pg = mem_pages.AllocPage();
  MetaSlab *slab = (MetaSlab *) PADDR_TO_KPTR(pg->physical_address());
  pg->slab_ptr = slab;
  pg->slab_obj_size = obj_size;
  return slab;
}

void MetaSlab::FreeSlab(MetaSlab *slab)
{
  auto &mem_pages = GlobalInstance<MemPages>();
  Page *pg = mem_pages.page(KPTR_TO_PADDR(slab));
  mem_pages.FreePage(pg);
}

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

    // kprintf("slab %x mem %x idx %d\n", slab, mem, idx);
    return mem + idx * ObjSize;
  }

  virtual void Free(void *ptr) {
    auto &mem_pages = GlobalInstance<MemPages>();
    // use the struct page to find its slab
    paddr addr = KPTR_TO_PADDR(ptr);
    Page *pg = mem_pages.page(addr);
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


static MemCache<MetaSlab, 64> meta_slab_cache;

static MemCache<BitmapSlab<4>, 16> chunk16_cache;
static MemCache<BitmapSlab<2>, 32> chunk32_cache;
static MemCache<BitmapSlab<1>, 64> chunk64_cache;
static MemCache<FreeListSlab, 128> chunk128_cache;
static MemCache<FreeListSlab, 256> chunk256_cache;
static MemCache<FreeListSlab, 512> chunk512_cache;
static MemCache<FreeListSlab, 1024> chunk1024_cache;
static MemCache<FreeListSlab, 2048> chunk2048_cache;

BaseSlab *BaseSlab::AllocSlab(int obj_size)
{
  auto &mem_pages = GlobalInstance<MemPages>();
  BaseSlab *slab = (BaseSlab *) meta_slab_cache.Allocate();
  Page *pg = slab->data_page = mem_pages.AllocPage();
  pg->slab_ptr = slab;
  pg->slab_obj_size = obj_size;
  return slab;
}

void BaseSlab::FreeSlab(BaseSlab *slab)
{
  meta_slab_cache.Free(slab);
}

MemCacheBase::MemCacheBase()
{
  stat.allocated = 0;
  slab_queue.full.InitHead();
  slab_queue.half.InitHead();
  slab_queue.empty.InitHead();
}

void InitSlab()
{
  // placement new all global caches
  new (&meta_slab_cache) MemCache<MetaSlab, 64>();

  new (&chunk16_cache) MemCache<BitmapSlab<4>, 16>();
  new (&chunk32_cache) MemCache<BitmapSlab<2>, 32>();
  new (&chunk64_cache) MemCache<BitmapSlab<1>, 64>();
  new (&chunk128_cache) MemCache<FreeListSlab, 128>;
  new (&chunk256_cache) MemCache<FreeListSlab, 256>;
  new (&chunk512_cache) MemCache<FreeListSlab, 512>;
  new (&chunk1024_cache) MemCache<FreeListSlab, 1024>;
  new (&chunk2048_cache) MemCache<FreeListSlab, 2048>;
  kprintf("Memory Allocator Initialized\n");
}

static MemCacheBase *kGlobalMemCache[] = {
  &chunk16_cache, &chunk32_cache, &chunk64_cache, &chunk128_cache,
  &chunk256_cache, &chunk512_cache, &chunk1024_cache, &chunk2048_cache
};

MemCacheBase *FitGlobalMemCache(int obj_size)
{
  //obj_size Byte
  int idx = 32 - __builtin_clz(obj_size - 1) - 4;
  // GlobalInstance<Console>().printf("idx = %d,  ", idx);
  if (idx < 0 )
    idx = 0;
  kassert(idx < 8);
  return kGlobalMemCache[idx];
}

void *Alloc(int obj_size)
{
  if (obj_size > 2048) {
    int nr_page = (obj_size - 1) / PAGESIZE + 1;
    auto pg = GlobalInstance<MemPages>().AllocPages(nr_page);
    pg->slab_obj_size = nr_page * PAGESIZE;
    return PADDR_TO_KPTR(pg->physical_address());
  }
  MemCacheBase *base = FitGlobalMemCache(obj_size);
  // kprintf("base %x\n", base);
  return base->Allocate();
}

void Free(void *ptr)
{
  if (ptr == nullptr) return;
  auto &mem_page = GlobalInstance<MemPages>();
  paddr addr = KPTR_TO_PADDR(ptr);
  Page *pg = mem_page.page(addr);
  // GlobalInstance<Console>().printf("page_size = %d\n", pg->slab_obj_size);
  if (pg->slab_obj_size > 2048) {
    mem_page.FreePages(pg, pg->slab_obj_size / PAGESIZE);
    return;
  }
  MemCacheBase *base = FitGlobalMemCache(pg->slab_obj_size);
  base->Free(ptr);
}

}
