/*
* memoryTests.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: 
* Copyright (c) 2026 Max Van den Eynde
*/

#include "tests.h"
#include "drivers/pit.h"
#include "kernel/console.h"
#include "kernel/debug.h"
#include "kernel/memory/physicalMemory.h"

void tests::runAllMemoryTests() {
    pmmTest();
}

void tests::pmmTest() {
    out::setColor(Color::white, Color::blue);
    out::clear();
    out::println("TEST: PHYSICAL MEMORY MANAGER TEST");
    out::println("========================");
    out::print("PMM total: ");
    out::printNumber(pmm::getTotalPages());
    out::println("");
    out::print("PMM used: ");
    out::printNumber(pmm::getUsedPages());
    out::println("");
    out::print("PMM available: ");
    out::printNumber(pmm::getFreePages());
    out::println("");
    out::print("Free Memory: ");
    out::printNumber(pmm::getFreeMemory());
    out::println("");

    // Test allocation
    physAddr a = pmm::allocPage();
    physAddr b = pmm::allocPage();

    out::println("\n========================");
    out::println("POST ALLOC");
    out::print("PMM total: ");
    out::printNumber(pmm::getTotalPages());
    out::println("");
    out::print("PMM used: ");
    out::printNumber(pmm::getUsedPages());
    out::println("");
    out::print("PMM available: ");
    out::printNumber(pmm::getFreePages());
    out::println("");
    out::print("Free Memory: ");
    out::printNumber(pmm::getFreeMemory());
    out::println("");
    out::print("a address: ");
    out::printNumber(a);
    out::println("");
    out::print("b address: ");
    out::printNumber(b);
    out::println("");

    // Free + alloc again
    pmm::freePage(a);
    physAddr c = pmm::allocPage();

    out::println("\n========================");
    out::println("POST FREE");
    out::print("PMM total: ");
    out::printNumber(pmm::getTotalPages());
    out::println("");
    out::print("PMM used: ");
    out::printNumber(pmm::getUsedPages());
    out::println("");
    out::print("PMM available: ");
    out::printNumber(pmm::getFreePages());
    out::println("");
    out::print("Free Memory: ");
    out::printNumber(pmm::getFreeMemory());
    out::println("");
    out::print("a address: ");
    out::printNumber(a);
    out::println("");
    out::print("b address: ");
    out::printNumber(b);
    out::println("");
    out::print("c address: ");
    out::printNumber(c);
    out::println("");
    TEST_RESULT(a == c);

    time::wait(3000);
}
