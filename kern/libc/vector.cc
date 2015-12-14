#include "libc/vector.h"
#include "libc/string.h"
#include "mm/allocator.h"

void BaseVector::ResetPtr(size_t new_capacity, size_t ele_size)
{
  void *old_ptr = ptr;
  ptr = kernel::Alloc(new_capacity * ele_size);
  memcpy(ptr, old_ptr, len * ele_size);
  kernel::Free(old_ptr);
  capacity = new_capacity;
}

void BaseVector::CopyFrom(const BaseVector &src, size_t ele_size)
{
  kernel::Free(ptr);
  len = src.len;
  capacity = src.capacity;
  ptr = kernel::Alloc(ele_size * capacity);
  memcpy(ptr, src.ptr, ele_size * len);
}
