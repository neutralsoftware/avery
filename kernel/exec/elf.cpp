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
        debug::log("Validating ELF file");

        if (!data || !header) {
            debug::error("ELF validation failed: missing data or header");
            return Result::InvalidArgument;
        }

        if (!isValidMagic(data, size)) {
            debug::error("ELF validation failed: bad magic");
            return Result::BadMagic;
        }

        if (header->ident.elfClass != Class64) {
            debug::warn("ELF validation failed: unsupported class ", header->ident.elfClass);
            return Result::UnsupportedClass;
        }

        if (header->ident.dataEncoding != DataLSB) {
            debug::warn("ELF validation failed: unsupported endianness ", header->ident.dataEncoding);
            return Result::UnsupportedEndianness;
        }

        if (header->ident.identVersion != VersionCurrent) {
            debug::warn("ELF validation failed: unsupported ident version ", header->ident.identVersion);
            return Result::UnsupportedVersion;
        }

        if (header->version != VersionCurrent) {
            debug::warn("ELF validation failed: unsupported header version ", header->version);
            return Result::UnsupportedVersion;
        }

        if (header->type != static_cast<elf64Half>(Type::Exec)) {
            debug::warn("ELF validation failed: unsupported type ", header->type);
            return Result::UnsupportedType;
        }

        if (header->machine != Machinex86_64) {
            debug::warn("ELF validation failed: unsupported machine ", header->machine);
            return Result::UnsupportedMachine;
        }

        if (header->headerSize < sizeof(Header)) {
            debug::error("ELF validation failed: header too small ", header->headerSize);
            return Result::Malformed;
        }

        if (header->programHeaderEntrySize < sizeof(ProgramHeader)) {
            debug::error("ELF validation failed: program header entry too small ", header->programHeaderEntrySize);
            return Result::Malformed;
        }

        if (header->programHeaderCount == 0) {
            debug::error("ELF validation failed: no program headers");
            return Result::Malformed;
        }

        usize programHeaderOffset = header->programHeaderOffset;
        usize programHeaderEntrySize = header->programHeaderEntrySize;
        usize programHeaderCount = header->programHeaderCount;

        if (programHeaderEntrySize != sizeof(ProgramHeader)) {
            debug::error("ELF validation failed: unexpected program header entry size ", programHeaderEntrySize);
            return Result::Malformed;
        }

        if (programHeaderCount > (static_cast<usize>(-1) / programHeaderEntrySize)) {
            debug::error("ELF validation failed: program header table size overflow");
            return Result::OutOfBounds;
        }

        usize programHeaderTableSize = programHeaderEntrySize * programHeaderCount;

        if (!rangeInBounds(programHeaderOffset, programHeaderTableSize, size)) {
            debug::error("ELF validation failed: program header table out of bounds offset ", programHeaderOffset,
                         " size ", programHeaderTableSize, " file size ", size);
            return Result::OutOfBounds;
        }

        debug::log("ELF header accepted: entry ", header->entry, " program headers ", programHeaderCount);

        for (usize i = 0; i < programHeaderCount; i++) {
            const ProgramHeader* ph = getProgramHeader(i);

            if (!ph) {
                debug::error("ELF validation failed: program header ", i, " out of bounds");
                return Result::OutOfBounds;
            }

            if (ph->isLoadable()) {
                debug::log("ELF loadable segment ", i, " offset ", ph->offset, " vaddr ", ph->virtualAddress,
                           " file size ", ph->fileSize, " memory size ", ph->memorySize, " flags ", ph->flags);

                if (ph->memorySize < ph->fileSize) {
                    debug::error("ELF validation failed: segment ", i, " memory size smaller than file size");
                    return Result::Malformed;
                }

                if (!rangeInBounds(
                    ph->offset,
                    ph->fileSize,
                    size
                )) {
                    debug::error("ELF validation failed: segment ", i, " file range out of bounds offset ",
                                 ph->offset, " size ", ph->fileSize, " file size ", size);
                    return Result::OutOfBounds;
                }

                if (ph->alignment != 0 && ph->alignment != 1) {
                    if ((ph->virtualAddress % ph->alignment) != (ph->offset % ph->alignment)) {
                        debug::error("ELF validation failed: segment ", i, " alignment mismatch alignment ",
                                     ph->alignment);
                        return Result::Malformed;
                    }
                }
            }
        }

        debug::log("ELF validation succeeded");
        return Result::Ok;
    }

    Result File::getLoadInfo(LoadInfo* loadInfo) const {
        if (!loadInfo) {
            debug::error("ELF load info failed: missing output pointer");
            return Result::InvalidArgument;
        }

        Result result = validateExecutable();

        if (result != Result::Ok) {
            debug::error("ELF load info failed: validation result ", static_cast<u32>(result));
            return result;
        }

        loadInfo->entry = header->entry;
        loadInfo->programHeaders =
            reinterpret_cast<const ProgramHeader*>(data + header->programHeaderOffset);
        loadInfo->programHeaderCount = header->programHeaderCount;

        debug::log("ELF load info ready: entry ", loadInfo->entry, " program headers ",
                   loadInfo->programHeaderCount);
        return Result::Ok;
    }

    Result parse(const u8* data, usize size, File* outFile) {
        debug::log("Parsing ELF buffer at ", data, " size ", size);

        if (!data || !outFile) {
            debug::error("ELF parse failed: invalid argument data ", data, " out file ", outFile);
            return Result::InvalidArgument;
        }

        if (size < sizeof(Header)) {
            debug::error("ELF parse failed: buffer too small ", size);
            return Result::Malformed;
        }

        if (!isValidMagic(data, size)) {
            debug::error("ELF parse failed: bad magic");
            return Result::BadMagic;
        }

        const Header* header = reinterpret_cast<const Header*>(data);

        outFile->data = data;
        outFile->size = size;
        outFile->header = header;

        debug::log("ELF parse created file view: entry ", header->entry, " program headers ",
                   header->programHeaderCount);
        return outFile->validateExecutable();
    }
}
