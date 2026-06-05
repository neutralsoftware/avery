/*
* types.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Type definition for the Avery Kernel
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_TYPES_H
#define AVERY_TYPES_H

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
using usize = decltype(sizeof(0));
using uptr = decltype(sizeof(0));
using physAddr = uptr;
using virtAddr = uptr;

using i8 = char;
using i16 = short;
using i32 = int;
using i64 = long long;

using cstring = const char*;

namespace debug {
    void serialError(const char* message);
}

template <typename T>
struct Tuple {
    T a;
    T b;

    Tuple<T>(T a, T b) : a(a), b(b) {
    }

    T first() { return a; }
    T second() { return b; }
};

template <typename T>
Tuple(T, T) -> Tuple<T>;

inline void* operator new(usize, void* ptr) noexcept {
    return ptr;
}

inline void operator delete(void*, void*) noexcept {
}

template <typename T>
concept ByteNumber = requires(T a, T b) {
    a + b;
    ~a;
    a & b;
};

template <typename T>
    requires ByteNumber<T>
T alignUp(T x, T a) {
    return (x + a - 1) & ~(a - 1);
}

template <typename T>
    requires ByteNumber<T>
T alignDown(T x, T a) {
    return (x + a - 1) & ~(a - 1);
}

template <typename T>
class Option {
public:
    Option() {
        present = false;
    }

    Option(const T& value) {
        present = true;
        storage = value;
    }

    [[nodiscard]] bool hasValue() const {
        return present;
    }

    T& value() {
        if (!present) {
            debug::serialError("Tried to access an option that had a null value");
        }
        return storage;
    }

    const T& value() const {
        if (!present) {
            debug::serialError("Tried to access an option that had a null value");
        }
        return storage;
    }

    T valueOr(const T& fallback) const {
        if (!present) {
            return fallback;
        }
        return storage;
    }

    static Option none() {
        return Option();
    }

private:
    bool present;
    T storage;
};

template <typename T>
class Option<T&> {
public:
    Option() {
        present = false;
        storage = nullptr;
    }

    Option(T& value) {
        present = true;
        storage = &value;
    }

    [[nodiscard]] bool hasValue() const {
        return present;
    }

    T& value() {
        if (!present) {
            debug::serialError("Tried to access an option that had a null value");
        }
        return storage;
    }

    const T& value() const {
        if (!present) {
            debug::serialError("Tried to access an option that had a null value");
        }
        return storage;
    }

    T valueOr(const T& fallback) const {
        if (!present) {
            return fallback;
        }
        return storage;
    }

    static Option none() {
        return Option();
    }

private:
    bool present;
    T* storage;
};

class string {
public:
    string();
    string(cstring str);
    string(const string& other);
    string(string&& other) noexcept;

    ~string();

    string& operator=(const string& other);
    string& operator=(string&& other) noexcept;

    bool operator==(const string& other) const;
    bool operator!=(const string& other) const;

    [[nodiscard]] cstring cStr() const;
    [[nodiscard]] usize length() const;
    [[nodiscard]] bool empty() const;

    void clear();
    void append(cstring text);
    void append(char c);
    char popBack();

    Option<char&> operator[](usize index);
    Option<const char&> operator[](usize index) const;

private:
    char* data;
    usize len;
    usize capacity;

    void reserve(usize newCapacity);
};

template <typename T>
class UniquePtr {
public:
    UniquePtr() {
        ptr = nullptr;
    }

    explicit UniquePtr(T* ptr) {
        this->ptr = ptr;
    }

    ~UniquePtr() {
        delete ptr;
    }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept {
        ptr = other.ptr;
        other.ptr = nullptr;
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        delete ptr;

        ptr = other.ptr;
        other.ptr = nullptr;

        return *this;
    }

    T* get() const {
        return ptr;
    }

    T* release() {
        T* old = ptr;
        ptr = nullptr;
        return old;
    }

    void reset(T* newPtr = nullptr) {
        if (this->ptr == newPtr) {
            return;
        }

        delete this->ptr;
        this->ptr = newPtr;
    }

    T& operator*() const {
        return *ptr;
    }

    T* operator->() const {
        return ptr;
    }

    operator bool() const {
        return ptr != nullptr;
    }

private:
    T* ptr;
};

template <typename T>
class LinkedList {
public:
    struct Node {
        T value;
        Node* next;
        Node* prev;

        Node(const T& value)
            : value(value), next(nullptr), prev(nullptr) {
        }
    };

    LinkedList() {
        head = nullptr;
        tail = nullptr;
        count = 0;
    }

    ~LinkedList() {
        clear();
    }

    void pushBack(const T& value) {
        Node* newNode = new Node(value);
        ASSERT(newNode != nullptr);

        if (empty()) {
            head = tail = newNode;
            count++;
            return;
        }

        newNode->prev = tail;
        tail->next = newNode;
        tail = newNode;

        count++;
    }

    void pushFront(const T& value) {
        Node* newNode = new Node(value);
        ASSERT(newNode != nullptr);

        if (empty()) {
            head = tail = newNode;
            count++;
            return;
        }

        newNode->next = head;
        head->prev = newNode;
        head = newNode;

        count++;
    }

    void popBack() {
        if (empty()) {
            return;
        }

        Node* oldNode = tail;

        if (head == tail) {
            head = nullptr;
            tail = nullptr;
        }
        else {
            tail = tail->prev;
            tail->next = nullptr;
        }

        delete oldNode;
        count--;
    }

    void popFront() {
        if (empty()) {
            return;
        }

        Node* oldNode = head;

        if (head == tail) {
            head = nullptr;
            tail = nullptr;
        }
        else {
            head = head->next;
            head->prev = nullptr;
        }

        delete oldNode;
        count--;
    }

    T& front() {
        ASSERT(head != nullptr);
        return head->value;
    }

    T& back() {
        ASSERT(tail != nullptr);
        return tail->value;
    }

    [[nodiscard]] bool empty() const {
        return count == 0;
    }

    [[nodiscard]] usize size() const {
        return count;
    }

    void clear() {
        Node* current = head;

        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }

        head = nullptr;
        tail = nullptr;
        count = 0;
    }

private:
    Node* head;
    Node* tail;
    usize count;
};

template <typename T>
class Queue {
public:
    Queue() = default;

    void push(const T& value) {
        list.pushBack(value);
    }

    Option<T> pop() {
        if (list.empty()) {
            return Option<T>::none();
        }

        T value = list.front();
        list.popFront();

        return value;
    }

    T& front() {
        return list.front();
    }

    [[nodiscard]] bool empty() const {
        return list.empty();
    }

    [[nodiscard]] usize size() const {
        return list.size();
    }

private:
    LinkedList<T> list;
};


template <typename T>
class Vector {
public:
    Vector() {
        data = nullptr;
        count = 0;
        cap = 0;
    }

    ~Vector() {
        delete[] data;
    }

    void push(const T& value) {
        if (count >= cap) {
            grow();
        }

        data[count] = value;
        count++;
    }

    void push(T&& value) {
        if (count >= cap) {
            grow();
        }

        data[count] = static_cast<T&&>(value);
        count++;
    }

    void pop() {
        if (count == 0) {
            return;
        }

        count--;
    }


    Option<T&> operator[](usize index) {
        if (index >= count) {
            return Option<T&>::none();
        }

        return Option<T&>(data[index]);
    }

    Option<const T&> operator[](usize index) const {
        if (index >= count) {
            return Option<const T&>::none();
        }

        return Option<const T&>(data[index]);
    }


    Option<T&> last() {
        if (count == 0) {
            return Option<T&>::none();
        }

        return Option<T&>(data[count - 1]);
    }

    Option<const T&> last() const {
        if (count == 0) {
            return Option<const T&>::none();
        }
        return Option<const T&>(data[count - 1]);
    }

    Option<T&> first() {
        if (count == 0) {
            return Option<T&>::none();
        }

        return Option<T&>(data[0]);
    }

    Option<const T&> first() const {
        if (count == 0) {
            return Option<const T&>::none();
        }

        return Option<const T&>(data[0]);
    }

    [[nodiscard]] usize size() const {
        return count;
    }

    [[nodiscard]] usize capacity() const {
        return cap;
    }

    [[nodiscard]] bool empty() const {
        return count == 0;
    }

    void clear() {
        count = 0;
    }

    void reserve(usize newCapacity) {
        if (newCapacity <= cap) {
            return;
        }

        T* newData = new T[newCapacity];

        for (usize i = 0; i < count; i++) {
            newData[i] = data[i];
        }

        delete[] data;

        data = newData;
        cap = newCapacity;
    }

    void resize(usize newSize) {
        if (newSize > cap) {
            reserve(newSize);
        }

        count = newSize;
    }

private:
    T* data;
    usize count;
    usize cap;

    void grow() {
        if (cap == 0) {
            reserve(4);
        }
        else {
            reserve(cap * 2);
        }
    }
};

#endif //AVERY_TYPES_H
