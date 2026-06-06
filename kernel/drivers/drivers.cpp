/*
* drivers.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Driver manager
* Copyright (c) 2026 Max Van den Eynde
*/

#include <drivers/driver.h>

#include "drivers/ata.h"
#include "drivers/keyboard.h"
#include "drivers/pit.h"
#include "kernel/debug.h"

Device* DeviceManager::devices[MaxDevices] = {};
usize DeviceManager::s_deviceCount = 0;
Driver* DriverManager::drivers[MaxDrivers] = {};
usize DriverManager::driverCount = 0;

bool DeviceManager::registerDevice(Device* device) {
    if (!device) {
        return false;
    }

    for (usize i = 0; i < deviceCount(); i++) {
        if (devices[i] == device) {
            return true;
        }
    }

    if (deviceCount() >= MaxDevices) {
        return false;
    }

    devices[s_deviceCount++] = device;

    DriverManager::tryBind(*device);

    return true;
}

bool DriverManager::registerDriver(Driver* driver) {
    if (!driver) {
        return false;
    }

    for (usize i = 0; i < driverCount; i++) {
        if (drivers[i] == driver) {
            return true;
        }
    }

    if (driverCount >= MaxDrivers) {
        return false;
    }

    drivers[driverCount++] = driver;
    driver->setState(DriverState::Registered);

    tryBind(*driver);
    return true;
}

bool DriverManager::tryBind(Device& device) {
    if (device.driver) {
        return false;
    }

    for (usize i = 0; i < driverCount; i++) {
        Driver* driver = drivers[i];

        driver->setState(DriverState::Probing);
        if (!driver->probe(device)) {
            driver->setState(DriverState::Registered);
            continue;
        }
        if (!driver->start(device)) {
            driver->setState(DriverState::Failed);
            continue;
        }

        device.driver = driver;
        driver->setState(DriverState::Active);
        return true;
    }

    return false;
}

bool DriverManager::tryBind(Driver& driver) {
    for (usize i = 0; i < DeviceManager::deviceCount(); i++) {
        Device* device = DeviceManager::deviceAt(i);

        if (!device) continue;
        if (device->driver) continue;
        driver.setState(DriverState::Probing);
        if (!driver.probe(*device)) {
            driver.setState(DriverState::Registered);
            continue;
        }
        if (!driver.start(*device)) {
            driver.setState(DriverState::Failed);
            continue;
        }

        device->driver = &driver;
        driver.setState(DriverState::Active);
    }

    return true;
}

Device* DeviceManager::deviceAt(usize index) {
    if (index >= deviceCount()) {
        return nullptr;
    }

    return devices[index];
}

usize DeviceManager::deviceCount() {
    return s_deviceCount;
}

void DeviceManager::unregisterDevice(Device* device) {
    if (!device) {
        return;
    }

    DriverManager::unbind(*device);

    for (usize i = 0; i < deviceCount(); i++) {
        if (deviceAt(i) == device) {
            for (usize j = i; j + 1 < deviceCount(); j++) {
                devices[j] = devices[j + 1];
            }

            devices[s_deviceCount - 1] = nullptr;
            s_deviceCount--;
            return;
        }
    }
}

void DriverManager::unbind(Device& device) {
    Driver* driver = device.driver;

    if (!driver) return;

    driver->stop(device);
    driver->setState(DriverState::Registered);
    device.driver = nullptr;
}

void DriverManager::unregisterDriver(Driver* driver) {
    if (!driver) {
        return;
    }

    for (usize i = 0; i < DeviceManager::deviceCount(); i++) {
        Device* device = DeviceManager::deviceAt(i);

        if (device && device->driver == driver) {
            unbind(*device);
        }
    }

    for (usize i = 0; i < driverCount; i++) {
        if (drivers[i] == driver) {
            for (usize j = i; j + 1 < driverCount; j++) {
                drivers[j] = drivers[j + 1];
            }

            drivers[driverCount - 1] = nullptr;
            driverCount--;
            driver->setState(DriverState::Stopping);
            return;
        }
    }
}

void drivers::init() {
    time::registerDriver();
    time::registerDevice();
    keyboard::registerDriver();
    keyboard::registerDevice();
    ata::registerDriver();

    for (usize i = 0; i < DriverManager::driverCount; i++) {
        auto* driver = DriverManager::drivers[i];
        debug::log("Registered driver ", i, ": ", driver->name());
    }
}

BlockDevice* drivers::firstBlockDevice() {
    for (usize i = 0; i < DeviceManager::deviceCount(); i++) {
        auto* device = DeviceManager::deviceAt(i);
        if (device && device->type() == DeviceType::Block) {
            return reinterpret_cast<BlockDevice*>(device);
        }
    }

    return nullptr;
}
