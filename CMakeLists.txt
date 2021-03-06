cmake_minimum_required(VERSION 3.4)

project(chestnut-64)

# set(CMAKE_C_COMPILER "clang")
# set(CMAKE_CXX_COMPILER "clang++")

set(CMAKE_C_FLAGS "-ffreestanding -fno-builtin -nostdlib -nostdinc -mcmodel=large -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -fno-exceptions -fno-rtti")
set(CMAKE_CXX_LINK_EXECUTABLE "ld -T ${PROJECT_SOURCE_DIR}/linker.ld -nodefaultlibs -melf_x86_64 -z max-page-size=0x1000 <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

enable_language(ASM_NASM)
add_definitions("-D__KERNEL__ -D__CHESTNUT__")

include_directories("kern/" "kern/acpica/include")

# file(GLOB_RECURSE ACPICA_SOURCE "kern/acpica/*.c")
# add_library(acpica ${ACPICA_SOURCE})

add_executable(kernel.bin
        kern/boot.asm
        kern/smp-boot.asm
        kern/main.cc
        kern/terminal.cc
        kern/console.cc
        kern/page-table.cc
        kern/kvm-clock.cc
        kern/apic.cc
        kern/acpi.cc
        kern/cpu.cc
        kern/irqs.cc
        kern/mm/allocator.cc
        kern/mm/uslab.cc
        kern/libc/string.c
        kern/libc/vector.cc
        kern/libc/cxxrt.cc
        )

target_link_libraries(kernel.bin)

add_custom_target(boot.iso
                 "${PROJECT_SOURCE_DIR}/gen-iso.sh" "${PROJECT_BINARY_DIR}/kernel.bin")
add_dependencies(boot.iso kernel.bin)
