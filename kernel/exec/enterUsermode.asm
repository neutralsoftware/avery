;
; enterUsermode
; As part of the Avery project
; Created by Max Van den Eynde in 2026
; --------------------------------------
; Description: Usermode entrance
; Copyright (c) 2026 Max Van den Eynde
;

bits 64

global enter_usermode_asm

section .text

enter_usermode_asm:
	cli

	mov ax, 0x23
	mov ds, ax
	mov es, ax

	push qword 0x23
	push rsi

	pushfq
	pop rax
	or rax, 0x200
	push rax

	push qword 0x1B
	push rdi

	iretq