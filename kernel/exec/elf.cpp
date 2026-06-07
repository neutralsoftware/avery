/*
* elf.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Executable and Linkable Format implementation functions
* Copyright (c) 2026 Max Van den Eynde
*/

#include <kernel/exec/elf.h>

#include "fs/vfs.h"
#include "kernel/debug.h"

namespace elf {
    static bool rangeInBounds(usize offset, usize size, usize fileSize) {
        if (offset > fileSize) {
            return false;
        }

        if (size > fileSize - offset) {
            return false;
        }

        return true;
    }

    bool isValidMagic(const u8* data, usize size) {
        ASSERT(data != nullptr);

        if (size < sizeof(Identification)) {
            return false;
        }

        return data[0] == Magic[0]
            && data[1] == Magic[1]
            && data[2] == Magic[2]
            && data[3] == Magic[3];
    }

    bool ProgramHeader::isLoadable() const {
        return type == static_cast<elf64Word>(PhType::Load);
    }

    bool ProgramHeader::isReadable() const {
        return (flags & static_cast<elf64Word>(PhFlag::R)) != 0;
    }

    bool ProgramHeader::isWritable() const {
        return (flags & static_cast<elf64Word>(PhFlag::W)) != 0;
    }

    bool ProgramHeader::isExecutable() const {
        return (flags & static_cast<elf64Word>(PhFlag::X)) != 0;
    }

    const ProgramHeader* File::getProgramHeader(usize index) const {
        if (!data || !header) {
            return nullptr;
        }

        if (index >= header->programHeaderCount) {
            return nullptr;
        }

        usize tableOffset = static_cast<usize>(header->programHeaderOffset);
        usize entrySize = static_cast<usize>(header->programHeaderEntrySize);
        usize offset = tableOffset + index * entrySize;

        if (!rangeInBounds(offset, sizeof(ProgramHeader), size)) {
            return nullptr;
        }

        return reinterpret_cast<const ProgramHeader*>(data + offset);
    }

    Result File::validateExecutable() const {
        if (!data || !header) {
            return Result::InvalidArgument;
        }

        if (!isValidMagic(data, size)) {
            return Result::BadMagic;
        }

        if (header->ident.elfClass != Class64) {
            return Result::UnsupportedClass;
        }

        if (header->ident.dataEncoding != DataLSB) {
            return Result::UnsupportedEndianness;
        }

        if (header->ident.identVersion != VersionCurrent) {
            return Result::UnsupportedVersion;
        }

        if (header->version != VersionCurrent) {
            return Result::UnsupportedVersion;
        }

        if (header->type != static_cast<elf64Half>(Type::Exec)) {
            return Result::UnsupportedType;
        }

        if (header->machine != Machinex86_64) {
            return Result::UnsupportedMachine;
        }

        if (header->headerSize < sizeof(Header)) {
            return Result::Malformed;
        }

        if (header->programHeaderEntrySize < sizeof(ProgramHeader)) {
            return Result::Malformed;
        }

        if (header->programHeaderCount == 0) {
            return Result::Malformed;
        }

        usize programHeaderOffset = header->programHeaderOffset;
        usize programHeaderEntrySize = header->programHeaderEntrySize;
        usize programHeaderCount = header->programHeaderCount;

        if (programHeaderEntrySize != sizeof(ProgramHeader)) {
            return Result::Malformed;
        }

        if (programHeaderCount > (static_cast<usize>(-1) / programHeaderEntrySize)) {
            return Result::OutOfBounds;
        }

        usize programHeaderTableSize = programHeaderEntrySize * programHeaderCount;

        if (!rangeInBounds(programHeaderOffset, programHeaderTableSize, size)) {
            return Result::OutOfBounds;
        }

        for (usize i = 0; i < programHeaderCount; i++) {
            const ProgramHeader* ph = getProgramHeader(i);

            if (!ph) {
                return Result::OutOfBounds;
            }

            if (ph->isLoadable()) {
                if (ph->memorySize < ph->fileSize) {
                    return Result::Malformed;
                }

                if (!rangeInBounds(
                    ph->offset,
                    ph->fileSize,
                    size
                )) {
                    return Result::OutOfBounds;
                }

                if (ph->alignment != 0 && ph->alignment != 1) {
                    if ((ph->virtualAddress % ph->alignment) != (ph->offset % ph->alignment)) {
                        return Result::Malformed;
                    }
                }
            }
        }

        return Result::Ok;
    }

    Result File::getLoadInfo(LoadInfo* loadInfo) const {
        if (!loadInfo) {
            return Result::InvalidArgument;
        }

        Result result = validateExecutable();

        if (result != Result::Ok) {
            return result;
        }

        loadInfo->entry = header->entry;
        loadInfo->programHeaders =
            reinterpret_cast<const ProgramHeader*>(data + header->programHeaderOffset);
        loadInfo->programHeaderCount = header->programHeaderCount;

        return Result::Ok;
    }

    Result parse(const u8* data, usize size, File* outFile) {
        if (!data || !outFile) {
            return Result::InvalidArgument;
        }

        if (size < sizeof(Header)) {
            return Result::Malformed;
        }

        if (!isValidMagic(data, size)) {
            return Result::BadMagic;
        }

        const Header* header = reinterpret_cast<const Header*>(data);

        outFile->data = data;
        outFile->size = size;
        outFile->header = header;

        return outFile->validateExecutable();
    }
}
