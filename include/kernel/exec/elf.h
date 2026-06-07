/*
* elf.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Executable and Linkable Format runner
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_ELF_H
#define AVERY_ELF_H
#include "../../types.h"

using elf64Addr = u64;
using elf64Offset = u64;
using elf64Half = u16;
using elf64Word = u32;
using elf64SWord = i32;
using elf64XWord = u64;
using elf64SXWord = i64;

namespace elf {
    constexpr u8 Magic[4] = {
        0x7F,
        'E',
        'L',
        'F'
    };

    constexpr u8 IdentificationSize = 16;

    constexpr u8 ClassNone = 0;
    constexpr u8 Class32 = 1;
    constexpr u8 Class64 = 2;

    constexpr u8 DataNone = 0;
    constexpr u8 DataLSB = 1;
    constexpr u8 DataMSB = 2;

    constexpr u8 VersionCurrent = 1;

    constexpr u8 OSABISYSV = 0;

    enum class Type : u16 {
        None = 0,
        Rel = 1,
        Exec = 2,
        Dyn = 3,
        Core = 4,
    };

    constexpr u16 Machinex86_64 = 62;

    enum class PhType : u32 {
        Null = 0,
        Load = 1,
        Dynamic = 2,
        Interp = 3,
        Note = 4,
        PHDR = 6,
        TLS = 7
    };

    enum class PhFlag : u32 {
        X = 1,
        W = 2,
        R = 4
    };

    struct Identification {
        u8 magic[4];
        u8 elfClass;
        u8 dataEncoding;
        u8 identVersion;
        u8 osABI;
        u8 ABIVersion;
        u8 padding[7];
    } __attribute__((packed));

    struct Header {
        Identification ident;
        elf64Half type;
        elf64Half machine;
        elf64Word version;
        elf64Addr entry;
        elf64Offset programHeaderOffset;
        elf64Offset sectionHeaderOffset;
        elf64Word flags;
        elf64Half headerSize;
        elf64Half programHeaderEntrySize;
        elf64Half programHeaderCount;
        elf64Half sectionHeaderEntrySize;
        elf64Half sectionHeaderCount;
        elf64Half sectionNameStringTableIndex;
    } __attribute__((packed));

    struct ProgramHeader {
        elf64Word type;
        elf64Word flags;
        elf64Offset offset;
        elf64Addr virtualAddress;
        elf64Addr physicalAddress;
        elf64XWord fileSize;
        elf64XWord memorySize;
        elf64XWord alignment;

        bool isLoadable() const;
        bool isReadable() const;
        bool isWritable() const;
        bool isExecutable() const;
    } __attribute__((packed));

    struct SectionHeader {
        elf64Word name;
        elf64Word type;
        elf64XWord flags;
        elf64Addr address;
        elf64Offset offset;
        elf64XWord size;
        elf64Word link;
        elf64Word info;
        elf64XWord addressAlignment;
        elf64XWord entrySize;
    } __attribute__((packed));

    enum class Result : u32 {
        Ok = 0,
        InvalidArgument,
        BadMagic,
        UnsupportedClass,
        UnsupportedEndianness,
        UnsupportedVersion,
        UnsupportedType,
        UnsupportedMachine,
        Malformed,
        OutOfBounds,
    };

    struct LoadInfo {
        elf64Addr entry;
        const ProgramHeader* programHeaders;
        elf64Half programHeaderCount;
    };

    struct File {
        const u8* data;
        usize size;
        const Header* header;

        const ProgramHeader* getProgramHeader(usize index) const;
        Result validateExecutable() const;
        Result getLoadInfo(LoadInfo* loadInfo) const;
    };

    bool isValidMagic(const u8* data, usize size);
    Result parse(const u8* data, usize size, File* outFile);
}


#endif //AVERY_ELF_H
