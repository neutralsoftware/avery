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
#include "kernel/memory/virtualMemory.h"

void tests::runAllMemoryTests() {
    pmmTest();
    vmmTest();
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

void tests::vmmTest() {
    out::setColor(Color::white, Color::blue);
    out::clear();

    out::println("TEST: VIRTUAL MEMORY MANAGER TEST");
    out::println("========================");

    physAddr phys = pmm::allocPage();

    out::print("Allocated physical page: ");
    out::printNumber(phys);
    out::println("");

    TEST_RESULT(phys != 0);
    out::setColor(Color::white, Color::blue);

    virtAddr virt = 0xFFFF900000000000;

    bool mapped = vmm::mapPage(
        vmm::getKernelPml4(),
        virt,
        phys,
        vmm::FlagWritable
    );

    out::println("\n========================");
    out::println("POST MAP");

    out::print("Virtual address: ");
    out::printNumber(virt);
    out::println("");

    out::print("Physical address: ");
    out::printNumber(phys);
    out::println("");

    out::print("Mapped: ");
    out::printNumber(mapped);
    out::println("");

    TEST_RESULT(mapped);
    out::setColor(Color::white, Color::blue);


    volatile u64* ptr = (volatile u64*)virt;
    *ptr = 0x123456789ABCDEF0;

    u64 readValue = *ptr;

    out::println("\n========================");
    out::println("MEMORY WRITE TEST");

    out::print("Written value: ");
    out::printNumber(0x123456789ABCDEF0);
    out::println("");

    out::print("Read value: ");
    out::printNumber(readValue);
    out::println("");

    TEST_RESULT(readValue == 0x123456789ABCDEF0);
    out::setColor(Color::white, Color::blue);

    physAddr translated = vmm::virtToPhysical(vmm::getKernelPml4(), virt);

    out::println("\n========================");
    out::println("TRANSLATION TEST");

    out::print("Translated physical: ");
    out::printNumber(translated);
    out::println("");

    TEST_RESULT(translated == phys);
    out::setColor(Color::white, Color::blue);

    bool unmapped = vmm::unmapPage(vmm::getKernelPml4(), virt);

    out::println("\n========================");
    out::println("POST UNMAP");

    out::print("Unmapped: ");
    out::printNumber(unmapped);
    out::println("");

    TEST_RESULT(unmapped);
    out::setColor(Color::white, Color::blue);

    physAddr translatedAfterUnmap = vmm::virtToPhysical(vmm::getKernelPml4(), virt);

    out::print("Translated after unmap: ");
    out::printNumber(translatedAfterUnmap);
    out::println("");

    TEST_RESULT(translatedAfterUnmap == 0);
    out::setColor(Color::white, Color::blue);

    pmm::freePage(phys);

    out::println("\n========================");
    out::println("VMM TEST COMPLETE");

    time::wait(3000);
}
