#ifndef COMMON_H
#define COMMON_H

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef uint8 u8;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef unsigned long size_t;
typedef uint64 uintptr_t;

typedef unsigned long ulong;

#define NULL 0

#define __no_inline __attribute((noinline))
#define __no_return __attribute((noreturn))

#ifdef __cplusplus
#define __link extern "C"
#else
#define __link
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define halt() asm("hlt")

/* telling the compiler not to re-order the instructions */
static inline void soft_barrier()
{
  asm volatile("" : : : "memory");
}

static inline void lfence_barrier()
{
  asm volatile("lfence" : : : "memory");
}

void kprintf(const char *fmt, ...);

#define panic(fmt, ...)                         \
  do {                                          \
    kprintf("PANIC: " fmt, ##__VA_ARGS__);      \
    asm("cli");                                 \
    halt();                                     \
  } while (0)                                   \

#define kassert(expr)                           \
  do {                                          \
    if (!(expr))                                \
      panic("Assertion Error on " #expr);	\
  } while (0)                                   \

#ifdef __cplusplus

void *operator new(size_t, void *p)     throw();
void *operator new[](size_t, void *p)   throw();
void  operator delete  (void *, void *) throw();
void  operator delete[](void *, void *) throw();

void *operator new(size_t size);
void *operator new[](size_t size);
void  operator delete(void *p) throw();
void  operator delete[](void *p) throw();

template <typename T>
T&& move(T& o)
{
  return (T&&) o;
}

// getting global variable
// each module use specialization to export global variable
template <typename T>
T& GlobalInstance();

template <typename T>
void InitializeGlobal()
{
  T& ins = GlobalInstance<T>();
  new(&ins) T();
}

template <typename T1, typename T2, typename ...Targs>
void InitializeGlobal()
{
  InitializeGlobal<T1>();
  InitializeGlobal<T2, Targs...>();
}

#endif

#endif /* COMMON_H */
