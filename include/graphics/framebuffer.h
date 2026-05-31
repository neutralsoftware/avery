/*
* framebuffer.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Framebuffer features
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_FRAMEBUFFER_H
#define AVERY_FRAMEBUFFER_H
#include "../types.h"

struct limine_framebuffer_request;
struct limine_framebuffer;

class Framebuffer {
public:
    static Framebuffer createFromLimineRequest(volatile limine_framebuffer_request& request);

    void setColor(Tuple<u64> position, Color color) const;
    void paintRectangle(Tuple<u64> start, Tuple<u64> end, Color color) const;

private:
    volatile u32* fb_ptr = nullptr;
    limine_framebuffer* fb = nullptr;
};

#endif //AVERY_FRAMEBUFFER_H
