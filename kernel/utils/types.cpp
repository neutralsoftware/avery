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

bool string::startsWith(const string& other) const {
    if (len < other.len) return false;

    for (usize i = 0; i < other.len; i++) {
        if (data[i] != other.data[i]) return false;
    }

    return true;
}

void string::prepend(char c) {
    reserve(len + 1);

    for (usize i = len; i > 0; i--) {
        data[i] = data[i - 1];
    }

    data[0] = c;
    len++;
    data[len] = '\0';
}

void string::prepend(cstring text) {
    usize textLength = 0;

    while (text[textLength] != '\0') {
        textLength++;
    }

    if (textLength == 0) {
        return;
    }

    reserve(len + textLength);

    for (usize i = len; i > 0; i--) {
        data[i + textLength - 1] = data[i - 1];
    }

    memory::copy(
        reinterpret_cast<u8*>(data),
        reinterpret_cast<const u8*>(text),
        static_cast<int>(textLength)
    );

    len += textLength;
    data[len] = '\0';
}

void string::removePrefix(cstring text) {
    if (!text || !data)
        return;

    usize prefixLength = 0;
    while (text[prefixLength] != '\0')
        prefixLength++;

    if (prefixLength == 0 || prefixLength > len)
        return;

    for (usize i = 0; i < prefixLength; i++) {
        if (data[i] != text[i])
            return;
    }

    for (usize i = 0; i <= len - prefixLength; i++)
        data[i] = data[i + prefixLength];

    len -= prefixLength;
}

void string::removeSuffix(cstring text) {
    if (!text || !data)
        return;

    usize suffixLength = 0;
    while (text[suffixLength] != '\0')
        suffixLength++;

    if (suffixLength == 0 || suffixLength > len)
        return;

    usize start = len - suffixLength;

    for (usize i = 0; i < suffixLength; i++) {
        if (data[start + i] != text[i])
            return;
    }

    data[start] = '\0';
    len -= suffixLength;
}

void string::trim() {
    if (!data || len == 0)
        return;

    auto isSpace = [](char c)
    {
        return c == ' ' ||
            c == '\n' ||
            c == '\r' ||
            c == '\t' ||
            c == '\v' ||
            c == '\f';
    };

    usize start = 0;
    while (start < len && isSpace(data[start]))
        start++;

    usize end = len;
    while (end > start && isSpace(data[end - 1]))
        end--;

    usize newLength = end - start;

    for (usize i = 0; i < newLength; i++)
        data[i] = data[start + i];

    data[newLength] = '\0';
    len = newLength;
}
