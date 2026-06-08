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

#include "../../include/types.h"
#include "drivers/pit.h"
#include "graphics/graphicsTypes.h"
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
    return 5;
  }

  const u8 *glyph = font8x16[c - FONT_FIRST];
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
    return 5;
  }

  u64 advance = static_cast<u64>(maxCol) + 3;

  if (advance > FONT_WIDTH + 1) {
    return FONT_WIDTH + 1;
  }

  return advance;
}
}

Framebuffer Framebuffer::createFromLimineRequest(
    volatile limine_framebuffer_request &request) {
  Framebuffer framebuffer;

  ASSERT(request.response != nullptr &&
         request.response->framebuffer_count >= 1);

  framebuffer.fb = request.response->framebuffers[0];
  framebuffer.fb_ptr = static_cast<volatile u32 *>(framebuffer.fb->address);

  return framebuffer;
}

u32 Framebuffer::packColor(Color color) const {
  if (fb->bpp != 32) {
    return 0;
  }

  return ((static_cast<u32>(color.r) >> (8 - fb->red_mask_size))
          << fb->red_mask_shift) |
         ((static_cast<u32>(color.g) >> (8 - fb->green_mask_size))
          << fb->green_mask_shift) |
         ((static_cast<u32>(color.b) >> (8 - fb->blue_mask_size))
          << fb->blue_mask_shift);
}

u64 Framebuffer::pixelsPerRow() const { return fb->pitch / 4; }

void Framebuffer::setColor(Tuple<u64> position, Color color) const {
  u64 x = position.first();
  u64 y = position.second();

  if (x >= fb->width || y >= fb->height) {
    return;
  }

  if (fb->bpp != 32) {
    return;
  }

  fb_ptr[y * pixelsPerRow() + x] = packColor(color);
}

void Framebuffer::paintRectangle(Tuple<u64> start, Tuple<u64> end,
                                 Color color) const {
  paintRectanglePacked(start, end, packColor(color));
}

void Framebuffer::paintRectanglePacked(Tuple<u64> start, Tuple<u64> end,
                                       u32 color) const {
  if (fb->bpp != 32) {
    return;
  }

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

  const u64 pitch = pixelsPerRow();

  for (u64 y = startY; y < endY; y++) {
    volatile u32 *row = fb_ptr + y * pitch + startX;
    for (u64 x = startX; x < endX; x++) {
      row[x - startX] = color;
    }
  }
}

void Framebuffer::drawCharacter(Tuple<u64> pos, Color fg, Color bg, char c,
                                float scaleBy) const {
  if (scaleBy < 1.0f) {
    scaleBy = 1.0f;
  }

  const u64 scale = static_cast<u64>(scaleBy);

  if (c < FONT_FIRST || c > FONT_LAST) {
    c = '?';
  }

  const u8 *glyph = font8x16[c - FONT_FIRST];
  const u32 packedFg = packColor(fg);
  const u32 packedBg = packColor(bg);
  const u64 glyphWidth = FONT_WIDTH * scale;
  const u64 glyphHeight = FONT_HEIGHT * scale;

  paintRectanglePacked(
      pos, {pos.first() + glyphWidth, pos.second() + glyphHeight}, packedBg);
  const u64 pitch = pixelsPerRow();

  for (u64 row = 0; row < FONT_HEIGHT; row++) {
    u8 bits = glyph[row];

    for (u64 col = 0; col < FONT_WIDTH; col++) {
      if ((bits & (0x80 >> col)) == 0) {
        continue;
      }

      const u64 pixelX = pos.first() + col * scale;
      const u64 pixelY = pos.second() + row * scale;

      if (pixelX >= fb->width || pixelY >= fb->height) {
        continue;
      }

      for (u64 sy = 0; sy < scale; sy++) {
        if (pixelY + sy >= fb->height) {
          continue;
        }

        volatile u32 *target = fb_ptr + (pixelY + sy) * pitch + pixelX;

        for (u64 sx = 0; sx < scale; sx++) {
          if (pixelX + sx >= fb->width) {
            continue;
          }

          target[sx] = packedFg;
        }
      }
    }
  }
}

u64 Framebuffer::height() const { return fb->height; }

u64 Framebuffer::width() const { return fb->width; }

Color Framebuffer::getColor(Tuple<u64> position) const {
  u64 x = position.first();
  u64 y = position.second();

  auto expandTo8Bit = [](u32 val, u8 maskSize) -> u8 {
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

  u32 raw = fb_ptr[y * pixelsPerRow() + x];

  u32 rRaw = (raw >> fb->red_mask_shift) & ((1u << fb->red_mask_size) - 1u);

  u32 gRaw = (raw >> fb->green_mask_shift) & ((1u << fb->green_mask_size) - 1u);

  u32 bRaw = (raw >> fb->blue_mask_shift) & ((1u << fb->blue_mask_size) - 1u);

  return Color{expandTo8Bit(rRaw, fb->red_mask_size),
               expandTo8Bit(gRaw, fb->green_mask_size),
               expandTo8Bit(bRaw, fb->blue_mask_size)};
}

void Framebuffer::copyRect(Tuple<u64> src, Tuple<u64> dst,
                           Tuple<u64> size) const {
  if (fb->bpp != 32) {
    return;
  }

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
  const u64 pitch = pixelsPerRow();

  if (copyBottomToTop) {
    for (u64 y = size.second(); y > 0; y--) {
      const u64 row = y - 1;
      volatile u32 *srcRow =
          fb_ptr + (src.second() + row) * pitch + src.first();
      volatile u32 *dstRow =
          fb_ptr + (dst.second() + row) * pitch + dst.first();

      if (copyRightToLeft) {
        for (u64 x = size.first(); x > 0; x--) {
          const u64 col = x - 1;
          dstRow[col] = srcRow[col];
        }
      } else {
        for (u64 col = 0; col < size.first(); col++) {
          dstRow[col] = srcRow[col];
        }
      }
    }
  } else {
    for (u64 row = 0; row < size.second(); row++) {
      volatile u32 *srcRow =
          fb_ptr + (src.second() + row) * pitch + src.first();
      volatile u32 *dstRow =
          fb_ptr + (dst.second() + row) * pitch + dst.first();

      if (copyRightToLeft) {
        for (u64 x = size.first(); x > 0; x--) {
          const u64 col = x - 1;
          dstRow[col] = srcRow[col];
        }
      } else {
        for (u64 col = 0; col < size.first(); col++) {
          dstRow[col] = srcRow[col];
        }
      }
    }
  }
}

void Framebuffer::flushPixels(const u32 *pixels, u64 sourcePitch,
                              Tuple<u64> start, Tuple<u64> end) const {
  if (!pixels || fb->bpp != 32) {
    return;
  }

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

  const u64 targetPitch = pixelsPerRow();

  for (u64 y = startY; y < endY; y++) {
    const u32 *source = pixels + y * sourcePitch + startX;
    volatile u32 *target = fb_ptr + y * targetPitch + startX;

    for (u64 x = startX; x < endX; x++) {
      target[x - startX] = source[x - startX];
    }
  }
}

FramebufferConsole::FramebufferConsole(const Framebuffer &framebuffer,
                                       Color foreground, Color background,
                                       float scale)
    : framebuffer(framebuffer), fg(foreground), bg(background), scale(scale) {
  if (this->scale < 1.0f) {
    this->scale = 1.0f;
  }

  charHeight = FONT_HEIGHT * fontScale(scale);
  backbufferPitch = framebuffer.width();
  backbuffer = new u32[backbufferPitch * framebuffer.height()];
  syncPackedColors();

  rows = framebuffer.height() / charHeight;
  if (rows == 0) {
    rows = 1;
  }

  clear();
}

void FramebufferConsole::clear() {
  if (!backbuffer) {
    framebuffer.paintRectanglePacked(
        {0, 0}, {framebuffer.width(), framebuffer.height()}, packedBg);
    cursorX = 0;
    cursorY = 0;
    cursorDrawn = false;
    cursorBlinkTimestamp = time::getUptime();
    drawCursor();
    return;
  }

  fillBackbuffer({0, 0}, {framebuffer.width(), framebuffer.height()}, packedBg);
  flushBackbuffer({0, 0}, {framebuffer.width(), framebuffer.height()});

  cursorX = 0;
  cursorY = 0;
  cursorDrawn = false;
  cursorBlinkTimestamp = time::getUptime();
  drawCursor();
}

void FramebufferConsole::drawCell(u64 pixelX, u64 y, char c) {
  const u64 pixelY = y * charHeight;
  const u64 scaledFontWidth = FONT_WIDTH * fontScale(scale);

  if (!backbuffer) {
    framebuffer.drawCharacter({pixelX, pixelY}, fg, bg, c, scale);
    return;
  }

  if (c < FONT_FIRST || c > FONT_LAST) {
    c = '?';
  }

  if (c == ' ') {
    fillBackbuffer({pixelX, pixelY},
                   {pixelX + scaledFontWidth, pixelY + charHeight}, packedBg);
    flushBackbuffer({pixelX, pixelY},
                    {pixelX + scaledFontWidth, pixelY + charHeight});
    return;
  }

  const u8 *glyph = font8x16[c - FONT_FIRST];
  const u64 scaleValue = fontScale(scale);
  fillBackbuffer({pixelX, pixelY},
                 {pixelX + scaledFontWidth, pixelY + charHeight}, packedBg);

  for (u64 row = 0; row < FONT_HEIGHT; row++) {
    const u8 bits = glyph[row];

    for (u64 col = 0; col < FONT_WIDTH; col++) {
      if ((bits & (0x80 >> col)) == 0) {
        continue;
      }

      const u64 startX = pixelX + col * scaleValue;
      const u64 startY = pixelY + row * scaleValue;

      fillBackbuffer({startX, startY},
                     {startX + scaleValue, startY + scaleValue}, packedFg);
    }
  }

  flushBackbuffer({pixelX, pixelY},
                  {pixelX + scaledFontWidth, pixelY + charHeight});
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
  if (cursorDrawn) {
    eraseCursor();
  }

  switch (c) {
  case '\n':
    newline();
    break;

  case '\r':
    cursorX = 0;
    break;

  case '\t': {
    constexpr u64 tabSize = 4;
    const u64 spaceWidth = glyphAdvance(' ') * fontScale(scale);
    const u64 tabWidth = spaceWidth * tabSize;
    u64 nextTab = ((cursorX / tabWidth) + 1) * tabWidth;

    while (cursorX < nextTab) {
      putChar(' ');
    }

    break;
  }

  case '\b':
    backspace();
    break;

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

    break;
  }

  if (cursorVisible) {
    cursorBlinkTimestamp = time::getUptime();
    drawCursor();
  }
}

void FramebufferConsole::write(cstring str) {
  while (*str) {
    putChar(*str);
    str++;
  }
}

void FramebufferConsole::writeLn(cstring str) {
  while (*str) {
    putChar(*str);
    str++;
  }
  putChar('\n');
}

void FramebufferConsole::backspace() {
  if (cursorDrawn) {
    eraseCursor();
  }

  if (cursorX == 0) {
    if (cursorY == 0) {
      if (cursorVisible) {
        cursorBlinkTimestamp = time::getUptime();
        drawCursor();
      }
      return;
    }

    cursorY--;
    cursorX = framebuffer.width() - FONT_WIDTH * fontScale(scale);
  } else {
    const u64 charWidth = FONT_WIDTH * fontScale(scale);

    if (cursorX > charWidth) {
      cursorX -= charWidth;
    } else {
      cursorX = 0;
    }
  }

  drawCell(cursorX, cursorY, ' ');

  if (cursorVisible) {
    cursorBlinkTimestamp = time::getUptime();
    drawCursor();
  }
}

void FramebufferConsole::backspace(char c) {
  if (cursorDrawn) {
    eraseCursor();
  }

  const u64 charWidth = glyphAdvance(c) * fontScale(scale);

  if (cursorX >= charWidth) {
    cursorX -= charWidth;
  } else {
    cursorX = 0;
  }

  drawCell(cursorX, cursorY, ' ');

  if (cursorVisible) {
    cursorBlinkTimestamp = time::getUptime();
    drawCursor();
  }
}

void FramebufferConsole::scroll() {
  if (charHeight == 0 || charHeight >= framebuffer.height()) {
    fillBackbuffer({0, 0}, {framebuffer.width(), framebuffer.height()},
                   packedBg);
    framebuffer.paintRectanglePacked(
        {0, 0}, {framebuffer.width(), framebuffer.height()}, packedBg);
    return;
  }

  if (!backbuffer) {
    framebuffer.copyRect(
        {0, charHeight}, {0, 0},
        {framebuffer.width(), framebuffer.height() - charHeight});

    framebuffer.paintRectangle({0, framebuffer.height() - charHeight},
                               {framebuffer.width(), framebuffer.height()}, bg);
    return;
  }

  const u64 copyHeight = framebuffer.height() - charHeight;

  for (u64 y = 0; y < copyHeight; y++) {
    u32 *dst = backbuffer + y * backbufferPitch;
    u32 *src = backbuffer + (y + charHeight) * backbufferPitch;

    for (u64 x = 0; x < framebuffer.width(); x++) {
      dst[x] = src[x];
    }
  }

  fillBackbuffer({0, copyHeight}, {framebuffer.width(), framebuffer.height()},
                 packedBg);
  framebuffer.copyRect({0, charHeight}, {0, 0},
                       {framebuffer.width(), copyHeight});
  framebuffer.paintRectanglePacked(
      {0, copyHeight}, {framebuffer.width(), framebuffer.height()}, packedBg);
}

void FramebufferConsole::drawCursor() {
  const u64 cursorWidth = fontScale(scale);

  const u64 pixelX = cursorX;
  const u64 pixelY = cursorY * charHeight;

  framebuffer.paintRectanglePacked(
      {pixelX, pixelY}, {pixelX + cursorWidth, pixelY + charHeight}, packedFg);

  cursorDrawn = true;
}

void FramebufferConsole::eraseCursor() {
  if (!cursorDrawn) {
    return;
  }

  const u64 cursorWidth = fontScale(scale);

  const u64 pixelX = cursorX;
  const u64 pixelY = cursorY * charHeight;

  if (backbuffer) {
    flushBackbuffer({pixelX, pixelY},
                    {pixelX + cursorWidth, pixelY + charHeight});
  } else {
    framebuffer.paintRectanglePacked(
        {pixelX, pixelY}, {pixelX + cursorWidth, pixelY + charHeight},
        packedBg);
  }

  cursorDrawn = false;
}

void FramebufferConsole::setColor(Color foreground, Color background) {
  this->fg = foreground;
  this->bg = background;
  syncPackedColors();
}

void FramebufferConsole::fillBackbuffer(Tuple<u64> start, Tuple<u64> end,
                                        u32 color) {
  if (!backbuffer) {
    return;
  }

  u64 startX = start.first();
  u64 startY = start.second();
  u64 endX = end.first();
  u64 endY = end.second();

  if (endX <= startX || endY <= startY) {
    return;
  }

  if (startX >= framebuffer.width() || startY >= framebuffer.height()) {
    return;
  }

  if (endX > framebuffer.width()) {
    endX = framebuffer.width();
  }

  if (endY > framebuffer.height()) {
    endY = framebuffer.height();
  }

  for (u64 y = startY; y < endY; y++) {
    u32 *row = backbuffer + y * backbufferPitch + startX;

    for (u64 x = startX; x < endX; x++) {
      row[x - startX] = color;
    }
  }
}

void FramebufferConsole::flushBackbuffer(Tuple<u64> start, Tuple<u64> end) {
  if (!backbuffer) {
    return;
  }

  framebuffer.flushPixels(backbuffer, backbufferPitch, start, end);
}

void FramebufferConsole::syncPackedColors() {
  packedFg = framebuffer.packColor(fg);
  packedBg = framebuffer.packColor(bg);
}

void FramebufferConsole::setCursor(u64 x, u64 y) {
  if (cursorDrawn) {
    eraseCursor();
  }

  cursorX = x * FONT_WIDTH * fontScale(scale);

  if (cursorX >= framebuffer.width()) {
    cursorX = framebuffer.width() - FONT_WIDTH * fontScale(scale);
  }

  cursorY = y;

  if (cursorY >= rows) {
    cursorY = rows - 1;
  }

  if (cursorVisible) {
    cursorBlinkTimestamp = time::getUptime();
    drawCursor();
  }
}

void FramebufferConsole::updateCursor() {
  if (!cursorVisible) {
    eraseCursor();
    return;
  }

  u64 now = time::getUptime();

  if (now - cursorBlinkTimestamp < 500) {
    return;
  }

  cursorBlinkTimestamp = now;

  if (cursorDrawn) {
    eraseCursor();
  } else {
    drawCursor();
  }
}
