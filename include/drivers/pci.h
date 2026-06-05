/*
* pci.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Peripheral Component Interconnect definitions
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_PCI_H
#define AVERY_PCI_H
#include "driver.h"

enum class PCIBarType {
    None,
    IO,
    MMIO32,
    MMIO64,
};

struct PCIBar {
    PCIBarType type;

    u64 address;
    u64 size;

    bool prefetchable;
};

class PCIDevice : public Device {
public:
    PCIDevice(string name, u8 bus, u8 slot, u8 function);

    [[nodiscard]] u8 bus() const;
    [[nodiscard]] u8 slot() const;
    [[nodiscard]] u8 function() const;

    [[nodiscard]] u16 vendorID() const;
    [[nodiscard]] u16 deviceID() const;

    [[nodiscard]] PCIBar bar(u8 index) const;

    [[nodiscard]] u8 classCode() const;
    [[nodiscard]] u8 subclass() const;
    [[nodiscard]] u8 progIF() const;
    [[nodiscard]] u8 revisionID() const;

    [[nodiscard]] u8 interruptLine() const;
    [[nodiscard]] u8 interruptPin() const;

    [[nodiscard]] bool isClass(u8 classCode, u8 subClass) const;
    [[nodiscard]] bool isVendorDevice(u16 vendor, u16 device) const;

    void enableIOSpace() const;
    void enableMemorySpace() const;
    void enableBusMastering() const;

    void createMMIOInterface(u8 barIndex);
    [[nodiscard]] mmio::Interface* mmioInterface() const;

private:
    u8 pciBus;
    u8 pciSlot;
    u8 pciFunction;

    u16 pciVendorID;
    u16 pciDeviceID;

    u8 pciClassCode;
    u8 pciSubClass;
    u8 pciProgIF;
    u8 pciRevisionID;

    u8 pciInterruptLine;
    u8 pciInterruptPin;

    PCIBar bars[6]{};

    [[nodiscard]] PCIBar readBAR(u8 index) const;

    mmio::Interface* interface;
};

namespace pci {
    constexpr u16 ConfigAddress = 0xCF8;
    constexpr u16 ConfigData = 0xCFC;

    inline u32 configAddress(u8 bus, u8 slot, u8 function, u8 offset) {
        return (1u << 31)
            | (static_cast<u32>(bus) << 16)
            | (static_cast<u32>(slot) << 11)
            | (static_cast<u32>(function) << 8)
            | (static_cast<u32>(offset) & 0xFC);
    }

    u32 read32(u8 bus, u8 slot, u8 function, u8 offset);
    u16 read16(u8 bus, u8 slot, u8 function, u8 offset);
    u8 read8(u8 bus, u8 slot, u8 function, u8 offset);
    void write32(u8 bus, u8 slot, u8 function, u8 offset, u32 value);

    void checkBus(u8 bus);
    void checkSlot(u8 bus, u8 slot);
    void checkFunction(u8 bus, u8 slot, u8 function);

    void enumerateAndCreateDevices();
}

#endif //AVERY_PCI_H
