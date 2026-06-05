/*
* pci.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Discover and treatment of PCI devices
* Copyright (c) 2026 Max Van den Eynde
*/

#include <drivers/pci.h>

#include "io/io.h"
#include "kernel/debug.h"
#include "kernel/memory.h"

namespace pci {
    u32 read32(u8 bus, u8 slot, u8 function, u8 offset) {
        io::outl(ConfigAddress, configAddress(bus, slot, function, offset));
        return io::inl(ConfigAddress);
    }

    u16 read16(u8 bus, u8 slot, u8 function, u8 offset) {
        u32 value = read32(bus, slot, function, offset);
        return (value >> ((offset & 2) * 8)) & 0xFFFF;
    }

    u8 read8(u8 bus, u8 slot, u8 function, u8 offset) {
        u32 value = read32(bus, slot, function, offset);
        return (value >> ((offset & 1) * 8)) & 0xFF;
    }

    void write32(u8 bus, u8 slot, u8 function, u8 offset, u32 value) {
        io::outl(ConfigAddress, configAddress(bus, slot, function, offset));
        io::outl(ConfigAddress, value);
    }

    void enumerateAndCreateDevices() {
        for (u16 bus = 0; bus < 256; bus++) {
            checkBus(static_cast<u8>(bus));
        }
    }

    void checkBus(u8 bus) {
        for (u8 slot = 0; slot < 32; slot++) {
            checkSlot(bus, slot);
        }
    }

    void checkSlot(u8 bus, u8 slot) {
        u16 vendor = pci::read16(bus, slot, 0, 0x00);

        if (vendor == 0xFFFF) {
            return;
        }

        checkFunction(bus, slot, 0);

        u8 headerType = pci::read8(bus, slot, 0, 0x0E);

        if (headerType & 0x80) {
            for (u8 function = 1; function < 8; function++) {
                if (pci::read16(bus, slot, function, 0x00) != 0xFFFF) {
                    checkFunction(bus, slot, function);
                }
            }
        }
    }

    void checkFunction(u8 bus, u8 slot, u8 function) {
        auto* device = new PCIDevice("", bus, slot, function);
        debug::log("Registering ");

        DeviceManager::registerDevice(device);
    }
}

PCIDevice::PCIDevice(string name, u8 bus, u8 slot, u8 function) : Device(memory::move(name), DeviceType::PCI),
                                                                  pciBus(bus),
                                                                  pciSlot(slot), pciFunction(function) {
    pciVendorID = pci::read16(bus, slot, function, 0x00);
    pciDeviceID = pci::read16(bus, slot, function, 0x02);

    pciRevisionID = pci::read8(bus, slot, function, 0x08);
    pciProgIF = pci::read8(bus, slot, function, 0x09);
    pciSubClass = pci::read8(bus, slot, function, 0x0A);
    pciClassCode = pci::read8(bus, slot, function, 0x0B);

    pciInterruptLine = pci::read8(bus, slot, function, 0x3C);
    pciInterruptPin = pci::read8(bus, slot, function, 0x3D);

    interface = nullptr;

    for (u8 i = 0; i < 6; i++) {
        bars[i] = readBAR(i);
    }
}

u8 PCIDevice::bus() const {
    return pciBus;
}

u8 PCIDevice::slot() const {
    return pciSlot;
}

u8 PCIDevice::function() const {
    return pciFunction;
}

u16 PCIDevice::vendorID() const {
    return pciVendorID;
}

u16 PCIDevice::deviceID() const {
    return pciDeviceID;
}

u8 PCIDevice::classCode() const {
    return pciClassCode;
}

u8 PCIDevice::subclass() const {
    return pciSubClass;
}

u8 PCIDevice::progIF() const {
    return pciProgIF;
}

u8 PCIDevice::revisionID() const {
    return pciRevisionID;
}

u8 PCIDevice::interruptLine() const {
    return pciInterruptLine;
}

u8 PCIDevice::interruptPin() const {
    return pciInterruptPin;
}

PCIBar PCIDevice::bar(u8 index) const {
    if (index >= 6) {
        return PCIBar{
            PCIBarType::None,
            0,
            0,
            false
        };
    }
    return bars[index];
}

PCIBar PCIDevice::readBAR(u8 index) const {
    if (index >= 6) {
        return {
            PCIBarType::None,
            0,
            0,
            false
        };
    }

    u8 offset = 0x10 + index * 4;

    u32 original = pci::read32(pciBus, pciSlot, pciFunction, offset);

    if (original == 0) {
        return {
            PCIBarType::None,
            0,
            0,
            false
        };
    }

    pci::write32(pciBus, pciSlot, pciFunction, offset, 0xFFFFFFFF);
    u32 sizeMask = pci::read32(pciBus, pciSlot, pciFunction, offset);
    pci::write32(pciBus, pciSlot, pciFunction, offset, original);

    PCIBar result{};

    if (original & 0x1) {
        result.type = PCIBarType::IO;
        result.address = original & ~0x3ull;
        result.size = ~(sizeMask & ~0x3u) + 1;
        result.prefetchable = false;
        return result;
    }

    u8 memoryType = (original >> 1) & 0x3;
    result.prefetchable = original & (1 << 3);

    if (memoryType == 0x0) {
        result.type = PCIBarType::MMIO32;
        result.address = original & 0xFull;
        result.size = ~(sizeMask & ~0xFu) + 1;
        return result;
    }

    if (memoryType == 0x2) {
        u32 originalHigh = pci::read32(pciBus, pciSlot, pciFunction, offset + 4);

        pci::write32(pciBus, pciSlot, pciFunction, offset + 4, 0xFFFFFFFF);
        u32 sizeMaskHigh = pci::read32(pciBus, pciSlot, pciFunction, offset + 4);
        pci::write32(pciBus, pciSlot, pciFunction, offset + 4, originalHigh);

        result.type = PCIBarType::MMIO64;

        result.address = ((u64)originalHigh << 32)
            | (original & ~0xFull);

        u64 fullMask =
            ((u64)sizeMaskHigh << 32) |
            (sizeMask & ~0xFu);

        result.size = ~fullMask + 1;
        return result;
    }

    return {
        PCIBarType::None,
        0,
        0,
        false,
    };
}

void PCIDevice::createMMIOInterface(u8 barIndex) {
    PCIBar selected = bar(barIndex);

    if (selected.type != PCIBarType::MMIO32 && selected.type != PCIBarType::MMIO64) {
        interface = nullptr;
        return;
    }

    interface = new mmio::Interface(selected.address, selected.size);
}

mmio::Interface* PCIDevice::mmioInterface() const {
    return interface;
}

void PCIDevice::enableIOSpace() const {
    u16 command = pci::read16(pciBus, pciSlot, pciFunction, 0x4);
    command |= 1 << 0;
    pci::write32(pciBus, pciSlot, pciFunction, 0x4, command);
}

void PCIDevice::enableMemorySpace() const {
    u16 command = pci::read16(pciBus, pciSlot, pciFunction, 0x4);
    command |= 1 << 1;
    pci::write32(pciBus, pciSlot, pciFunction, 0x4, command);
}

void PCIDevice::enableBusMastering() const {
    u16 command = pci::read16(pciBus, pciSlot, pciFunction, 0x4);
    command |= 1 << 2;
    pci::write32(pciBus, pciSlot, pciFunction, 0x4, command);
}

bool PCIDevice::isClass(u8 classCode, u8 subClass) const {
    return pciClassCode == classCode && subClass == pciSubClass;
}

bool PCIDevice::isVendorDevice(u16 vendor, u16 device) const {
    return pciVendorID == vendor && device == pciDeviceID;
}
