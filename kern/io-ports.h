// mode: c++
#ifndef IO_PORTS_H
#define IO_PORTS_H

#include "libc/common.h"

namespace kernel {

class IoPorts {
public:
  inline static void OutB(u16 port, u8 value) {
    asm volatile("outb %b0,%w1":: "a"(value), "d"(port));
  }

  inline static void OutW(u16 port, u16 value) {
    asm volatile("outw %w0,%w1":: "a"(value), "d"(port));
  }

  inline static void OutDW(u16 port, u32 value) {
    asm volatile("outl %0,%w1":: "a"(value), "Nd"(port));
  }

  inline static u8 InB(u16 port) {
    u8 value;
    asm volatile("inb %w1, %b0": "=a"(value): "d"(port));
    return value;
  }

  inline static u16 InW(u16 port) {
    u16 value;
    asm volatile("inw %w1, %w0": "=a"(value): "d"(port));
    return value;
  }

  inline static u32 InDW(u16 port) {
    u32 value;
    asm volatile("inl %w1, %0": "=a"(value): "Nd"(port));
    return value;
  }

  inline static u8 PciReadB(u8 bus, u8 slot, u8 func, u8 offset) {
    PciWriteAddr(bus, slot, func, offset);
    return (u8)((InDW(kPciDataPort) >> ((offset & 3) * 8)) & 0xff);
  }

  inline static u16 PciReadW(u8 bus, u8 slot, u8 func, u8 offset) {
    kassert((offset & 1) == 0); // aligned check
    PciWriteAddr(bus, slot, func, offset);
    return (u16)((InDW(kPciDataPort) >> ((offset & 2) * 8)) & 0xffff);
  }

  inline static u32 PciReadDW(u8 bus, u8 slot, u8 func, u8 offset) {
    kassert((offset & 3) == 0); // aligned check
    PciWriteAddr(bus, slot, func, offset);
    return (u32)InDW(kPciDataPort);
  }

  inline static void PciWriteB(u8 bus, u8 slot, u8 func, u8 offset, u8 value) {
    PciWriteAddr(bus, slot, func, offset);
    OutB(kPciDataPort + (offset & 3), value);
  }

  inline static void PciWriteW(u8 bus, u8 slot, u8 func, u8 offset, u16 value) {
    kassert((offset & 1) == 0); // aligned check
    PciWriteAddr(bus, slot, func, offset);
    OutW(kPciDataPort + (offset & 2), value);
  }

  inline static void PciWriteDW(u8 bus, u8 slot, u8 func, u8 offset, u32 value) {
    kassert((offset & 3) == 0); // aligned check
    PciWriteAddr(bus, slot, func, offset);
    OutDW(kPciDataPort, value);
  }
private:
  static const u32 kPciAddressPort = 0xCF8;
  static const u32 kPciDataPort = 0xCFC;

  static void PciWriteAddr(u8 bus, u8 slot, u8 func, u8 offset) {
    kassert(slot < 32);
    kassert(func < 8);
    u32 addr = (u32)(((u32)bus << 16) | ((u32)slot << 11) |
                     ((u32)func << 8) | (offset & 0xfc) | ((u32)0x80000000));
    OutDW(kPciAddressPort, addr);
  }
};

}

#endif /* IO_PORTS_H */
