/*
* pic.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Handler for the legacy PIC
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_PIC_H
#define AVERY_PIC_H
#include "../types.h"

namespace pic {
    void remap();
    void maskAll();

    void enableIRQ(u8 irq);
    void disableIRQ(u8 irq);
    void eoi(u8 irq);
}

#endif //AVERY_PIC_H
