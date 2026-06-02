/*
* isr.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Interrupt Subroutines
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_ISR_H
#define AVERY_ISR_H

#define ISR_EXTERN_FUNCTION(N) extern "C" void isr##N()
#include "../types.h"

namespace isr {
    ISR_EXTERN_FUNCTION(0);
    ISR_EXTERN_FUNCTION(1);
    ISR_EXTERN_FUNCTION(2);
    ISR_EXTERN_FUNCTION(3);
    ISR_EXTERN_FUNCTION(4);
    ISR_EXTERN_FUNCTION(5);
    ISR_EXTERN_FUNCTION(6);
    ISR_EXTERN_FUNCTION(7);
    ISR_EXTERN_FUNCTION(8);
    ISR_EXTERN_FUNCTION(9);
    ISR_EXTERN_FUNCTION(10);
    ISR_EXTERN_FUNCTION(11);
    ISR_EXTERN_FUNCTION(12);
    ISR_EXTERN_FUNCTION(13);
    ISR_EXTERN_FUNCTION(14);
    ISR_EXTERN_FUNCTION(15);
    ISR_EXTERN_FUNCTION(16);
    ISR_EXTERN_FUNCTION(17);
    ISR_EXTERN_FUNCTION(18);
    ISR_EXTERN_FUNCTION(19);
    ISR_EXTERN_FUNCTION(20);
    ISR_EXTERN_FUNCTION(21);
    ISR_EXTERN_FUNCTION(22);
    ISR_EXTERN_FUNCTION(23);
    ISR_EXTERN_FUNCTION(24);
    ISR_EXTERN_FUNCTION(25);
    ISR_EXTERN_FUNCTION(26);
    ISR_EXTERN_FUNCTION(27);
    ISR_EXTERN_FUNCTION(28);
    ISR_EXTERN_FUNCTION(29);
    ISR_EXTERN_FUNCTION(30);
    ISR_EXTERN_FUNCTION(31);

    struct Registers {
        u64 r15, r14, r13, r12, r11, r10, r9, r8;
        u64 rsi, rdi, rbp, rdx, rcx, rbx, rax;
        u64 int_no;
        u64 err_code;
        u64 rip;
        u64 cs;
        u64 rflags;
        u64 rsp;
        u64 ss;
    };

    void fault_handler(Registers* regs);
};

namespace core {
    void initIsrs();
}

#endif //AVERY_ISR_H
