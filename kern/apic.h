#ifndef APIC_H
#define APIC_H

#include "libc/common.h"
#include "kvm-clock.h"

namespace kernel {

/**
 * List of available local apic registers
 */
enum class LocalApicRegister {
  ID                              = 0x0020,  // Local APIC ID
  VERSION                         = 0x0030,  // Local APIC Version
  TASK_PRIORITY                   = 0x0080,  // Task Priority
  ARBITRATION_PRIORITY            = 0x0090,  // Arbitration Priority
  PROCESSOR_PRIORITY              = 0x00a0,  // Processor Priority
  EOI                             = 0x00b0,  // EOI
  RRD                             = 0x00c0,  // Remote Read
  LOGICAL_DESTINATION             = 0x00d0,  // Logical Destination
  DESTINATION_FORMAT              = 0x00e0,  // Destination Format
  SPURIOUS_INTERRUPT_VECTOR       = 0x00f0,  // Spurious Interrupt Vector
  ISR                             = 0x0100,  // In-Service (8 registers)
  TMR                             = 0x0180,  // Trigger Mode (8 registers)
  IRR                             = 0x0200,  // Interrupt Request (8 registers)
  ERROR_STATUS                    = 0x0280,  // Error Status
  INTERRUPT_COMMAND_LO            = 0x0300,  // Interrupt Command (writing here sends command)
  INTERRUPT_COMMAND_HI            = 0x0310,  // Interrupt Command [63:32]
  TIMER                           = 0x0320,  // LVT Timer
  THERMAL                         = 0x0330,  // LVT Thermal Sensor
  PERF                            = 0x0340,  // LVT Performance Counter
  LINT0                           = 0x0350,  // LVT LINT0
  LINT1                           = 0x0360,  // LVT LINT1
  ERROR                           = 0x0370,  // LVT Error
  TIMER_INITIAL_COUNT             = 0x0380,  // Initial Count (for Timer)
  TIMER_CURRENT_COUNT             = 0x0390,  // Current Count (for Timer)
  TIMER_DIVIDE_CONFIG             = 0x03e0   // Divide Configuration (for Timer)
};

enum class IoApicAccessorRegister {
  IOREGSEL    = 0x00,  // Register selector
  IOWIN       = 0x10   // Register data
};

enum class IoApicRegister {
  ID          = 0x00,  // IO APIC ID
  VER         = 0x01,  // IO APIC Version / Max Redirection Entry
  ARB         = 0x02,  // APIC Arbitration ID
  REDTBL      = 0x10   // Redirection Entries
};

/**
 * Local apic command object
 */
class LocalApicInterruptCommand {
friend class LocalApicRegisterAccessor;
public:
  enum class DeliveryMode {
    FIXED = 0,
    LOWEST_PRIORITY = 1,
    SMI = 2,
    NMI = 4,
    INIT = 5,
    STARTUP = 6
  };

  enum class DestinationMode {
    PHYSICAL = 0,
    LOGICAL = 1
  };

  enum class DeliveryStatus {
    IDLE = 0,
    PENDING = 1
  };

  enum class DestinationShorthand {
    NONE = 0,
    SELF = 1,
    ALL = 2,
    ALL_EXCEPT_SELF = 3
  };

  LocalApicInterruptCommand(u8 vector_num,
                            DeliveryMode dmode,
                            DestinationMode destmode,
                            bool is_no_deassert,
                            bool trigger_mode,
                            DestinationShorthand dshort,
                            u8 dest)
    : hi(dest << 24),
      lo(vector_num |
         (static_cast<u32>(dmode) << 8) |
         (static_cast<u32>(destmode) << 11) |
         (static_cast<u32>(is_no_deassert) << 14) |
         (static_cast<u32>(trigger_mode) << 15) |
         (static_cast<u32>(dshort) << 18)) {}
private:
  u32 hi;
  u32 lo;
};

/**
 * Provides access to local apic registers
 */
class LocalApicRegisterAccessor {
public:
  LocalApicRegisterAccessor(void* local_base) :
    local_apic_base(static_cast<u8*>(local_base)) {
    kassert(local_apic_base);
  }

  u32 Read(LocalApicRegister reg) {
    return *(volatile u32*)(local_apic_base + static_cast<u32>(reg));
  }

  void Write(LocalApicRegister reg, u32 value) {
    *(volatile u32*)(local_apic_base + static_cast<u32>(reg)) = value;
  }

  inline void InterruptCommand(LocalApicInterruptCommand command) {
    Write(LocalApicRegister::INTERRUPT_COMMAND_HI, command.hi);
    Write(LocalApicRegister::INTERRUPT_COMMAND_LO, command.lo);
  }

private:
  u8* local_apic_base;
};

class LocalApic {
public:
  LocalApic(void* local_apic_address);

  /**
   * Send INIT command
   */
  void SendApicInit(u8 apicid) {
    registers.InterruptCommand(
      LocalApicInterruptCommand(
        0,
        LocalApicInterruptCommand::DeliveryMode::INIT,
        LocalApicInterruptCommand::DestinationMode::PHYSICAL,
        true,
        false,
        LocalApicInterruptCommand::DestinationShorthand::NONE,
        apicid
        ));
  }

  /**
   * Send STARTUP command
   */
  void SendApicStartup(u8 apicid, u8 vector_num) {
    registers.InterruptCommand(
      LocalApicInterruptCommand(
        vector_num,
        LocalApicInterruptCommand::DeliveryMode::STARTUP,
        LocalApicInterruptCommand::DestinationMode::PHYSICAL,
        true,
        false,
        LocalApicInterruptCommand::DestinationShorthand::NONE,
        apicid
        ));
  }

  /**
   * Set EOI (End Of Interrupt) flag. Interrupt handler must
   * do it before IRETQ
   * TODO: seems KVM have a faster EOI()?
   */
  void EOI() {
    registers.Write(LocalApicRegister::EOI, 0);
  }

  /**
   * Read Local Apic ID
   */
  u32 Id() {
    return registers.Read(LocalApicRegister::ID);
  }

  u32 bus_frequency() const { return bus_freq; }

  void CpuSetAPICBase(uintptr_t apic);
  uintptr_t CpuGetAPICBase();

  void InitCpu(KvmSystemTime *systime);
private:
  static const int kApicBaseMSR = 0x1b;
  static const int kApicBaseMSREnable = 0x800;

  void* local_apic_address;
  LocalApicRegisterAccessor registers;
  u32 bus_freq;
  ~LocalApic() = delete;
};

class IoApicRegistersAccessor {
public:
  explicit IoApicRegistersAccessor(uintptr_t address)
    :	addr(address) {
    kassert(addr);
  }

  inline u32 Read(IoApicRegister reg) const {
    kassert(addr);
    *(volatile u32*)(addr + (u8) IoApicAccessorRegister::IOREGSEL)
      = (u32) reg;

    return *(volatile u32*)(addr + (u8) IoApicAccessorRegister::IOWIN);
  }

  inline void Write(IoApicRegister reg, u8 reg_offset, u8 value) const {
    kassert(addr);
    *(volatile u32*)(addr + (u8) IoApicAccessorRegister::IOREGSEL)
      = (u32) reg + reg_offset;

    *(volatile u32*)(addr + (u8) IoApicAccessorRegister::IOWIN) = value;
  }

  inline void SetEntry(u8 index, u64 data) const {
    Write(IoApicRegister::REDTBL, index * 2, (u32) data);
    Write(IoApicRegister::REDTBL, index * 2 + 1, (u32) (data >> 32));
  }
private:
  uintptr_t addr;
};

class IoApic {
public:
  IoApic(u32 id, uintptr_t address, u32 intr_base);
  void Init();

  void EnableIrq(u32 first_irq_offset, u32 irq) {
    // kprintf("Enabling IRQ %d %d\n", first_irq_offset, irq);
    registers.SetEntry(irq, first_irq_offset + irq + interrupt_base);
  }

  u32 interrupt_base_address() const { return interrupt_base; }

private:
  u32 id_;
  uintptr_t addr;
  u32 interrupt_base;
  IoApicRegistersAccessor registers;
};

}

#endif /* APIC_H */
