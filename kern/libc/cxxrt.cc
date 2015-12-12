#include "libc/common.h"
#include "console.h"

__link void __cxa_pure_virtual()
{
  panic("Runtime Error: pure C++ virtual function called!");
}
