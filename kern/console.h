// -*- c++ -*-
#ifndef CONSOLE_H
#define CONSOLE_H

#include "terminal.h"
#include "io-ports.h"
#include "libc/stdarg.h"

namespace kernel {

class Console;

class ConsoleSource {
public:
  ConsoleSource() : next(nullptr) {}

  virtual void WriteByte(char ch) = 0;
private:
friend Console;
  ConsoleSource *next; // chain of console sources
};

class VGABuffer : public ConsoleSource {
public:
  VGABuffer();
  VGABuffer(const VGABuffer &rhs) = delete;

  virtual void WriteByte(char ch);
private:
  int x;
  int y;
  char vga_buffer[Terminal::kVGAHeight][Terminal::kVGAWidth];
};

class SerialPort : public ConsoleSource {
public:
  SerialPort(int p);
  SerialPort() : SerialPort(0x03F8) {} // COM1

  /*
  int is_transmit_empty() {
    return IoPorts::InB(port + 5) & 0x20;
  }
  */

  virtual void WriteByte(char a) {
    // FIXME:
    // It seems KVM doesn't support polling with serial port. (KVM will hang on
    // ``inb`` instruction. Don't know why.)
    //
    // It's a para-virt device anyway, let's push this into the buffer.
    //
    // while (is_transmit_empty() == 0);
    IoPorts::OutB(port, a);
  }
private:
  int port;
};

class Console {
  ConsoleSource *sources;
public:
  Console();

  void AddSource(ConsoleSource *src) {
    src->next = sources;
    sources = src;
  }

  void WriteByte(char a) {
    for (ConsoleSource *s = sources; s != nullptr; s = s->next) {
      s->WriteByte(a);
    }
  }

  void vprintf(const char *fmt, va_list ap);
  void printf(const char *fmt, ...);
};

}

#endif /* CONSOLE_H */
