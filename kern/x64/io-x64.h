// Copyright 2014 Runtime.JS project authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef IO_X64_H
#define IO_X64_H

#include "libc/common.h"

namespace kernel {

/**
 * Functions to read and write IO ports
 */
class IoPortsX64 {
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

/**
 * Serial port output
 */
class SerialPortX64 {
public:
  SerialPortX64() {
    port_ = 0x3F8;			  // COM1
    IoPortsX64::OutB(port_ + 1, 0x00);    // Disable all interrupts
    IoPortsX64::OutB(port_ + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    IoPortsX64::OutB(port_ + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    IoPortsX64::OutB(port_ + 1, 0x00);    //                  (hi byte)
    IoPortsX64::OutB(port_ + 3, 0x03);    // 8 bits, no parity, one stop bit
    IoPortsX64::OutB(port_ + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    IoPortsX64::OutB(port_ + 4, 0x0B);    // IRQs enabled, RTS/DSR set
  }

  void WriteByte(char a) {
    write_serial(a);
  }

  int is_transmit_empty() {
    return IoPortsX64::InB(port_ + 5) & 0x20;
  }

  void write_serial(char a) {
    // FIXME:
    // It seems KVM doesn't support polling with serial port. (KVM will hang on
    // ``inb`` instruction. Don't know why.)
    //
    // It's a para-virt device anyway, let's push this into the buffer.
    //
    // while (is_transmit_empty() == 0);
    IoPortsX64::OutB(port_, a);
  }
private:
  int port_;
};

}

#endif /* IO_X64_H */
