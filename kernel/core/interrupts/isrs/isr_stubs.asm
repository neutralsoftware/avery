;
; isr_stubs.asm
; Created by Max Van den Eynde
; in 2026 as part of the Avery project
;

; global isrN (name) (has error code?)
global isr0 ; divide by zero (no)
global isr1 ; debug exception (no)
global isr2 ; non maskable interrupt (no)
global isr3 ; breakpoint (no)
global isr4 ; into detected overflow (no)
global isr5 ; out of bounds (no)
global isr6 ; invalid opcode (no)
global isr7 ; no coprocessor (no)
global isr8 ; double fault exception (yes)
global isr9 ; coprocessor segment overrun (no)
global isr10 ; bad tss (yes)
global isr11 ; segment not present (yes)
global isr12 ; stack fault (yes)
global isr13 ; general protection fault (yes)
global isr14 ; page fault exception (yes)
global isr15 ; unknown interrupt (no)
global isr16 ; coprocessor fault exception (no)
global isr17 ; alignment check (no)
global isr18 ; machine check (no)
global isr19 ; reserved up to 31
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

extern fault_handler

%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push qword 0
    push qword %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push qword %1
    jmp isr_common_stub
%endmacro

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR   29
ISR_ERR   30
ISR_NOERR 31

isr_common_stub:
	push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
	mov rdi, rsp
    call fault_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    iretq











