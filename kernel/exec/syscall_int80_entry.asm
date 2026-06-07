;
; syscall_int80_entry
; As part of the Avery project
; Created by Max Van den Eynde in 2026
; --------------------------------------
; Description: Entry for int 0x80 syscall
; Copyright (c) 2026 Max Van den Eynde
;

bits 64

global syscall_int80_entry
extern syscall_handler

section .text

syscall_int80_entry:
	push rbp
	mov rbp, rsp

	push rbx
    push r12
    push r13
    push r14
    push r15

    push r9             ; 7th C++ argument: arg5

    mov r9, r8          ; C++ arg5 = syscall arg4
    mov r8, r10         ; C++ arg4 = syscall arg3
    mov rcx, rdx        ; C++ arg3 = syscall arg2
    mov rdx, rsi        ; C++ arg2 = syscall arg1
    mov rsi, rdi        ; C++ arg1 = syscall arg0
    mov rdi, rax        ; C++ arg0 = syscall number

    call syscall_handler

    add rsp, 8

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    iretq