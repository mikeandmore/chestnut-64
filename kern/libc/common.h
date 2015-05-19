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

typedef uint64 size_t;
typedef uint64 uintptr_t;

#define NULL 0

#define __link extern "C"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define halt() asm("hlt")

#define panic(fmt, ...)							\
	do {								\
		kernel::console->printf("PANIC: " fmt, ##__VA_ARGS__);	\
		asm("cli");						\
		halt();							\
	} while (0)							\

#define kassert(expr)						\
	do {							\
		if (!(expr))					\
			panic("Assertion Error on " #expr);	\
	} while (0)						\

#endif /* COMMON_H */
