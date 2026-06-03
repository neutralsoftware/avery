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

using DriverInitFunction = void (*)();

enum class DriverType {
    Generic
};

class Driver {
public:
    string name;
    DriverType type;

    DriverInitFunction init;
};

#endif //AVERY_DRIVER_H
