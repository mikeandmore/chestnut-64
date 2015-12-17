#include "libc/common.h"
#include "mm/allocator.h"

__link void __cxa_pure_virtual()
{
  panic("Runtime Error: pure C++ virtual function called!");
}

// placement new
void *operator new(size_t, void *p)     throw() { return p; }
void *operator new[](size_t, void *p)   throw() { return p; }
void  operator delete  (void *, void *) throw() { };
void  operator delete[](void *, void *) throw() { };


void *operator new(size_t size) { return kernel::Alloc(size); }
void *operator new[](size_t size) { return kernel::Alloc(size); }
void  operator delete(void *p) throw() { kernel::Free(p); }
void  operator delete[](void *p) throw() { kernel::Free(p); }
