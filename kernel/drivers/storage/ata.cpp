/*
* ata.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: ATA implementation
* Copyright (c) 2026 Max Van den Eynde
*/

#include "drivers/ata.h"

#include "types.h"
#include "io/io.h"
#include "kernel/debug.h"

namespace {
    constexpr u8 ATA_REG_DATA = 0x00;
    constexpr u8 ATA_REG_SECCOUNT0 = 0x02;
    constexpr u8 ATA_REG_LBA0 = 0x03;
    constexpr u8 ATA_REG_LBA1 = 0x04;
    constexpr u8 ATA_REG_LBA2 = 0x05;
    constexpr u8 ATA_REG_HDDEVSEL = 0x06;
    constexpr u8 ATA_REG_COMMAND = 0x07;
    constexpr u8 ATA_REG_STATUS = 0x07;

    constexpr u8 ATA_SR_BSY = 0x80;
    constexpr u8 ATA_SR_DF = 0x20;
    constexpr u8 ATA_SR_DRQ = 0x08;
    constexpr u8 ATA_SR_ERR = 0x01;

    constexpr u8 ATA_CMD_READ_PIO = 0x20;
    constexpr u8 ATA_CMD_WRITE_PIO = 0x30;
    constexpr u8 ATA_CMD_CACHE_FLUSH = 0xE7;
    constexpr u8 ATA_CMD_IDENTIFY = 0xEC;

    bool waitBSY(u16 ioBase) {
        for (usize i = 0; i < 100000; i++) {
            if (!(io::inb(ioBase + ATA_REG_STATUS) & ATA_SR_BSY)) return true;
        }
        return false;
    }

    bool waitDRQ(u16 ioBase) {
        for (usize i = 0; i < 100000; i++) {
            u8 status = io::inb(ioBase + ATA_REG_STATUS);

            if (status & (ATA_SR_ERR | ATA_SR_DF)) {
                return false;
            }

            if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) return true;
        }
        return false;
    }

    void selectDrive(u16 ioBase, u8 drive, u64 lba) {
        io::outb(ioBase + ATA_REG_HDDEVSEL,
                 static_cast<u8>(0xE0 | static_cast<u8>((drive & 1) << 4) | ((lba >> 24) & 0x0F)));
        io::wait();
    }

    bool identifyDrive(u16 ioBase, u8 drive, u16* identify) {
        selectDrive(ioBase, drive, 0);

        io::outb(ioBase + ATA_REG_SECCOUNT0, 0);
        io::outb(ioBase + ATA_REG_LBA0, 0);
        io::outb(ioBase + ATA_REG_LBA1, 0);
        io::outb(ioBase + ATA_REG_LBA2, 0);
        io::outb(ioBase + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

        io::wait();

        if (io::inb(ioBase + ATA_REG_STATUS) == 0) {
            return false;
        }

        if (!waitDRQ(ioBase)) {
            return false;
        }

        for (usize i = 0; i < 256; i++) {
            identify[i] = io::inw(ioBase + ATA_REG_DATA);
        }

        return true;
    }

    u64 sectorCountFromIdentify(const u16* id) {
        return static_cast<u64>(id[60]) | (static_cast<u64>(id[61]) << 16);
    }
}


ATADiskDevice::ATADiskDevice([[maybe_unused]] PCIDevice* controller, u16 ioBase, [[maybe_unused]] u16 ctrlBase,
                             u8 drive, u64 sectors) :
    BlockDevice("Ata Disk", sectors, 512),
    ioBase(ioBase),
    drive(drive) {
}

bool ATADiskDevice::readBlocks(u64 lba, u32 count, void* buffer) {
    return access(false, lba, count, buffer);
}

bool ATADiskDevice::writeBlocks(u64 lba, u32 count, const void* buffer) {
    return access(true, lba, count, const_cast<void*>(buffer));
}

bool ATADiskDevice::access(bool write, u64 lba, u32 count, void* buffer) {
    if (count == 0) {
        return true;
    }

    if (lba + count > blockCount()) {
        debug::error("Tried to access region ", lba, " + ", count, " which resulted in an overflow.");
        return false;
    }

    if (lba > 0x0FFFFFFF) {
        debug::error("For now LBA28 is not supported");
        return false;
    }

    auto* words = reinterpret_cast<u16*>(buffer);

    for (u32 sector = 0; sector < count; sector++) {
        u64 currentLBA = lba + sector;

        if (!waitBSY(ioBase)) {
            return false;
        }

        selectDrive(ioBase, drive, currentLBA);

        io::outb(ioBase + ATA_REG_SECCOUNT0, 1);
        io::outb(ioBase + ATA_REG_LBA0, currentLBA & 0xFF);
        io::outb(ioBase + ATA_REG_LBA1, (currentLBA >> 8) & 0xFF);
        io::outb(ioBase + ATA_REG_LBA2, (currentLBA >> 16) & 0xFF);

        io::outb(ioBase + ATA_REG_COMMAND, write ? ATA_CMD_WRITE_PIO : ATA_CMD_READ_PIO);

        if (write) {
            for (usize i = 0; i < 256; i++) {
                io::outw(ioBase + ATA_REG_DATA, words[sector * 256 + i]);
            }

            io::outb(ioBase + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);

            if (!waitBSY(ioBase)) {
                return false;
            }
        }
        else {
            for (usize i = 0; i < 256; i++) {
                words[sector * 256 + i] = io::inw(ioBase + ATA_REG_DATA);
            }
        }
    }

    return true;
}

bool ATADriver::probe(Device& device) {
    if (device.type() != DeviceType::PCI) {
        return false;
    }

    auto& pciDevice = static_cast<PCIDevice&>(device);

    return pciDevice.isClass(0x01, 0x01);
}

bool ATADriver::start(Device& device) {
    debug::log("Starting ATA Driver");

    auto& pciDevice = static_cast<PCIDevice&>(device);

    pciDevice.enableIOSpace();
    pciDevice.enableBusMastering();

    struct Channel {
        u16 ioBase;
        u16 ctrlBase;
    };

    Channel channels[] = {
        {0x1F0, 0x3F6},
        {0x170, 0x376}
    };

    bool foundAny = false;

    for (auto& channel : channels) {
        for (u8 drive = 0; drive < 2; drive++) {
            u16 identify[256]{};

            if (!identifyDrive(channel.ioBase, drive, identify)) continue;

            u64 sectors = sectorCountFromIdentify(identify);

            if (sectors == 0) {
                debug::warn("Found a drive with no sectors.");
            }

            auto* disk = new ATADiskDevice(
                &pciDevice,
                channel.ioBase,
                channel.ctrlBase,
                drive,
                sectors
            );

            disk->parent = &pciDevice;
            disk->driver = this;

            DeviceManager::registerDevice(disk);
            foundAny = true;
        }
    }

    if (!foundAny) {
        return false;
    }

    device.driver = this;
    setState(DriverState::Active);
    return true;
}

bool ATADriver::stop(Device& device) {
    if (device.driver == this) {
        device.driver = nullptr;
    }

    setState(DriverState::Stopping);
    return true;
}

void ata::registerDriver() {
    driver = new ATADriver();
    DriverManager::registerDriver(driver);
}
