/*
* irq.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Interrupt Requests handler
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_IRQ_H
#define AVERY_IRQ_H

#define IRQ_EXTERN_FUNCTION(N) extern "C" void irq##N()
#include "isr.h"

namespace irq {
    IRQ_EXTERN_FUNCTION(0);
    IRQ_EXTERN_FUNCTION(1);
    IRQ_EXTERN_FUNCTION(2);
    IRQ_EXTERN_FUNCTION(3);
    IRQ_EXTERN_FUNCTION(4);
    IRQ_EXTERN_FUNCTION(5);
    IRQ_EXTERN_FUNCTION(6);
    IRQ_EXTERN_FUNCTION(7);
    IRQ_EXTERN_FUNCTION(8);
    IRQ_EXTERN_FUNCTION(9);
    IRQ_EXTERN_FUNCTION(10);
    IRQ_EXTERN_FUNCTION(11);
    IRQ_EXTERN_FUNCTION(12);
    IRQ_EXTERN_FUNCTION(13);
    IRQ_EXTERN_FUNCTION(14);
    IRQ_EXTERN_FUNCTION(15);

    using IRQHandler = void (*)(isr::Registers*);

    void installHandler(int irq, IRQHandler);
    void uninstallHandler(int irq);
    void remap();
}

namespace core {
    void initIrq();
}

extern "C" void irq_handler(isr::Registers* regs);


#endif //AVERY_IRQ_H
