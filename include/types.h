/*
* types.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Type definition for the Avery Kernel
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_TYPES_H
#define AVERY_TYPES_H

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
using usize = decltype(sizeof(0));

using i8 = char;
using i16 = short;
using i32 = int;
using i64 = long long;

using string = const char*;


#endif //AVERY_TYPES_H
