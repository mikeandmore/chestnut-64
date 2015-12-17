// -*- c++ -*-

#ifndef VECTOR_H
#define VECTOR_H

#include "libc/common.h"
#include "libc/string.h"
#include "console.h"

class BaseVector {
protected:
  size_t capacity;
  size_t len;
  void* ptr;

  BaseVector(size_t length, void *data)
    : len(length), ptr(data), capacity(length) {}

  void CopyFrom(const BaseVector &src, size_t ele_size);
  void Compact(size_t ele_size) {
    ResetPtr(len, ele_size);
  }
  void Extend(size_t ele_size) {
    if (capacity == 0) {
      ResetPtr(16, ele_size);
    } else {
      ResetPtr(capacity * 2, ele_size);
    }
  }
  void EnsureSpace(size_t idx, size_t ele_size) {
    if (idx >= capacity) Extend(ele_size);
  }
private:
  void ResetPtr(size_t new_capacity, size_t ele_size);
public:
  size_t size() const { return len; }
};

template <typename T>
class Vector : public BaseVector {
public:

  typedef T* Iterator;

  Vector() : BaseVector(0, nullptr) {}

  Vector(Vector &&rhs) : BaseVector(rhs.len, rhs.ptr) {
    rhs.ptr = nullptr;
    rhs.len = 0;
  }
  Vector(const Vector &rhs) = delete; // disabled, call CopyFrom()

  T& operator[](size_t idx) {
    if (idx >= len) panic("Vector access overflow: %d/%d", idx, len);
    return static_cast<T*>(ptr)[idx];
  }
  const T& operator[](size_t idx) const {
    if (idx >= len) panic("Vector access overflow: %d/%d", idx, len);
    return static_cast<const T*>(ptr)[idx];
  }

  Iterator begin() { return static_cast<T*>(ptr); }
  const Iterator begin() const { return static_cast<const T*>(ptr); }
  Iterator end() { return static_cast<T*>(ptr) + len; }
  const Iterator end() const { return static_cast<const T*>(ptr) + len; }

  void Insert(size_t pos, T &&xref) {
    EnsureSpace(len, sizeof(T));
    T* arr = static_cast<T*>(ptr);
    memmove(arr + pos + 1, arr + pos, (len - pos) * sizeof(T));
    new(&arr[pos]) T(xref);
    len++;
  }
  void Remove(size_t pos) {
    T* arr = static_cast<T*>(ptr);
    arr[pos].~T();
    memmove(arr + pos, arr + pos + 1, (len - pos - 1) * sizeof(T));
    len--;
  }
  void PushBack(T &&xref) {
    Insert(len, (T&&) xref);
  }
  void PopBack() {
    Remove(len);
  }
};

#endif /* VECTOR_H */
