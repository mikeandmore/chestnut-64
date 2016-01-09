#include "console.h"
#include "libc/common.h"
#include "libc/stdarg.h"
#include "libc/string.h"

#include "x64/io-x64.h"

namespace kernel {

Console::Console()
  : x(0), y(0)
{
  for (int i = 0; i < Terminal::kVGAHeight; i++) {
    for (int j = 0; j < Terminal::kVGAWidth; j++) {
      vga_buffer[i][j] = 0;
    }
  }
}

void Console::putchar(char ch)
{
  GlobalInstance<SerialPortX64>().WriteByte(ch);
  return;
  if (x >= Terminal::kVGAWidth) {
    x = 0;
    y += 1;
  }
  if (y >= Terminal::kVGAHeight) {
    //copy the screen and move up one line...
    for(int i = 0; i < Terminal::kVGAHeight - 1; i++ ) {
      for (int j = 0; j < Terminal::kVGAWidth; j++) {
        vga_buffer[i][j] = vga_buffer[i + 1][j];
      }
    }
    for (int j = 0; j < Terminal::kVGAWidth; j++) {
      vga_buffer[Terminal::kVGAHeight - 1][j] = 0;
    }

    for (int i = 0; i < Terminal::kVGAHeight; i++) {
      for (int j = 0; j < Terminal::kVGAWidth; j++) {
        GlobalInstance<Terminal>().DrawChar(vga_buffer[i][j], j, i);
      }
    }
    //end of move the screen one line up
    y = Terminal::kVGAHeight - 1;
  }
  if (ch == '\n') {
    x = 0;
    y++;
    return;
  }
  GlobalInstance<Terminal>().DrawChar(ch, x, y);
  vga_buffer[y][x] = ch;
  x += 1;
}

void Console::vprintf(const char *fmt, va_list ap)
{
  const char *hex = "0123456789abcdef";
  char buf[32], *s;
  unsigned long long u;
  int c, l;

  while ((c = *fmt++) != '\0') {
    if (c != '%') {
      putchar(c);
      continue;
    }
    l = 0;
  nextfmt:
    c = *fmt++;
    switch (c) {
    case 'l':
      l++;
      goto nextfmt;
    case 'c':
      putchar(va_arg(ap, int));
      break;
    case 's':
      for (s = va_arg(ap, char *); *s != '\0'; s++)
        putchar(*s);
      break;
    case 'd':	/* A lie, always prints unsigned */
    case 'u':
    case 'x':
      switch (l) {
      case 2:
        u = va_arg(ap, unsigned long long);
        break;
      case 1:
        u = va_arg(ap, unsigned long);
        break;
      default:
        u = va_arg(ap, unsigned int);
        break;
      }
      s = buf;
      if (c == 'd' || c == 'u') {
        do
          *s++ = '0' + (u % 10U);
        while (u /= 10);
      } else {
        do
          *s++ = hex[u & 0xfu];
        while (u >>= 4);
      }
      while (--s >= buf)
        putchar(*s);
      break;
    }
  }
}

void Console::printf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

}

static kernel::Console def_console;

template <>
kernel::Console &GlobalInstance()
{
  return def_console;
}

void kprintf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  def_console.vprintf(fmt, ap);
  va_end(ap);
}
