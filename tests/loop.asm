;
; loop
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
	xor rax, rax

.loop:
	inc rax
	jmp .loop