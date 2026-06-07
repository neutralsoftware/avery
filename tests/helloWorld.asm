;
; helloWorld
; As part of the Avery project
; Created by Max Van den Eynde in 2026
; --------------------------------------
; Description: Simple test to try syscalls
; Copyright (c) 2026 Max Van den Eynde
;

bits 64
global _start

section .text
_start:
	mov rax, 1
	mov rdi, 1
	mov rsi, message
	mov rdx, message_len
	int 0x80

	mov rax, 0
	xor rdi, rdi
	int 0x80

section .rodata
message:
	db "Hello from usermode Avery!", 10
message_len equ $ - message