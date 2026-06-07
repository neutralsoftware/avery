;
; simpleExit.asm
; Created by Max Van den Eynde
; in 2026 as part of the Avery project
;

bits 64
global _start

section .text
_start:
	mov rax, 0
	mov rdi, 42
	int 0x80