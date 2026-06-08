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
#include "graphicsTypes.h"

struct limine_framebuffer_request;
struct limine_framebuffer;

class Framebuffer {
public:
  static Framebuffer
  createFromLimineRequest(volatile limine_framebuffer_request &request);

  void setColor(Tuple<u64> position, Color color) const;
  void paintRectangle(Tuple<u64> start, Tuple<u64> end, Color color) const;
  void drawCharacter(Tuple<u64> pos, Color fg, Color bg, char c,
                     float scaleBy = 1.0) const;
  void copyRect(Tuple<u64> src, Tuple<u64> dst, Tuple<u64> size) const;
  void paintRectanglePacked(Tuple<u64> start, Tuple<u64> end, u32 color) const;
  void flushPixels(const u32 *pixels, u64 sourcePitch, Tuple<u64> start,
                   Tuple<u64> end) const;

  [[nodiscard]] Color getColor(Tuple<u64> pos) const;
  [[nodiscard]] u32 packColor(Color color) const;
  [[nodiscard]] u64 pixelsPerRow() const;

  [[nodiscard]] u64 width() const;
  [[nodiscard]] u64 height() const;

private:
  volatile u32 *fb_ptr = nullptr;
  limine_framebuffer *fb = nullptr;
};

class FramebufferConsole {
public:
  FramebufferConsole(const Framebuffer &framebuffer, Color foreground,
                     Color background, float scale = 1.0);

  void clear();
  void putChar(char c);
  void write(cstring str);
  void writeLn(cstring str);
  void newline();
  void backspace();
  void backspace(char c);

  void setColor(Color foreground, Color background);
  void setCursor(u64 x, u64 y);
  void updateCursor();

private:
  void drawCursor();
  void eraseCursor();
  void scroll();
  void drawCell(u64 pixelX, u64 y, char c);
  void fillBackbuffer(Tuple<u64> start, Tuple<u64> end, u32 color);
  void flushBackbuffer(Tuple<u64> start, Tuple<u64> end);
  void syncPackedColors();

  const Framebuffer &framebuffer;

  Color fg;
  Color bg;
  u32 packedFg = 0;
  u32 packedBg = 0;

  float scale;

  u64 cursorX = 0;
  u64 cursorY = 0;
  bool cursorVisible = true;
  bool cursorDrawn = false;
  u64 cursorBlinkTimestamp = 0;

  u64 rows;
  u64 charHeight;
  u64 backbufferPitch;
  u32 *backbuffer = nullptr;
};

#endif // AVERY_FRAMEBUFFER_H
