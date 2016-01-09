use32

[global _boot]
[global GDT32Pointer]
[global GDT64Pointer]
[global GDT64VPointer]

[extern kernel_main]

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
        dw 5
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
        cmp eax, 0x36d76289  ; if this matches, we're good
        jne $
        mov ecx, ebx         ; save a copy in case we need it later
        mov edx, [ebx]       ; size of multiboot structure
        add edx, ebx         ; maximum address for multiboot structure
        add ebx, 8           ; total_size and reserved are both u32
.find_mem_map:
        cmp ebx, edx         ; if equal, we don't know how much memory
   	jge $
	cmp DWORD [ebx], 6   ; check if we've found the memory map
	je .found_mem_map
	mov eax, [ebx + 4]
	add eax, 7
	and eax, ~7
	add ebx, eax
	jmp .find_mem_map
.found_mem_map:         ; yay, memory map found :)
        mov [MemMap], ebx    ; found memory map, so store location for parsing

.load_gdt1:
        lgdt [GDT32Pointer]
        jmp 0x08:.GdtReady

.GdtReady:
	mov eax, 0x10
	mov ds, ax
	mov ss, ax
	mov esp, _boot_stack

	call setup_paging_and_long_mode

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

	; mov [gs:0x30], dword 0

        mov rax, MemMap + 0xFFFFFF0000000000
	mov rdi, [rax]

        mov rax, kernel_main
	call rax

use32
[extern Pml4]
[extern Pdpt]
[extern Pd]

setup_paging_and_long_mode:
   	mov eax, Pdpt
	or eax, 1

       	mov dword [Pml4], eax
        mov dword [Pml4 + 0x04], 0

        mov dword [Pml4 + 0xFF0], eax
        mov dword [Pml4 + 0xFF4], 0

        mov dword [Pdpt], 0x0083
        mov dword [Pdpt + 0x04], 0

	;; mov eax, Pd
	;; or eax, 1
	;; mov [Pdpt], eax
	;; mov [Pdpt + 0xFF0], eax

        ;; kernel can only do 8M direct access of physical memory
	;; mov dword [Pd], 0x000083
	;; mov dword [Pd + 8], 0x200083
	;; mov dword [Pd + 16], 0x400083
	;; mov dword [Pd + 24], 0x600083

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

MemMap:
        dd 0
