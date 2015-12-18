[section .smp_boot]

[global _smp_cpu_counter]
[global _smp_start]
[global _smp_boot_code_end]
[extern Pml4]
[extern kernel_main]
[extern Gdtr1]
[extern Gdtr2]
[extern Gdtr3]

use16

align 8

_smp_start:
        cli
        xor ax,ax
        mov ds,ax
        mov es,ax
        mov ss,ax
        mov fs,ax
        mov gs,ax

        lgdt [dword Gdtr1]

        jmp dword 0x08:_smp_start32

use32
_smp_start32:
        ;; page table
        mov eax, Pml4
        mov cr3, eax

        ;; ???
        mov eax, cr4
        or eax, 1 << 5
        mov cr4, eax

        ;; ???
        mov ecx, 0xC0000080
        rdmsr
        or eax, 1 << 8
        wrmsr

        ;; protection mode
        mov eax, cr0
        or eax, 1 << 31
        mov cr0, eax

	mov eax, 0x10
        mov ds, ax
        mov es, ax
        mov ss, ax
        xor eax, eax
        xor ebx, ebx
        xor ecx, ecx
        xor edx, edx
        xor esi, esi
        xor edi, edi
        xor ebp, ebp

        lgdt [Gdtr2]
        jmp 0x08:_smp_start64
use64
_smp_start64:
	mov rax, 0x10
        mov ds, ax
        mov es, ax
        mov ss, ax
        xor rax, rax
        xor rbx, rbx
        xor rcx, rcx
        xor rdx, rdx
        xor rsi, rsi
        xor rdi, rdi
        xor rbp, rbp

        ;; mov rsp, [StackTop]... or gs related?
        lgdt [Gdtr3]
        mov [gs:0x30], dword 0

        mov rdi, 0

        call kernel_main

        cli
        hlt


use32

align 16
_smp_cpu_counter:
	dw 0

_smp_boot_code_end:
        dd 0
