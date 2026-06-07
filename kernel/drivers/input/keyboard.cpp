/*
* keyboard.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: PS/2 and input driver support
* Copyright (c) 2026 Max Van den Eynde
*/

#include <drivers/keyboard.h>

#include "core/irq.h"
#include "io/io.h"
#include "types.h"

const unsigned char keyboard::es[128] =
{
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '\'', '?', '\b',
    '\t',

    'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o', 'p',
    '`', '+', '\n',

    0,

    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    '?', '?',

    0,

    0,

    '?', 'z', 'x', 'c', 'v', 'b', 'n',
    'm', ',', '.', '-',

    0,

    '*',
    0,
    ' ',
    0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0,

    0,
    0,

    0,
    0,
    0,
    '-',

    0,
    0,
    0,
    '+',

    0,
    0,
    0,
    0,
    0,

    0, 0, 0,

    0,
    0,

    0,

    '<' // scancode 86 (ISO key, next to left shift)
};

const unsigned char keyboard::esShift[128] =
{
    0, 27, '!', '"', '#', '$', '%', '&', '/', '(',
    ')', '=', '?', '?', '\b',
    '\t',

    'Q', 'W', 'E', 'R',
    'T', 'Y', 'U', 'I', 'O', 'P',
    '^', '*', '\n',

    0,

    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    '?', '?',

    0,

    0,

    '?', 'Z', 'X', 'C', 'V', 'B', 'N',
    'M', '<', '>', '_',

    0,

    '*',
    0,
    ' ',
    0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0,

    0,
    0,

    0,
    0,
    0,
    '-',

    0,
    0,
    0,
    '+',

    0,
    0,
    0,
    0,
    0,

    0, 0, 0,

    0,
    0,

    '>' // ISO key (scancode 86)
};

const unsigned char* keyboard::activeLayout = es;

namespace {
    constexpr usize BufferSize = 256;

    char inputBuffer[BufferSize];
    volatile usize readIndex = 0;
    volatile usize writeIndex = 0;
    bool shiftPressed = false;
    bool extendedScancode = false;

    class KeyboardDevice final : public Device {
    public:
        KeyboardDevice() : Device("ps2-keyboard", DeviceType::Input) {
        }
    };

    class KeyboardDriver final : public Driver {
    public:
        string name() const override {
            return "PS/2 Keyboard - Avery";
        }

        bool probe(Device& device) override {
            return device.type() == DeviceType::Input;
        }

        bool start(Device&) override {
            irq::installHandler(1, &keyboard::handler);
            return true;
        }

        bool stop(Device&) override {
            irq::uninstallHandler(1);
            return true;
        }
    };

    KeyboardDriver* registeredDriver = nullptr;
    KeyboardDevice* registeredDevice = nullptr;

    void enqueue(char c) {
        usize nextIndex = (writeIndex + 1) % BufferSize;

        if (nextIndex == readIndex) {
            return;
        }

        inputBuffer[writeIndex] = c;
        writeIndex = nextIndex;
    }
}

void keyboard::handler([[maybe_unused]] isr::Registers* regs) {
    u8 scancode = io::inb(0x60);

    if (scancode == 0xE0) {
        extendedScancode = true;
        return;
    }

    if (extendedScancode) {
        extendedScancode = false;
        return;
    }

    if ((scancode & 0x80) != 0) {
        u8 released = static_cast<u8>(scancode & 0x7F);

        if (released == 0x2A || released == 0x36) {
            shiftPressed = false;
        }

        return;
    }

    if (scancode == 0x2A || scancode == 0x36) {
        shiftPressed = true;
        return;
    }

    const unsigned char* layout = shiftPressed ? esShift : activeLayout;
    char c = static_cast<char>(layout[scancode]);

    if (c != 0) {
        enqueue(c);
    }
}

bool keyboard::hasChar() {
    return readIndex != writeIndex;
}

char keyboard::getChar() {
    while (readIndex == writeIndex) {
        asm volatile("pause");
    }

    char c = inputBuffer[readIndex];
    readIndex = (readIndex + 1) % BufferSize;
    return c;
}

void keyboard::init() {
    irq::installHandler(1, &handler);
}

bool keyboard::registerDriver() {
    if (!registeredDriver) {
        registeredDriver = new KeyboardDriver();
    }

    return DriverManager::registerDriver(registeredDriver);
}

bool keyboard::registerDevice() {
    if (!registeredDevice) {
        registeredDevice = new KeyboardDevice();
    }

    return DeviceManager::registerDevice(registeredDevice);
}
