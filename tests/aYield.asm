;
; aYield
; As part of the Avery project
; Created by Max Van den Eynde in 2026
; --------------------------------------
; Description: 
; Copyright (c) 2026 Max Van den Eynde
;

bits 64
global _start

section .text
_start:
.loop:
	mov rax, 1
	mov rdi, 1
	mov rsi, msg
	mov rdx, msg_len
	int 0x80

	jmp .loop

section .rodata
msg:
	db "A", 10
msg_len equ $ - msg