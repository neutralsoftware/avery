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
#include "../../include/types.h"
#include "kernel/debug.h"

namespace {
    u64 fontScale(float scale) {
        if (scale < 1.0f) {
            return 1;
        }

        return static_cast<u64>(scale);
    }

    u64 glyphAdvance(char c) {
        if (c < FONT_FIRST || c > FONT_LAST) {
            c = '?';
        }

        if (c == ' ') {
            return 4;
        }

        const u8* glyph = font8x16[c - FONT_FIRST];
        i64 maxCol = -1;

        for (u64 row = 0; row < FONT_HEIGHT; row++) {
            u8 bits = glyph[row];

            for (u64 col = 0; col < FONT_WIDTH; col++) {
                if (bits & (0x80 >> col)) {
                    if (static_cast<i64>(col) > maxCol) {
                        maxCol = static_cast<i64>(col);
                    }
                }
            }
        }

        if (maxCol < 0) {
            return 4;
        }

        u64 advance = static_cast<u64>(maxCol) + 2;

        if (advance > FONT_WIDTH) {
            return FONT_WIDTH;
        }

        return advance;
    }
}

Framebuffer Framebuffer::createFromLimineRequest(
    volatile limine_framebuffer_request& request
) {
    Framebuffer framebuffer;

    ASSERT(request.response == nullptr || request.response->framebuffer_count < 1);

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

u64 Framebuffer::height() const {
    return fb->height;
}

u64 Framebuffer::width() const {
    return fb->width;
}

Color Framebuffer::getColor(Tuple<u64> position) const {
    u64 x = position.first();
    u64 y = position.second();

    auto expandTo8Bit = [](u32 val, u8 maskSize) -> u8
    {
        if (maskSize == 0) {
            return 0;
        }

        if (maskSize >= 8) {
            return static_cast<u8>(val & 0xFF);
        }

        const u32 maxValue = (1u << maskSize) - 1u;

        return static_cast<u8>((val * 255u) / maxValue);
    };

    if (x >= fb->width || y >= fb->height) {
        return Color{0, 0, 0};
    }

    if (fb->bpp != 32) {
        return Color{0, 0, 0};
    }

    usize pixelsPerRow = fb->pitch / 4;
    usize index = y * pixelsPerRow + x;

    u32 raw = fb_ptr[index];

    u32 rRaw = (raw >> fb->red_mask_shift) &
        ((1u << fb->red_mask_size) - 1u);

    u32 gRaw = (raw >> fb->green_mask_shift) &
        ((1u << fb->green_mask_size) - 1u);

    u32 bRaw = (raw >> fb->blue_mask_shift) &
        ((1u << fb->blue_mask_size) - 1u);

    return Color{
        expandTo8Bit(rRaw, fb->red_mask_size),
        expandTo8Bit(gRaw, fb->green_mask_size),
        expandTo8Bit(bRaw, fb->blue_mask_size)
    };
}

void Framebuffer::copyRect(Tuple<u64> src, Tuple<u64> dst, Tuple<u64> size) const {
    if (size.first() == 0 || size.second() == 0) {
        return;
    }

    if (src.first() >= width() || dst.first() >= width()) {
        return;
    }

    if (src.second() >= height() || dst.second() >= height()) {
        return;
    }

    if (src.first() + size.first() > width()) {
        size.a = width() - src.first();
    }

    if (dst.first() + size.first() > width()) {
        size.a = width() - dst.first();
    }

    if (src.second() + size.second() > height()) {
        size.b = height() - src.second();
    }

    if (dst.second() + size.second() > height()) {
        size.b = height() - dst.second();
    }

    if (size.first() == 0 || size.second() == 0) {
        return;
    }

    const bool copyBottomToTop = dst.second() > src.second();
    const bool copyRightToLeft = dst.first() > src.first();

    if (copyBottomToTop) {
        for (u64 y = size.second(); y > 0; y--) {
            const u64 row = y - 1;

            if (copyRightToLeft) {
                for (u64 x = size.first(); x > 0; x--) {
                    const u64 col = x - 1;

                    Color color = getColor({
                        src.first() + col,
                        src.second() + row
                    });

                    setColor({
                                 dst.first() + col,
                                 dst.second() + row
                             }, color);
                }
            }
            else {
                for (u64 col = 0; col < size.first(); col++) {
                    Color color = getColor({
                        src.first() + col,
                        src.second() + row
                    });

                    setColor({
                                 dst.first() + col,
                                 dst.second() + row
                             }, color);
                }
            }
        }
    }
    else {
        for (u64 row = 0; row < size.second(); row++) {
            if (copyRightToLeft) {
                for (u64 x = size.first(); x > 0; x--) {
                    const u64 col = x - 1;

                    Color color = getColor({
                        src.first() + col,
                        src.second() + row
                    });

                    setColor({
                                 dst.first() + col,
                                 dst.second() + row
                             }, color);
                }
            }
            else {
                for (u64 col = 0; col < size.first(); col++) {
                    Color color = getColor({
                        src.first() + col,
                        src.second() + row
                    });

                    setColor({
                                 dst.first() + col,
                                 dst.second() + row
                             }, color);
                }
            }
        }
    }
}

FramebufferConsole::FramebufferConsole(const Framebuffer& framebuffer,
                                       Color foreground,
                                       Color background,
                                       float scale) :
    framebuffer(framebuffer), fg(foreground), bg(background), scale(scale) {
    if (this->scale < 1.0f) {
        this->scale = 1.0f;
    }

    const u64 charHeight = FONT_HEIGHT * fontScale(scale);

    rows = framebuffer.height() / charHeight;

    clear();
}

void FramebufferConsole::clear() {
    framebuffer.paintRectangle(
        {0, 0}, {framebuffer.width(), framebuffer.height()}, bg);

    cursorX = 0;
    cursorY = 0;
}

void FramebufferConsole::drawCell(u64 pixelX, u64 y, char c) {
    const u64 pixelY = y * FONT_HEIGHT * fontScale(scale);

    framebuffer.drawCharacter(
        {pixelX, pixelY},
        fg,
        bg,
        c,
        scale
    );
}

void FramebufferConsole::newline() {
    cursorX = 0;
    cursorY++;

    if (cursorY >= rows) {
        scroll();
        cursorY = rows - 1;
    }
}

void FramebufferConsole::putChar(char c) {
    switch (c) {
    case '\n':
        newline();
        return;

    case '\r':
        cursorX = 0;
        return;

    case '\t':
    {
        constexpr u64 tabSize = 4;
        const u64 spaceWidth = glyphAdvance(' ') * fontScale(scale);
        const u64 tabWidth = spaceWidth * tabSize;
        u64 nextTab = ((cursorX / tabWidth) + 1) * tabWidth;

        while (cursorX < nextTab) {
            putChar(' ');
        }

        return;
    }

    case '\b':
        backspace();
        return;

    default:
        const u64 scaledFontWidth = FONT_WIDTH * fontScale(scale);
        const u64 advance = glyphAdvance(c) * fontScale(scale);

        if (cursorX + scaledFontWidth > framebuffer.width()) {
            newline();
        }

        drawCell(cursorX, cursorY, c);
        cursorX += advance;

        if (cursorX >= framebuffer.width()) {
            newline();
        }

        return;
    }
}

void FramebufferConsole::write(string str) {
    while (*str) {
        putChar(*str);
        str++;
    }
}

void FramebufferConsole::writeLn(string str) {
    while (*str) {
        putChar(*str);
        str++;
    }
    putChar('\n');
}

void FramebufferConsole::backspace() {
    if (cursorX == 0) {
        if (cursorY == 0) {
            return;
        }

        cursorY--;
        cursorX = framebuffer.width() - FONT_WIDTH * fontScale(scale);
    }
    else {
        const u64 charWidth = FONT_WIDTH * fontScale(scale);

        if (cursorX > charWidth) {
            cursorX -= charWidth;
        }
        else {
            cursorX = 0;
        }
    }

    drawCell(cursorX, cursorY, ' ');
}

void FramebufferConsole::scroll() {
    const u64 charHeight = FONT_HEIGHT * fontScale(scale);

    framebuffer.copyRect(
        {0, charHeight},
        {0, 0},
        {framebuffer.width(), framebuffer.height() - charHeight}
    );

    framebuffer.paintRectangle(
        {0, framebuffer.height() - charHeight},
        {framebuffer.width(), charHeight},
        bg
    );
}

void FramebufferConsole::drawCursor() {
    const u64 charHeight = FONT_HEIGHT * fontScale(scale);

    const u64 pixelX = cursorX;
    const u64 pixelY = cursorY * charHeight;

    framebuffer.paintRectangle(
        {pixelX, pixelY},
        {pixelX + FONT_WIDTH * fontScale(scale), pixelY + charHeight},
        fg
    );
}


void FramebufferConsole::eraseCursor() {
    const u64 charHeight = FONT_HEIGHT * fontScale(scale);

    const u64 pixelX = cursorX;
    const u64 pixelY = cursorY * charHeight;

    framebuffer.paintRectangle(
        {pixelX, pixelY},
        {pixelX + FONT_WIDTH * fontScale(scale), pixelY + charHeight},
        bg
    );
}

void FramebufferConsole::setColor(Color foreground, Color background) {
    this->fg = foreground;
    this->bg = background;
}

void FramebufferConsole::setCursor(u64 x, u64 y) {
    cursorX = x * FONT_WIDTH * fontScale(scale);

    if (cursorX >= framebuffer.width()) {
        cursorX = framebuffer.width() - FONT_WIDTH * fontScale(scale);
    }

    cursorY = y;

    if (cursorY >= rows) {
        cursorY = rows - 1;
    }
}
