/*
* drivers.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Driver manager
* Copyright (c) 2026 Max Van den Eynde
*/

#include <drivers/driver.h>

bool DeviceManager::registerDevice(Device* device) {
    if (!device) {
        return false;
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

    if (driverCount >= MaxDrivers) {
        return false;
    }

    drivers[driverCount++] = driver;

    tryBind(*driver);
    return true;
}

bool DriverManager::tryBind(Device& device) {
    if (device.driver) {
        return false;
    }

    for (usize i = 0; i < driverCount; i++) {
        Driver* driver = drivers[i];

        if (!driver->probe(device)) continue;
        if (!driver->start(device)) continue;

        device.driver = driver;
        return true;
    }

    return false;
}

bool DriverManager::tryBind(Driver& driver) {
    for (usize i = 0; i < DeviceManager::deviceCount(); i++) {
        Device* device = DeviceManager::deviceAt(i);

        if (!device) continue;
        if (device->driver) continue;
        if (!driver.probe(*device)) continue;
        if (!driver.start(*device)) continue;

        device->driver = &driver;
    }

    return true;
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
    device.driver = nullptr;
}
