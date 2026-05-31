/*
* framebuffer.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: 
* Copyright (c) 2026 Max Van den Eynde
*/

#include <graphics/framebuffer.h>
#include <graphics/vgaFont.h>
#include <limine.h>

#include "graphics/graphicsTypes.h"
#include "types.h"

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

void Framebuffer::drawCharacter(
    Tuple<u64> pos,
    Color fg,
    Color bg,
    char c,
    float scaleBy
) const {
    if (scaleBy < 1.0f) {
        scaleBy = 1.0f;
    }

    const u64 scale = static_cast<u64>(scaleBy);

    if (c < FONT_FIRST || c > FONT_LAST) {
        c = '?';
    }

    const u8* glyph = font8x16[c - FONT_FIRST];

    for (u64 row = 0; row < FONT_HEIGHT; row++) {
        u8 bits = glyph[row];

        for (u64 col = 0; col < FONT_WIDTH; col++) {
            Color color = bg;

            if (bits & (0x80 >> col)) {
                color = fg;
            }

            const u64 pixelX = pos.first() + col * scale;
            const u64 pixelY = pos.second() + row * scale;

            for (u64 sy = 0; sy < scale; sy++) {
                for (u64 sx = 0; sx < scale; sx++) {
                    setColor({
                                 pixelX + sx,
                                 pixelY + sy
                             }, color);
                }
            }
        }
    }
}
