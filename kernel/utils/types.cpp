/*
* types.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Types definition for easy interoperability and access
* Copyright (c) 2026 Max Van den Eynde
*/

#include <kernel/memory.h>
#include <types.h>

string::string() {
    len = 0;
    capacity = 0;

    data = new char[1];
    data[0] = '\0';
}

string::string(cstring str) {
    usize strLength = 0;

    while (str[strLength] != '\0') {
        strLength++;
    }

    len = strLength;
    capacity = strLength;

    data = new char[capacity + 1];
    memory::copy(reinterpret_cast<u8*>(data), reinterpret_cast<const u8*>(str), static_cast<int>(len));
    data[len] = '\0';
}

string::string(const string& other) {
    len = other.len;
    capacity = other.capacity;

    data = new char[capacity + 1];
    memory::copy(reinterpret_cast<u8*>(data), reinterpret_cast<const u8*>(other.data), static_cast<int>(len));
    data[len] = '\0';
}

string::string(string&& other) noexcept {
    len = other.len;
    capacity = other.capacity;
    data = other.data;

    other.len = 0;
    other.capacity = 0;
    other.data = new char[1];
    other.data[0] = '\0';
}

string::~string() {
    delete[] data;
}

string& string::operator=(const string& other) {
    if (this == &other) {
        return *this;
    }

    char* newData = new char[other.capacity + 1];

    memory::copy(
        reinterpret_cast<u8*>(newData),
        reinterpret_cast<const u8*>(other.data),
        static_cast<int>(other.len)
    );

    newData[other.len] = '\0';

    delete[] data;

    data = newData;
    len = other.len;
    capacity = other.capacity;

    return *this;
}

string& string::operator=(string&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    delete[] data;

    data = other.data;
    len = other.len;
    capacity = other.capacity;

    other.len = 0;
    other.capacity = 0;
    other.data = new char[1];
    other.data[0] = '\0';

    return *this;
}

bool string::operator==(const string& other) const {
    if (other.len != this->len) {
        return false;
    }

    usize strLength = other.len;
    for (usize i = 0; i < strLength; i++) {
        if (data[i] != other.data[i]) {
            return false;
        }
    }

    return true;
}

bool string::operator!=(const string& other) const {
    return !(*this == other);
}

cstring string::cStr() const {
    return data;
}

usize string::length() const {
    return len;
}

bool string::empty() const {
    return len == 0;
}

void string::clear() {
    len = 0;
    data[0] = '\0';
}

void string::reserve(usize newCapacity) {
    if (newCapacity <= capacity) {
        return;
    }

    char* newData = new char[newCapacity + 1];
    memory::copy(reinterpret_cast<u8*>(newData), reinterpret_cast<const u8*>(data), static_cast<int>(len));
    newData[len] = '\0';
    delete[] data;

    data = newData;
    capacity = newCapacity;
}

void string::append(char c) {
    reserve(len + 1);
    memory::copy(reinterpret_cast<u8*>(data) + len, reinterpret_cast<const u8*>(&c), 1);
    len++;
    data[len] = '\0';
}

char string::popBack() {
    if (len == 0) {
        return '\0';
    }

    len--;
    char c = data[len];
    data[len] = '\0';

    return c;
}

void string::append(cstring text) {
    usize textLength = 0;

    while (text[textLength] != '\0') {
        textLength++;
    }

    reserve(len + textLength);
    memory::copy(reinterpret_cast<u8*>(data) + len, reinterpret_cast<const u8*>(text),
                 static_cast<int>(textLength));
    len += textLength;
    data[len] = '\0';
}

Option<char&> string::operator[](usize index) {
    if (index >= len) {
        return Option<char&>::none();
    }

    return data[index];
}

Option<const char&> string::operator[](usize index) const {
    if (index >= len) {
        return Option<const char&>::none();
    }

    return data[index];
}

extern "C" [[noreturn]] void __cxa_pure_virtual() {
    while (true) {
        asm volatile("hlt");
    }
}

extern "C" void* memset(void* dest, int value, usize count) {
    auto* p = static_cast<unsigned char*>(dest);

    for (usize i = 0; i < count; i++) {
        p[i] = static_cast<unsigned char>(value);
    }

    return dest;
}
