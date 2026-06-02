;
; gdt_flush.asm
; Created by Maxims Enterprise
; in 2026 as part of the Avery project
;

bits 64

global gdt_flush
extern gp

gdt_flush:
	lgdt [rel gp]

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov ss, ax

	push qword 0x08
	lea rax, [rel .flush]
	push rax
	retfq

.flush:
	ret