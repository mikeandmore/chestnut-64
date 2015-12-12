extern "C" {
#include "acpiosxf.h"
#include "acexcep.h"
#include "acpixf.h"
}

#include "console.h"
#include "mm/allocator.h"

extern "C" {

ACPI_STATUS AcpiOsInitialize()
{
  kernel::console->printf("ACPI Subsystem Initialize\n");
  return AE_OK;
}

ACPI_STATUS AcpiOsTerminate()
{
  kernel::console->printf("ACPI Terminated\n");
  return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer()
{
  ACPI_SIZE rsdp;
  auto st = AcpiFindRootPointer(&rsdp);
  if (ACPI_FAILURE(st)) {
    abort();
  }
  return rsdp;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal,
				     ACPI_STRING *NewVal)
{
  *NewVal = nullptr;
  return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable,
				ACPI_TABLE_HEADER **NewTable)
{
  *NewTable = nullptr;
  return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable,
					ACPI_PHYSICAL_ADDRESS *NewAddress,
					UINT32 *NewTableLength)
{
  *NewAddress = 0;
  *NewTableLength = 0;
  return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
  // TODO:
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
  // TODO:
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
  // TODO:
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
  // TODO:
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits,
				  UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle)
{
  // TODO:
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
  // TODO:
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle,
				UINT32 Units, UINT16 Timeout)
{
  // TODO:
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
  // TODO:
}

void *AcpiOsAllocate(ACPI_SIZE Size)
{
  return kernel::Alloc(Size);
}

void AcpiOsFree(void *Memory)
{
  kernel::Free(Memory);
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS Where, ACPI_SIZE Length)
{
  return PADDR_TO_KPTR(Where);
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Size)
{
  // Unmap is a no-op and leaves the mapppings in place because the amount of
  // mapped ACPI memory is limited, and we don't track whether what it maps
  // was previously mapped (so unmap can poke a hole where a previous mapping
  // existed, even before ACPI).
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress,
				     ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
  *PhysicalAddress = KPTR_TO_PADDR(LogicalAddress);
  return AE_OK;
}

}
