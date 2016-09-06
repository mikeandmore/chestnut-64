#include "console.h"
#include "libc/common.h"
#include "libc/stdarg.h"
#include "libc/string.h"
#include "io-ports.h"

namespace kernel {

VGABuffer::VGABuffer()
  : ConsoleSource(), x(0), y(0)
{
  memset(vga_buffer, 0, Terminal::kVGAHeight * Terminal::kVGAWidth);
}

void VGABuffer::WriteByte(char ch)
{
  if (x >= Terminal::kVGAWidth) {
    x = 0;
    y += 1;
  }
  if (y >= Terminal::kVGAHeight) {
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
        Global<Terminal>().DrawChar(vga_buffer[i][j], j, i);
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
  Global<Terminal>().DrawChar(ch, x, y);
  vga_buffer[y][x] = ch;
  x += 1;
}

SerialPort::SerialPort(int p)
  : ConsoleSource()
{
  port = p;
  IoPorts::OutB(port + 1, 0x00);    // Disable all interrupts
  IoPorts::OutB(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
  IoPorts::OutB(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
  IoPorts::OutB(port + 1, 0x00);    //			(hi byte)
  IoPorts::OutB(port + 3, 0x03);    // 8 bits, no parity, one stop bit
  IoPorts::OutB(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
  IoPorts::OutB(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void SerialPort::EnableInterrupt(SerialPort::InterruptMode mode)
{
  u8 mask = 1 << ((u8) mode);
  u8 old = IoPorts::InB(port + 1);
  IoPorts::OutB(port + 1, old | mask);
}

void SerialPort::DisableInterrupt(SerialPort::InterruptMode mode)
{
  u8 mask = 1 << ((u8) mode);
  u8 old = IoPorts::InB(port + 1);
  IoPorts::OutB(port + 1, old & ~mask);
}

Console::Console()
{
  sources = nullptr;
}

void Console::vprintf(const char *fmt, va_list ap)
{
  const char *hex = "0123456789abcdef";
  char buf[32], *s;
  unsigned long long u;
  int c, l;

  while ((c = *fmt++) != '\0') {
    if (c != '%') {
      WriteByte(c);
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
      WriteByte(va_arg(ap, int));
      break;
    case 's':
      for (s = va_arg(ap, char *); *s != '\0'; s++)
        WriteByte(*s);
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
        WriteByte(*s);
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
kernel::Console &Global()
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
