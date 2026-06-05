/*
* ata.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: ATA device manager
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_ATA_H
#define AVERY_ATA_H
#include "driver.h"
#include "pci.h"

class ATADiskDevice final : public BlockDevice {
public:
    ATADiskDevice(PCIDevice* controller, u16 ioBase, u16 ctrlBase, u8 drive, u64 sectors);
    bool readBlocks(u64 lba, u32 count, void* buffer) override;
    bool writeBlocks(u64 lba, u32 count, const void* buffer) override;

private:
    bool access(bool write, u64 lba, u32 count, void* buffer);

    u16 ioBase;
    u8 drive;
};

class ATADriver final : public Driver {
public:
    string name() const override {
        return "ATA Driver - Avery";
    }

    bool probe(Device& device) override;
    bool start(Device& device) override;
    bool stop(Device& device) override;
};

namespace ata {
    static ATADriver* driver = nullptr;
    void registerDriver();
}

#endif //AVERY_ATA_H
