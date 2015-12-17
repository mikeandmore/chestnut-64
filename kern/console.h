// -*- c++ -*-
#ifndef CONSOLE_H
#define CONSOLE_H

#include "terminal.h"
#include "libc/stdarg.h"

namespace kernel {
class Console
{
public:
  Console();
  Console(const Console &rhs) = delete;

  void putchar(char ch);
  void vprintf(const char *fmt, va_list ap);
  void printf(const char *fmt, ...);
private:
  int x;
  int y;
  char vga_buffer[Terminal::kVGAHeight][Terminal::kVGAWidth];
};

}

#endif /* CONSOLE_H */
