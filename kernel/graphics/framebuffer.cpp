/*
* framebuffer.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: 
* Copyright (c) 2026 Max Van den Eynde
*/

#include <graphics/framebuffer.h>
#include <limine.h>

Framebuffer Framebuffer::createFromLimineRequest(
    volatile limine_framebuffer_request& request
) {
    Framebuffer framebuffer;

    if (request.response == nullptr || request.response->framebuffer_count < 1) {
        while (true) {
            asm volatile("hlt");
        }
    }

    framebuffer.fb = request.response->framebuffers[0];
    framebuffer.fb_ptr = static_cast<volatile u32*>(framebuffer.fb->address);

    return framebuffer;
}

void Framebuffer::setColor(Tuple<u64> position, Color color) const {
    u64 x = position.first();
    u64 y = position.second();

    if (x >= fb->width || y >= fb->height) {
        return;
    }

    if (fb->bpp != 32) {
        return;
    }

    usize pixelsPerRow = fb->pitch / 4;
    usize index = y * pixelsPerRow + x;

    u32 packed =
        ((static_cast<u32>(color.r) >> (8 - fb->red_mask_size)) << fb->red_mask_shift) |
        ((static_cast<u32>(color.g) >> (8 - fb->green_mask_size)) << fb->green_mask_shift) |
        ((static_cast<u32>(color.b) >> (8 - fb->blue_mask_size)) << fb->blue_mask_shift);

    fb_ptr[index] = packed;
}

void Framebuffer::paintRectangle(Tuple<u64> start, Tuple<u64> end, Color color) const {
    u64 startX = start.first();
    u64 startY = start.second();
    u64 endX = end.first();
    u64 endY = end.second();

    if (endX <= startX || endY <= startY) {
        return;
    }

    if (startX >= fb->width || startY >= fb->height) {
        return;
    }

    if (endX > fb->width) {
        endX = fb->width;
    }

    if (endY > fb->height) {
        endY = fb->height;
    }

    for (u64 y = startY; y < endY; y++) {
        for (u64 x = startX; x < endX; x++) {
            setColor(Tuple<u64>{x, y}, color);
        }
    }
}
