;
; contextSwitch.asm
; As part of the Avery project
; Created by Max Van den Eynde in 2026
; --------------------------------------
; Description: Function to create the final context switch
; Copyright (c) 2026 Max Van den Eynde
;

bits 64

global switchContext

switchContext:
    mov [rdi + 0x00], r15
    mov [rdi + 0x08], r14
    mov [rdi + 0x10], r13
    mov [rdi + 0x18], r12
    mov [rdi + 0x20], rbx
    mov [rdi + 0x28], rbp
    mov [rdi + 0x30], rsp

    lea rax, [rel .return_point]
    mov [rdi + 0x38], rax

    mov r15, [rsi + 0x00]
    mov r14, [rsi + 0x08]
    mov r13, [rsi + 0x10]
    mov r12, [rsi + 0x18]
    mov rbx, [rsi + 0x20]
    mov rbp, [rsi + 0x28]
    mov rsp, [rsi + 0x30]

    mov rax, [rsi + 0x38]
    jmp rax

.return_point:
    ret