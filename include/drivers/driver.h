/*
* driver.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: ${FILE_DESCRIPTION}
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_DRIVER_H
#define AVERY_DRIVER_H
#include "../types.h"

enum class DeviceType {
    Unknown,
    Block,
    Character,
    PCI,
};

enum class DriverState {
    Created,
    Registered,
    Probing,
    Active,
    Failed,
    Stopping,
};

class Driver;

class Device {
public:
    virtual ~Device() = default;

    Device(string name, DeviceType type) : deviceName(name), deviceType(type) {
    }

    string name() const { return this->deviceName; }
    DeviceType type() const { return this->deviceType; }

    Driver* driver = nullptr;
    Device* parent = nullptr;

private:
    string deviceName;
    DeviceType deviceType;
};

class BlockDevice : public Device {
public:
    BlockDevice(string name, u64 blocks, u32 blockSize)
        : Device(name, DeviceType::Block), deviceBlocks(blocks), deviceBlockSize(blockSize) {
    }

    virtual bool readBlocks(u64 lba, u32 count, void* buffer) = 0;
    virtual bool writeBlocks(u64 lba, u32 count, const void* buffer) = 0;

    u64 blockCount() const { return deviceBlocks; }
    u64 blockSize() const { return deviceBlockSize; }

private:
    u64 deviceBlocks;
    u32 deviceBlockSize;
};

class CharacterDevice : public Device {
public:
    CharacterDevice(string name) : Device(name, DeviceType::Character) {
    }

    virtual int read(u8* buffer, usize size) = 0;
    virtual int write(const u8* buffer, usize size) = 0;
};

class PCIDevice : public Device {
public:
    PCIDevice(string name, DeviceType type) : Device(name, type) {
    }
};

class Driver {
public:
    Driver() = default;
    virtual ~Driver() = default;

    Driver(const Driver&) = delete;
    Driver& operator=(const Driver&) = delete;

    virtual string name() const = 0;

    virtual bool probe(Device& device) = 0;
    virtual bool start(Device& device) = 0;
    virtual bool stop(Device& device) = 0;

    DriverState state() const {
        return driverState;
    }

    void setState(DriverState state) {
        driverState = state;
    }

private:
    DriverState driverState = DriverState::Created;
};

class DeviceManager {
public:
    static bool registerDevice(Device* device);
    static void unregisterDevice(Device* device);

    static Device* deviceAt(usize index);
    static usize deviceCount();

private:
    static constexpr usize MaxDevices = 256;
    static Device* devices[MaxDevices];
    static usize s_deviceCount;
};

class DriverManager {
public:
    static bool registerDriver(Driver* driver);
    static void unregisterDriver(Driver* driver);

    static bool tryBind(Device& device);
    static bool tryBind(Driver& driver);
    static void unbind(Device& device);

private:
    static constexpr usize MaxDrivers = 128;
    static Driver* drivers[MaxDrivers];
    static usize driverCount;
};

#endif //AVERY_DRIVER_H
