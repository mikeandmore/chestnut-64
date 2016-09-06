use32

[global _boot]
[global GDT32Pointer]
[global GDT64Pointer]
[global GDT64VPointer]
[global IDT64]

[extern KernelMain]

[section .mbhdr]
[extern _load_start]
[extern _load_end]
[extern _bss_end]

align 8

MbHdr:
        dd 0xE85250D6
        dd 0
        dd HdrEnd - MbHdr
        dq -(0xE85250D6 + 0 + (HdrEnd - MbHdr))

align 8

AddrTag:
   	dw 2
	dw 0
	dd AddrTagEnd - AddrTag
	dd MbHdr
	dd 0
	dd 0
	dd 0
AddrTagEnd:

align 8

MbiTag:
	dw 1
	dw 0
	dd MbiTagEnd - MbiTag
	dd 6
MbiTagEnd:

align 8

EntryTag:
        dw 3
        dw 0
        dd EntryTagEnd - EntryTag
        dd _boot
EntryTagEnd:

align 8

FlagsTag:
        dw 4
        dw 0
        dd FlagsTagEnd - FlagsTag
        dd (1 << 0)
FlagsTagEnd:

align 8

FrmBufTag:
        dw 3
        dw 0
        dd FrmBufTagEnd - FrmBufTag
        dd 80
        dd 25
        dd 0
FrmBufTagEnd:

align 8

EndTags:
        dw 0
        dw 0
        dd EndTagsEnd - EndTags
EndTagsEnd:
HdrEnd:

[section .boot]
[extern _boot_stack]
_boot:
        cmp eax, 0x36d76289     ; check the magic returned by the bootloader
        jne $
        mov [MBI], ebx       ; save the pointer

.load_gdt1:
        lgdt [GDT32Pointer]
        jmp 0x08:.GdtReady

.GdtReady:
	mov eax, 0x10
	mov ds, ax
	mov ss, ax
	mov esp, _boot_stack

	call SetupPagingAndLongMode

	; mov eax, Gdtr2
	lgdt [GDT64Pointer]
	jmp 0x08:.Gdt2Ready

use64

.Gdt2Ready:
        xor ax, ax
	mov gs, ax

   	;; mov eax, 0x10
        mov ds, ax
	mov es, ax
	mov ss, ax

	mov rsp, _boot_stack + 0xFFFFFF0000000000

	mov rax, GDT64VPointer + 0xFFFFFF0000000000
        lgdt [rax]

	mov rax, cr0
	bts rax, 1		; Set Monitor co-processor (Bit 1)
	btr rax, 2		; Clear Emulation (Bit 2)
	mov cr0, rax

	;;  Enable SSE
	mov rax, cr4
	bts rax, 9		; FXSAVE and FXSTOR instructions (Bit 9)
	bts rax, 10		; SIMD Floating-Point Exceptions (Bit 10)
	mov cr4, rax

	fldcw [SSEControl] ; writes 0x37f into the control word: the value written by F(N)INIT
	fldcw [SSEControl + 2] ; writes 0x37e, the default with invalid operand exceptions enabled
	fldcw [SSEControl + 4] ; writes 0x37a, both division by zero and invalid operands cause exceptions.

	;;  Enable Math Co-processor
	finit

	; mov [gs:0x30], dword 0

        mov rax, MBI + 0xFFFFFF0000000000
	mov rdi, [rax]

        mov rax, KernelMain
	call rax

use32
[extern Pml4]
[extern Pdpt]
[extern Pd]

SetupPagingAndLongMode:
   	mov eax, Pdpt
	or eax, 1

       	mov dword [Pml4], eax
        mov dword [Pml4 + 0x04], 0

        mov dword [Pml4 + 0xFF0], eax
        mov dword [Pml4 + 0xFF4], 0

        ;; mov dword [Pdpt], 0x00000083
        ;; mov dword [Pdpt + 0x04], 0

	mov eax, Pd
	or eax, 1

	mov dword [Pdpt], eax
        mov dword [Pdpt + 0x04], 0

	mov dword [Pdpt + 0xFF0], eax
        mov dword [Pdpt + 0xFF4], 0

        ;; kernel can only do 8M direct access of physical memory
	mov dword [Pd], 0x000083
        mov dword [Pd + 0x04], 0

	mov dword [Pd + 0x08], 0x200083
        mov dword [Pd + 0x0c], 0

	mov dword [Pd + 0x10], 0x400083
        mov dword [Pd + 0x14], 0

	mov dword [Pd + 0x18], 0x600083
        mov dword [Pd + 0x1c], 0

	mov eax, Pml4
	mov cr3, eax

	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	ret

GDT32:
        dw 0         ; Limit (low).
        dw 0         ; Base (low).
        db 0         ; Base (middle)
        db 0         ; Access.
        db 0         ; Granularity.
        db 0         ; Base (high).

        dw 0xFFFF    ; Limit (low)
        dw 0         ; Base (low)
        db 0         ; Base (middle)
        db 10011010b ; Access
        db 11001111b ; Granularity
        db 0         ; Base (high)

        dw 0xFFFF    ; Limit (low).
        dw 0         ; Base (low).
        db 0         ; Base (middle)
        db 10010010b ; Access (read/write).
        db 11001111b ; Granularity.
        db 0         ; Base (high).
.EndGDT32:

GDT64:
        dw 0         ; Limit (low).
        dw 0         ; Base (low).
        db 0         ; Base (middle)
        db 0         ; Access.
        db 0         ; Granularity.
        db 0         ; Base (high).

        dw 0         ; Limit (low).
        dw 0         ; Base (low).
        db 0         ; Base (middle)
        db 10011010b ; Access (exec/read).
        db 10100000b ; Granularity.
        db 0         ; Base (high).

        dw 0         ; Limit (low).
        dw 0         ; Base (low).
        db 0         ; Base (middle)
        db 10010010b ; Access (read/write).
        db 10100000b ; Granularity.
        db 0         ; Base (high).
.EndGDT64:

GDT32Pointer:
        dw GDT32.EndGDT32 - GDT32 - 1
        dd GDT32

GDT64Pointer:
        dw GDT64.EndGDT64 - GDT64 - 1
        dq GDT64

GDT64VPointer:
        dw GDT64.EndGDT64 - GDT64 - 1
        dq GDT64 + 0xFFFFFF0000000000

IDT64:
        dw 4095
        dq 0xFFFFFF0000000000

SSEControl:
        dw 0x037F
        dw 0x037E
        dw 0x037A

MBI:
        dd 0
