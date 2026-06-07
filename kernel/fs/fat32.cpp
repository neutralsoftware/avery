#include <fs/fat32.h>
#include <fs/vfs.h>

#include "kernel/debug.h"
#include "kernel/memory.h"

namespace {
    char toLower(char c) {
        if (c >= 'A' && c <= 'Z') {
            return static_cast<char>(c - 'A' + 'a');
        }

        return c;
    }

    char toUpper(char c) {
        if (c >= 'a' && c <= 'z') {
            return static_cast<char>(c - 'a' + 'A');
        }

        return c;
    }

    bool namesEqual(const string& a, const string& b) {
        if (a.length() != b.length()) {
            return false;
        }

        for (usize i = 0; i < a.length(); i++) {
            if (toLower(a[i].value()) != toLower(b[i].value())) {
                return false;
            }
        }

        return true;
    }

    bool isEndOfChain(u32 cluster) {
        return cluster >= fat32::EndOfChain;
    }

    bool isDotEntry(const string& name) {
        return name == "." || name == "..";
    }

    bool makeShortName(const string& name, char out[11]) {
        if (name.empty()) {
            return false;
        }

        for (usize i = 0; i < 11; i++) {
            out[i] = ' ';
        }

        usize dot = name.length();
        usize dotCount = 0;

        for (usize i = 0; i < name.length(); i++) {
            char c = name[i].value();

            if (c == '/') {
                return false;
            }

            if (c == '.') {
                dot = i;
                dotCount++;
            }
        }

        if (dotCount > 1) {
            return false;
        }

        usize baseLength = dot;
        usize extLength = dotCount == 0 ? 0 : name.length() - dot - 1;

        if (baseLength == 0 || baseLength > 8 || extLength > 3) {
            return false;
        }

        for (usize i = 0; i < baseLength; i++) {
            out[i] = toUpper(name[i].value());
        }

        for (usize i = 0; i < extLength; i++) {
            out[8 + i] = toUpper(name[dot + 1 + i].value());
        }

        return true;
    }

    bool isAliasChar(char c) {
        c = toUpper(c);
        return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    }

    usize lastDotIndex(const string& name) {
        usize dot = name.length();

        for (usize i = 0; i < name.length(); i++) {
            if (name[i].value() == '.') {
                dot = i;
            }
        }

        return dot;
    }

    void makeLFNAlias(const string& name, u8 number, char out[11]) {
        for (usize i = 0; i < 11; i++) {
            out[i] = ' ';
        }

        usize dot = lastDotIndex(name);
        usize baseEnd = dot == name.length() ? name.length() : dot;
        usize baseOut = 0;
        usize maxBase = number < 10 ? 6 : 5;

        for (usize i = 0; i < baseEnd && baseOut < maxBase; i++) {
            char c = name[i].value();

            if (c == ' ' || c == '.') {
                continue;
            }

            out[baseOut++] = isAliasChar(c) ? toUpper(c) : '_';
        }

        if (baseOut == 0) {
            out[baseOut++] = '_';
        }

        while (baseOut < maxBase) {
            out[baseOut++] = '_';
        }

        out[baseOut++] = '~';

        if (number >= 10) {
            out[baseOut++] = static_cast<char>('0' + number / 10);
        }

        out[baseOut] = static_cast<char>('0' + number % 10);

        if (dot != name.length()) {
            usize extOut = 0;

            for (usize i = dot + 1; i < name.length() && extOut < 3; i++) {
                char c = name[i].value();

                if (c == ' ' || c == '.') {
                    continue;
                }

                out[8 + extOut] = isAliasChar(c) ? toUpper(c) : '_';
                extOut++;
            }
        }
    }

    u8 shortNameChecksum(const char name[11]) {
        u8 sum = 0;

        for (usize i = 0; i < 11; i++) {
            sum = static_cast<u8>(((sum & 1) ? 0x80 : 0) + (sum >> 1) + static_cast<u8>(name[i]));
        }

        return sum;
    }

    u32 lfnEntryCount(const string& name) {
        return static_cast<u32>((name.length() + 12) / 13);
    }

    u16 lfnCharAt(const string& name, usize index) {
        if (index < name.length()) {
            return static_cast<u8>(name[index].value());
        }

        if (index == name.length()) {
            return 0x0000;
        }

        return 0xFFFF;
    }

    void fillLFNChars(u16* out, usize count, const string& name, usize& index) {
        for (usize i = 0; i < count; i++) {
            out[i] = lfnCharAt(name, index);
            index++;
        }
    }

    fat32::LongDirectoryEntryRaw makeLFNEntry(const string& name, u8 order, u8 checksum) {
        fat32::LongDirectoryEntryRaw entry{};
        usize index = static_cast<usize>(order - 1) * 13;

        entry.order = order;
        entry.attributes = fat32::AttributeLongName;
        entry.type = 0;
        entry.checksum = checksum;
        entry.firstClusterLow = 0;

        fillLFNChars(entry.name1, 5, name, index);
        fillLFNChars(entry.name2, 6, name, index);
        fillLFNChars(entry.name3, 2, name, index);

        return entry;
    }

    string shortNameToString(const fat32::DirectoryEntryRaw& entry) {
        char normalName[13];
        usize out = 0;

        usize baseEnd = 8;
        while (baseEnd > 0 && entry.name[baseEnd - 1] == ' ') {
            baseEnd--;
        }

        for (usize j = 0; j < baseEnd; j++) {
            normalName[out++] = toLower(entry.name[j]);
        }

        usize extEnd = 11;
        while (extEnd > 8 && entry.name[extEnd - 1] == ' ') {
            extEnd--;
        }

        if (extEnd > 8) {
            normalName[out++] = '.';

            for (usize j = 8; j < extEnd; j++) {
                normalName[out++] = toLower(entry.name[j]);
            }
        }

        normalName[out] = '\0';
        return string(normalName);
    }

    void appendLFNChars(string& out, const u16* chars, usize count) {
        for (usize i = 0; i < count; i++) {
            u16 c = chars[i];

            if (c == 0x0000) return;
            if (c == 0xFFFF) continue;

            out.append(static_cast<char>(c & 0xFF));
        }
    }

    string pathComponent(const string& path, usize start, usize end) {
        string result;

        for (usize i = start; i < end; i++) {
            result.append(path[i].value());
        }

        return result;
    }

    string basenameOf(const string& path) {
        if (path == "/") {
            return "/";
        }

        usize end = path.length();
        while (end > 1 && path[end - 1].value() == '/') {
            end--;
        }

        usize start = end;
        while (start > 0 && path[start - 1].value() != '/') {
            start--;
        }

        return pathComponent(path, start, end);
    }

    string parentOf(const string& path) {
        if (path == "/") {
            return "/";
        }

        usize end = path.length();
        while (end > 1 && path[end - 1].value() == '/') {
            end--;
        }

        usize slash = end;
        while (slash > 0 && path[slash - 1].value() != '/') {
            slash--;
        }

        if (slash <= 1) {
            return "/";
        }

        return pathComponent(path, 0, slash - 1);
    }

    void fillDotName(char out[11], bool parent) {
        for (usize i = 0; i < 11; i++) {
            out[i] = ' ';
        }

        out[0] = '.';
        if (parent) {
            out[1] = '.';
        }
    }

    void markEntryAndLongNameDeleted(u8* clusterData, u32 entryOffset) {
        auto* entries = reinterpret_cast<fat32::DirectoryEntryRaw*>(clusterData);
        usize index = entryOffset / sizeof(fat32::DirectoryEntryRaw);

        entries[index].name[0] = static_cast<char>(fat32::EntryDeleted);

        while (index > 0) {
            index--;

            if (entries[index].attributes != fat32::AttributeLongName) {
                break;
            }

            entries[index].name[0] = static_cast<char>(fat32::EntryDeleted);
        }
    }
}

bool FAT32FileSystem::readSectors(u64 sector, u32 count, void* buffer) const {
    return backingDevice != nullptr && backingDevice->readBlocks(sector, count, buffer);
}

bool FAT32FileSystem::writeSectors(u64 sector, u32 count, const void* buffer) const {
    return backingDevice != nullptr && backingDevice->writeBlocks(sector, count, buffer);
}

bool FAT32FileSystem::readBootSector() {
    u8* firstSector = new u8[512];

    if (!readSectors(0, 1, firstSector)) {
        delete[] firstSector;
        debug::error("Could not read initial sector of FAT32 disk");
        return false;
    }

    memory::copy(
        reinterpret_cast<u8*>(&bpb),
        firstSector,
        static_cast<int>(sizeof(fat32::BIOSParameterBlock))
    );
    signature = static_cast<u16>(firstSector[510] | (firstSector[511] << 8));
    delete[] firstSector;
    return true;
}

bool FAT32FileSystem::validateBootSector() const {
    if (bytesPerSector() != 512) {
        debug::warn("Bytes per sector is not 512");
        return false;
    }

    if (sectorsPerCluster() == 0 || (sectorsPerCluster() & (sectorsPerCluster() - 1)) != 0) {
        debug::warn("Sectors per cluster is corrupted or not aligned");
        return false;
    }

    if (bpb.reservedSectorCount == 0 || bpb.tableCount == 0 || bpb.tableSize32 == 0) {
        debug::warn("Some sector information is corrupted because is null");
        return false;
    }

    if (bpb.rootCluster < 2) {
        debug::warn("The root cluster is not a valid cluster");
        return false;
    }

    if (signature != 0xAA55) {
        debug::error("The disk signature is ", signature, " instead of the correct ", 0xAA55);
        return false;
    }

    return true;
}

bool FAT32FileSystem::readFSInfo() {
    fsInfo.freeClusterCount = 0xFFFFFFFF;
    fsInfo.nextFreeCluster = 2;

    if (bpb.fsInfoSector == 0) {
        return true;
    }

    u8* sector = new u8[512];
    if (!readSectors(bpb.fsInfoSector, 1, sector)) {
        delete[] sector;
        return true;
    }

    memory::copy(
        reinterpret_cast<u8*>(&fsInfo),
        sector,
        static_cast<int>(sizeof(fat32::FSInfo))
    );
    delete[] sector;

    if (fsInfo.leadSignature != 0x41615252 ||
        fsInfo.structureSignature != 0x61417272 ||
        fsInfo.trailSignature != 0xAA550000) {
        fsInfo.freeClusterCount = 0xFFFFFFFF;
        fsInfo.nextFreeCluster = 2;
    }

    return true;
}

bool FAT32FileSystem::loadRootNode() {
    root = new FAT32VNode("/", VFSNodeType::Directory, this, bpb.rootCluster, 0);
    return root != nullptr;
}

bool FAT32FileSystem::mount(BlockDevice* device) {
    if (mounted || device == nullptr) {
        debug::warn("The device is null or the disk has already been mounted");
        return false;
    }

    backingDevice = device;

    if (!readBootSector() || !validateBootSector()) {
        debug::warn("The boot sector could not have been read or the boot sector is wrong. Disk corrupted");
        backingDevice = nullptr;
        return false;
    }

    totalSectors = bpb.totalSectors16 != 0 ? bpb.totalSectors16 : bpb.totalSectors32;
    fatFirstSector = bpb.reservedSectorCount;
    fatFirstDataSector = bpb.reservedSectorCount + bpb.tableCount * bpb.tableSize32;

    if (totalSectors <= fatFirstDataSector) {
        backingDevice = nullptr;
        debug::warn("Total sectors is less than the first sector");
        return false;
    }

    totalClusters = (totalSectors - fatFirstDataSector) / bpb.sectorsPerCluster;

    if (totalClusters == 0) {
        backingDevice = nullptr;
        debug::warn("There are no clusters in the disk");
        return false;
    }

    readFSInfo();

    loadRootNode();

    readOnly = false;
    mounted = true;
    return true;
}

bool FAT32FileSystem::unmount() {
    if (!mounted) {
        return false;
    }

    syncFSInfo();
    mounted = false;
    backingDevice = nullptr;
    return true;
}

VNode* FAT32FileSystem::rootNode() {
    return root;
}

u64 FAT32FileSystem::clusterToSector(u32 cluster) const {
    return fatFirstDataSector + (cluster - 2) * bpb.sectorsPerCluster;
}

bool FAT32FileSystem::readCluster(u32 cluster, void* buffer) {
    if (cluster < 2) {
        return false;
    }

    return readSectors(clusterToSector(cluster), bpb.sectorsPerCluster, buffer);
}

bool FAT32FileSystem::writeCluster(u32 cluster, const void* buffer) {
    if (cluster < 2) {
        return false;
    }

    return writeSectors(clusterToSector(cluster), bpb.sectorsPerCluster, buffer);
}

bool FAT32FileSystem::readFATEntry(u32 cluster, u32& nextCluster) {
    u8 sector[512];

    u32 fatOffset = cluster * 4;
    u32 fatSector = fatFirstSector + fatOffset / bpb.bytesPerSector;
    u32 entryOffset = fatOffset % bpb.bytesPerSector;

    if (!readSectors(fatSector, 1, sector)) {
        debug::error("Could not read FAT sector");
        return false;
    }

    u32 rawEntry =
        u32(sector[entryOffset]) |
        (u32(sector[entryOffset + 1]) << 8) |
        (u32(sector[entryOffset + 2]) << 16) |
        (u32(sector[entryOffset + 3]) << 24);

    nextCluster = rawEntry & 0x0FFFFFFF;
    return true;
}

bool FAT32FileSystem::writeFATEntry(u32 cluster, u32 value) {
    u8 sector[512];
    u32 fatOffset = cluster * 4;
    u32 entryOffset = fatOffset % bpb.bytesPerSector;

    for (u8 table = 0; table < bpb.tableCount; table++) {
        u32 fatSector = fatFirstSector + table * bpb.tableSize32 + fatOffset / bpb.bytesPerSector;

        if (!readSectors(fatSector, 1, sector)) {
            return false;
        }

        u32 oldValue =
            u32(sector[entryOffset]) |
            (u32(sector[entryOffset + 1]) << 8) |
            (u32(sector[entryOffset + 2]) << 16) |
            (u32(sector[entryOffset + 3]) << 24);
        u32 rawValue = (oldValue & 0xF0000000) | (value & 0x0FFFFFFF);

        sector[entryOffset] = static_cast<u8>(rawValue & 0xFF);
        sector[entryOffset + 1] = static_cast<u8>((rawValue >> 8) & 0xFF);
        sector[entryOffset + 2] = static_cast<u8>((rawValue >> 16) & 0xFF);
        sector[entryOffset + 3] = static_cast<u8>((rawValue >> 24) & 0xFF);

        if (!writeSectors(fatSector, 1, sector)) {
            return false;
        }
    }

    return true;
}

bool FAT32FileSystem::zeroCluster(u32 cluster) {
    u32 size = clusterSize();
    u8* data = new u8[size];
    memory::set(data, static_cast<u8>(0), static_cast<int>(size));
    bool ok = writeCluster(cluster, data);
    delete[] data;
    return ok;
}

bool FAT32FileSystem::allocateCluster(u32& outCluster) {
    u32 start = fsInfo.nextFreeCluster >= 2 && fsInfo.nextFreeCluster < totalClusters + 2
                    ? fsInfo.nextFreeCluster
                    : 2;

    for (u32 i = 0; i < totalClusters; i++) {
        u32 cluster = 2 + ((start - 2 + i) % totalClusters);
        u32 value = 0;

        if (!readFATEntry(cluster, value)) {
            return false;
        }

        if (value != fat32::FreeCluster) {
            continue;
        }

        if (!writeFATEntry(cluster, 0x0FFFFFFF)) {
            return false;
        }

        if (!zeroCluster(cluster)) {
            writeFATEntry(cluster, fat32::FreeCluster);
            return false;
        }

        outCluster = cluster;
        fsInfo.nextFreeCluster = cluster + 1;

        if (fsInfo.nextFreeCluster >= totalClusters + 2) {
            fsInfo.nextFreeCluster = 2;
        }

        if (fsInfo.freeClusterCount != 0xFFFFFFFF && fsInfo.freeClusterCount > 0) {
            fsInfo.freeClusterCount--;
        }

        syncFSInfo();
        return true;
    }

    return false;
}

bool FAT32FileSystem::freeClusterChain(u32 firstCluster) {
    if (firstCluster < 2) {
        return true;
    }

    u32 current = firstCluster;

    while (current >= 2 && !isEndOfChain(current)) {
        u32 next = 0;

        if (!readFATEntry(current, next)) {
            return false;
        }

        if (!writeFATEntry(current, fat32::FreeCluster)) {
            return false;
        }

        if (fsInfo.freeClusterCount != 0xFFFFFFFF) {
            fsInfo.freeClusterCount++;
        }

        if (next == fat32::BadCluster || next == fat32::FreeCluster) {
            break;
        }

        current = next;
    }

    syncFSInfo();
    return true;
}

bool FAT32FileSystem::extendClusterChain(u32 startCluster, u32& newCluster) {
    if (startCluster < 2) {
        return allocateCluster(newCluster);
    }

    u32 current = startCluster;

    while (true) {
        u32 next = 0;

        if (!readFATEntry(current, next)) {
            return false;
        }

        if (isEndOfChain(next)) {
            break;
        }

        if (next == fat32::FreeCluster || next == fat32::BadCluster) {
            return false;
        }

        current = next;
    }

    if (!allocateCluster(newCluster)) {
        return false;
    }

    return writeFATEntry(current, newCluster);
}

bool FAT32FileSystem::getClusterForOffset(u32 firstCluster, u64 offset, bool allocateIfMissing, u32& outCluster) {
    if (firstCluster < 2) {
        return false;
    }

    u32 cluster = firstCluster;
    u64 index = offset / clusterSize();

    while (index > 0) {
        u32 next = 0;

        if (!readFATEntry(cluster, next)) {
            return false;
        }

        if (isEndOfChain(next)) {
            if (!allocateIfMissing) {
                return false;
            }

            if (!extendClusterChain(firstCluster, next)) {
                return false;
            }
        }

        if (next == fat32::FreeCluster || next == fat32::BadCluster) {
            return false;
        }

        cluster = next;
        index--;
    }

    outCluster = cluster;
    return true;
}

bool FAT32FileSystem::readDirectory(u32 firstCluster, Vector<fat32::DirectoryEntryInfo>& entries) {
    u32 size = clusterSize();
    u8* clusterData = new u8[size];
    u32 currentCluster = firstCluster;
    u32 globalIndex = 0;
    string lfn;

    while (true) {
        if (!readCluster(currentCluster, clusterData)) {
            delete[] clusterData;
            debug::error("Could not read directory entry");
            return false;
        }

        auto* rawEntries = reinterpret_cast<fat32::DirectoryEntryRaw*>(clusterData);

        for (usize i = 0; i < size / sizeof(fat32::DirectoryEntryRaw); i++) {
            auto& entry = rawEntries[i];

            if (entry.name[0] == 0x00) {
                delete[] clusterData;
                return true;
            }

            if (static_cast<u8>(entry.name[0]) == fat32::EntryDeleted) {
                lfn.clear();
                globalIndex++;
                continue;
            }

            if (entry.attributes == fat32::AttributeLongName) {
                auto& lfnEntry = *reinterpret_cast<fat32::LongDirectoryEntryRaw*>(&entry);
                string part;

                appendLFNChars(part, lfnEntry.name1, 5);
                appendLFNChars(part, lfnEntry.name2, 6);
                appendLFNChars(part, lfnEntry.name3, 2);

                lfn.prepend(part.cStr());
                globalIndex++;
                continue;
            }

            if (entry.attributes & fat32::AttributeVolumeID) {
                lfn.clear();
                globalIndex++;
                continue;
            }

            fat32::DirectoryEntryInfo info;
            info.name = lfn.empty() ? shortNameToString(entry) : lfn;
            info.firstCluster =
                (static_cast<u32>(entry.firstClusterHigh) << 16) | static_cast<u32>(entry.firstClusterLow);
            info.size = entry.fileSize;
            info.attributes = entry.attributes;
            info.parentDirectoryCluster = firstCluster;
            info.directoryEntryCluster = currentCluster;
            info.directoryEntryOffset = static_cast<u32>(i * sizeof(fat32::DirectoryEntryRaw));
            info.directoryEntryIndex = globalIndex;

            entries.push(info);
            lfn.clear();
            globalIndex++;
        }

        u32 nextCluster = 0;

        if (!readFATEntry(currentCluster, nextCluster)) {
            delete[] clusterData;
            debug::error("Could not read directory entry");
            return false;
        }

        if (isEndOfChain(nextCluster)) {
            delete[] clusterData;
            return true;
        }

        if (nextCluster == fat32::FreeCluster || nextCluster == fat32::BadCluster) {
            delete[] clusterData;
            debug::error("Could not read directory entry");
            return false;
        }

        currentCluster = nextCluster;
    }
}

Vector<string> FAT32FileSystem::splitPath(const string& path) const {
    Vector<string> parts;
    usize start = 0;

    while (start < path.length()) {
        while (start < path.length() && path[start].value() == '/') {
            start++;
        }

        usize end = start;
        while (end < path.length() && path[end].value() != '/') {
            end++;
        }

        if (end > start) {
            parts.push(pathComponent(path, start, end));
        }

        start = end;
    }

    return parts;
}

bool FAT32FileSystem::findEntry(const string& path, fat32::DirectoryEntryInfo& entry) {
    if (path.empty() || path == "/") {
        entry.name = "/";
        entry.attributes = fat32::AttributeDirectory;
        entry.firstCluster = bpb.rootCluster;
        entry.size = 0;
        entry.parentDirectoryCluster = bpb.rootCluster;
        entry.directoryEntryCluster = 0;
        entry.directoryEntryOffset = 0;
        return true;
    }

    Vector<string> parts = splitPath(path);
    u32 currentDirectory = bpb.rootCluster;

    for (usize i = 0; i < parts.size(); i++) {
        Vector<fat32::DirectoryEntryInfo> entries;

        if (!readDirectory(currentDirectory, entries)) {
            return false;
        }

        bool found = false;

        for (usize j = 0; j < entries.size(); j++) {
            fat32::DirectoryEntryInfo& candidate = entries[j].value();

            if (!namesEqual(candidate.name, parts[i].value())) {
                continue;
            }

            if (i + 1 == parts.size()) {
                entry = candidate;
                return true;
            }

            if (!candidate.isDirectory()) {
                return false;
            }

            currentDirectory = candidate.firstCluster;
            found = true;
            break;
        }

        if (!found) {
            return false;
        }
    }

    return false;
}

bool FAT32FileSystem::shortNameExists(u32 directoryCluster, const char shortName[11]) {
    u32 size = clusterSize();
    u8* clusterData = new u8[size];
    u32 currentCluster = directoryCluster;

    while (true) {
        if (!readCluster(currentCluster, clusterData)) {
            delete[] clusterData;
            return true;
        }

        auto* entries = reinterpret_cast<fat32::DirectoryEntryRaw*>(clusterData);

        for (usize i = 0; i < size / sizeof(fat32::DirectoryEntryRaw); i++) {
            u8 first = static_cast<u8>(entries[i].name[0]);

            if (first == 0x00) {
                delete[] clusterData;
                return false;
            }

            if (first == fat32::EntryDeleted || entries[i].attributes == fat32::AttributeLongName) {
                continue;
            }

            bool same = true;

            for (usize j = 0; j < 11; j++) {
                if (entries[i].name[j] != shortName[j]) {
                    same = false;
                    break;
                }
            }

            if (same) {
                delete[] clusterData;
                return true;
            }
        }

        u32 next = 0;

        if (!readFATEntry(currentCluster, next)) {
            delete[] clusterData;
            return true;
        }

        if (isEndOfChain(next)) {
            delete[] clusterData;
            return false;
        }

        currentCluster = next;
    }
}

bool FAT32FileSystem::writeDirectoryEntry(
    u32 directoryCluster,
    u32 entryOffset,
    const fat32::DirectoryEntryRaw& raw
) {
    u32 size = clusterSize();
    u8* clusterData = new u8[size];

    if (!readCluster(directoryCluster, clusterData)) {
        delete[] clusterData;
        return false;
    }

    memory::copy(
        clusterData + entryOffset,
        reinterpret_cast<const u8*>(&raw),
        static_cast<int>(sizeof(fat32::DirectoryEntryRaw))
    );

    bool ok = writeCluster(directoryCluster, clusterData);
    delete[] clusterData;
    return ok;
}

bool FAT32FileSystem::updateDirectoryEntry(const fat32::DirectoryEntryInfo& entry) {
    if (entry.directoryEntryCluster < 2) {
        return true;
    }

    u32 size = clusterSize();
    u8* clusterData = new u8[size];

    if (!readCluster(entry.directoryEntryCluster, clusterData)) {
        delete[] clusterData;
        return false;
    }

    auto* raw = reinterpret_cast<fat32::DirectoryEntryRaw*>(clusterData + entry.directoryEntryOffset);
    raw->attributes = entry.attributes;
    raw->firstClusterHigh = static_cast<u16>((entry.firstCluster >> 16) & 0xFFFF);
    raw->firstClusterLow = static_cast<u16>(entry.firstCluster & 0xFFFF);
    raw->fileSize = entry.isDirectory() ? 0 : entry.size;

    bool ok = writeCluster(entry.directoryEntryCluster, clusterData);
    delete[] clusterData;
    return ok;
}

bool FAT32FileSystem::findFreeDirectoryEntry(
    u32 directoryCluster,
    u32& outEntryCluster,
    u32& outEntryOffset
) {
    u32 size = clusterSize();
    u8* clusterData = new u8[size];
    u32 currentCluster = directoryCluster;

    while (true) {
        if (!readCluster(currentCluster, clusterData)) {
            delete[] clusterData;
            return false;
        }

        auto* entries = reinterpret_cast<fat32::DirectoryEntryRaw*>(clusterData);

        for (usize i = 0; i < size / sizeof(fat32::DirectoryEntryRaw); i++) {
            u8 first = static_cast<u8>(entries[i].name[0]);

            if (first == 0x00 || first == fat32::EntryDeleted) {
                outEntryCluster = currentCluster;
                outEntryOffset = static_cast<u32>(i * sizeof(fat32::DirectoryEntryRaw));
                delete[] clusterData;
                return true;
            }
        }

        u32 next = 0;

        if (!readFATEntry(currentCluster, next)) {
            delete[] clusterData;
            return false;
        }

        if (isEndOfChain(next)) {
            u32 newCluster = 0;

            if (!extendClusterChain(directoryCluster, newCluster)) {
                delete[] clusterData;
                return false;
            }

            outEntryCluster = newCluster;
            outEntryOffset = 0;
            delete[] clusterData;
            return true;
        }

        currentCluster = next;
    }
}

bool FAT32FileSystem::findFreeDirectoryEntries(
    u32 directoryCluster,
    u32 neededEntries,
    u32& outEntryCluster,
    u32& outEntryOffset
) {
    if (neededEntries == 0) {
        return false;
    }

    u32 size = clusterSize();
    u8* clusterData = new u8[size];
    u32 currentCluster = directoryCluster;

    while (true) {
        if (!readCluster(currentCluster, clusterData)) {
            delete[] clusterData;
            return false;
        }

        auto* entries = reinterpret_cast<fat32::DirectoryEntryRaw*>(clusterData);
        u32 runStart = 0;
        u32 runLength = 0;

        for (usize i = 0; i < size / sizeof(fat32::DirectoryEntryRaw); i++) {
            u8 first = static_cast<u8>(entries[i].name[0]);

            if (first == 0x00 || first == fat32::EntryDeleted) {
                if (runLength == 0) {
                    runStart = static_cast<u32>(i);
                }

                runLength++;

                if (runLength >= neededEntries) {
                    outEntryCluster = currentCluster;
                    outEntryOffset = runStart * sizeof(fat32::DirectoryEntryRaw);
                    delete[] clusterData;
                    return true;
                }

                continue;
            }

            runLength = 0;
        }

        u32 next = 0;

        if (!readFATEntry(currentCluster, next)) {
            delete[] clusterData;
            return false;
        }

        if (isEndOfChain(next)) {
            u32 newCluster = 0;

            if (!extendClusterChain(directoryCluster, newCluster)) {
                delete[] clusterData;
                return false;
            }

            outEntryCluster = newCluster;
            outEntryOffset = 0;
            delete[] clusterData;
            return true;
        }

        currentCluster = next;
    }
}

bool FAT32FileSystem::createDirectoryEntry(
    u32 parentDirectoryCluster,
    const string& name,
    u8 attributes,
    fat32::DirectoryEntryInfo& outEntry
) {
    char shortName[11];
    bool hasShortName = makeShortName(name, shortName);

    if (!hasShortName) {
        bool foundAlias = false;

        for (u8 i = 1; i < 100; i++) {
            makeLFNAlias(name, i, shortName);

            if (!shortNameExists(parentDirectoryCluster, shortName)) {
                foundAlias = true;
                break;
            }
        }

        if (!foundAlias) {
            return false;
        }
    }

    u32 longEntries = hasShortName ? 0 : lfnEntryCount(name);
    u32 neededEntries = longEntries + 1;

    u32 entryCluster = 0;
    u32 entryOffset = 0;

    if (!findFreeDirectoryEntries(parentDirectoryCluster, neededEntries, entryCluster, entryOffset)) {
        return false;
    }

    fat32::DirectoryEntryRaw raw{};
    memory::copy(reinterpret_cast<u8*>(raw.name), reinterpret_cast<const u8*>(shortName), 11);
    raw.attributes = attributes;

    u32 firstCluster = 0;

    if (attributes & fat32::AttributeDirectory) {
        if (!allocateCluster(firstCluster)) {
            return false;
        }

        fat32::DirectoryEntryRaw dot{};
        fillDotName(dot.name, false);
        dot.attributes = fat32::AttributeDirectory;
        dot.firstClusterHigh = static_cast<u16>((firstCluster >> 16) & 0xFFFF);
        dot.firstClusterLow = static_cast<u16>(firstCluster & 0xFFFF);

        fat32::DirectoryEntryRaw dotDot{};
        fillDotName(dotDot.name, true);
        dotDot.attributes = fat32::AttributeDirectory;
        dotDot.firstClusterHigh = static_cast<u16>((parentDirectoryCluster >> 16) & 0xFFFF);
        dotDot.firstClusterLow = static_cast<u16>(parentDirectoryCluster & 0xFFFF);

        u32 size = clusterSize();
        u8* directoryData = new u8[size];
        memory::set(directoryData, static_cast<u8>(0), static_cast<int>(size));
        memory::copy(directoryData, reinterpret_cast<const u8*>(&dot), static_cast<int>(sizeof(dot)));
        memory::copy(
            directoryData + sizeof(dot),
            reinterpret_cast<const u8*>(&dotDot),
            static_cast<int>(sizeof(dotDot))
        );

        if (!writeCluster(firstCluster, directoryData)) {
            delete[] directoryData;
            freeClusterChain(firstCluster);
            return false;
        }

        delete[] directoryData;
    }

    raw.firstClusterHigh = static_cast<u16>((firstCluster >> 16) & 0xFFFF);
    raw.firstClusterLow = static_cast<u16>(firstCluster & 0xFFFF);
    raw.fileSize = 0;

    if (longEntries > 0) {
        u32 size = clusterSize();
        u8* clusterData = new u8[size];

        if (!readCluster(entryCluster, clusterData)) {
            delete[] clusterData;

            if (firstCluster >= 2) {
                freeClusterChain(firstCluster);
            }

            return false;
        }

        u8 checksum = shortNameChecksum(shortName);

        for (u32 i = 0; i < longEntries; i++) {
            u32 order = longEntries - i;
            fat32::LongDirectoryEntryRaw lfn = makeLFNEntry(name, static_cast<u8>(order), checksum);

            if (i == 0) {
                lfn.order = static_cast<u8>(lfn.order | 0x40);
            }

            memory::copy(
                clusterData + entryOffset + i * sizeof(fat32::DirectoryEntryRaw),
                reinterpret_cast<const u8*>(&lfn),
                static_cast<int>(sizeof(fat32::LongDirectoryEntryRaw))
            );
        }

        memory::copy(
            clusterData + entryOffset + longEntries * sizeof(fat32::DirectoryEntryRaw),
            reinterpret_cast<const u8*>(&raw),
            static_cast<int>(sizeof(fat32::DirectoryEntryRaw))
        );

        bool ok = writeCluster(entryCluster, clusterData);
        delete[] clusterData;

        if (!ok) {
            if (firstCluster >= 2) {
                freeClusterChain(firstCluster);
            }

            return false;
        }
    }
    else if (!writeDirectoryEntry(entryCluster, entryOffset, raw)) {
        if (firstCluster >= 2) {
            freeClusterChain(firstCluster);
        }

        return false;
    }

    outEntry.name = name;
    outEntry.attributes = attributes;
    outEntry.firstCluster = firstCluster;
    outEntry.size = 0;
    outEntry.parentDirectoryCluster = parentDirectoryCluster;
    outEntry.directoryEntryCluster = entryCluster;
    outEntry.directoryEntryOffset = entryOffset + longEntries * sizeof(fat32::DirectoryEntryRaw);
    outEntry.directoryEntryIndex = outEntry.directoryEntryOffset / sizeof(fat32::DirectoryEntryRaw);

    return true;
}

bool FAT32FileSystem::syncFSInfo() {
    if (bpb.fsInfoSector == 0) {
        return true;
    }

    fsInfo.leadSignature = 0x41615252;
    fsInfo.structureSignature = 0x61417272;
    fsInfo.trailSignature = 0xAA550000;

    u8 sector[512];
    memory::set(sector, static_cast<u8>(0), 512);
    memory::copy(sector, reinterpret_cast<const u8*>(&fsInfo), static_cast<int>(sizeof(fat32::FSInfo)));
    return writeSectors(bpb.fsInfoSector, 1, sector);
}

FileHandle* FAT32FileSystem::open(const string& path, VFSOpenMode mode, VFSError& error) {
    error.code = VFSError::None;

    fat32::DirectoryEntryInfo entry;

    if (!findEntry(path, entry)) {
        if (!vfs::hasMode(mode, VFSOpenMode::Create)) {
            error.code = VFSError::NotFound;
            return nullptr;
        }

        if (!createFile(path, error)) {
            return nullptr;
        }

        if (!findEntry(path, entry)) {
            error.code = VFSError::IOError;
            return nullptr;
        }
    }

    if (entry.isDirectory()) {
        error.code = VFSError::IsDirectory;
        return nullptr;
    }

    if (vfs::hasMode(mode, VFSOpenMode::Truncate)) {
        if (entry.firstCluster >= 2 && !freeClusterChain(entry.firstCluster)) {
            error.code = VFSError::IOError;
            return nullptr;
        }

        entry.firstCluster = 0;
        entry.size = 0;

        if (!updateDirectoryEntry(entry)) {
            error.code = VFSError::IOError;
            return nullptr;
        }
    }

    auto* node = new FAT32VNode(entry.name, VFSNodeType::File, this, entry.firstCluster, entry.size);
    node->setDirectoryEntry(entry);

    auto* handle = new FAT32FileHandle(node, mode);

    if (vfs::hasMode(mode, VFSOpenMode::Append)) {
        handle->seek(node->size());
    }

    return handle;
}

DirectoryHandle* FAT32FileSystem::openDirectory(const string& path, VFSError& error) {
    error.code = VFSError::None;

    fat32::DirectoryEntryInfo entry;
    if (!findEntry(path, entry)) {
        error.code = VFSError::NotFound;
        return nullptr;
    }

    if (!entry.isDirectory()) {
        error.code = VFSError::NotDirectory;
        return nullptr;
    }

    Vector<fat32::DirectoryEntryInfo> entries;
    if (!readDirectory(entry.firstCluster, entries)) {
        error.code = VFSError::IOError;
        return nullptr;
    }

    return new FAT32DirectoryHandle(this, entries);
}

bool FAT32FileSystem::stat(const string& path, VFSStat& outStat, VFSError& error) {
    error.code = VFSError::None;

    fat32::DirectoryEntryInfo entry;
    if (!findEntry(path, entry)) {
        error.code = VFSError::NotFound;
        return false;
    }

    outStat.name = entry.name;
    outStat.type = entry.isDirectory() ? VFSNodeType::Directory : VFSNodeType::File;
    outStat.size = entry.size;
    outStat.blockSize = bytesPerSector();
    return true;
}

bool FAT32FileSystem::createFile(const string& path, VFSError& error) {
    error.code = VFSError::None;

    fat32::DirectoryEntryInfo existing;
    if (findEntry(path, existing)) {
        error.code = VFSError::AlreadyExists;
        return false;
    }

    fat32::DirectoryEntryInfo parent;
    string parentPath = parentOf(path);
    string name = basenameOf(path);

    if (!findEntry(parentPath, parent) || !parent.isDirectory()) {
        error.code = VFSError::NotDirectory;
        return false;
    }

    fat32::DirectoryEntryInfo created;
    if (!createDirectoryEntry(parent.firstCluster, name, fat32::AttributeArchive, created)) {
        error.code = VFSError::IOError;
        return false;
    }

    return true;
}

bool FAT32FileSystem::createDirectory(const string& path, VFSError& error) {
    error.code = VFSError::None;

    fat32::DirectoryEntryInfo existing;
    if (findEntry(path, existing)) {
        error.code = VFSError::AlreadyExists;
        return false;
    }

    fat32::DirectoryEntryInfo parent;
    string parentPath = parentOf(path);
    string name = basenameOf(path);

    if (!findEntry(parentPath, parent) || !parent.isDirectory()) {
        error.code = VFSError::NotDirectory;
        return false;
    }

    fat32::DirectoryEntryInfo created;
    if (!createDirectoryEntry(parent.firstCluster, name, fat32::AttributeDirectory, created)) {
        error.code = VFSError::IOError;
        return false;
    }

    return true;
}

bool FAT32FileSystem::remove(const string& path, VFSError& error) {
    error.code = VFSError::None;

    fat32::DirectoryEntryInfo entry;
    if (!findEntry(path, entry)) {
        error.code = VFSError::NotFound;
        return false;
    }

    if (entry.directoryEntryCluster < 2 || isDotEntry(entry.name)) {
        error.code = VFSError::InvalidPath;
        return false;
    }

    if (entry.isDirectory()) {
        Vector<fat32::DirectoryEntryInfo> entries;

        if (!readDirectory(entry.firstCluster, entries)) {
            error.code = VFSError::IOError;
            return false;
        }

        for (usize i = 0; i < entries.size(); i++) {
            if (!isDotEntry(entries[i].value().name)) {
                error.code = VFSError::PermissionDenied;
                return false;
            }
        }
    }

    u32 size = clusterSize();
    u8* clusterData = new u8[size];

    if (!readCluster(entry.directoryEntryCluster, clusterData)) {
        delete[] clusterData;
        error.code = VFSError::IOError;
        return false;
    }

    markEntryAndLongNameDeleted(clusterData, entry.directoryEntryOffset);

    bool wrote = writeCluster(entry.directoryEntryCluster, clusterData);
    delete[] clusterData;

    if (!wrote) {
        error.code = VFSError::IOError;
        return false;
    }

    if (entry.firstCluster >= 2 && !freeClusterChain(entry.firstCluster)) {
        error.code = VFSError::IOError;
        return false;
    }

    return true;
}

bool FAT32FileSystem::rename(const string& oldPath, const string& newPath, VFSError& error) {
    error.code = VFSError::None;

    fat32::DirectoryEntryInfo oldEntry;
    if (!findEntry(oldPath, oldEntry)) {
        error.code = VFSError::NotFound;
        return false;
    }

    fat32::DirectoryEntryInfo existing;
    if (findEntry(newPath, existing)) {
        error.code = VFSError::AlreadyExists;
        return false;
    }

    string newName = basenameOf(newPath);
    char shortName[11];
    if (!makeShortName(newName, shortName)) {
        error.code = VFSError::InvalidPath;
        return false;
    }

    fat32::DirectoryEntryInfo oldParent;
    fat32::DirectoryEntryInfo newParent;

    if (!findEntry(parentOf(oldPath), oldParent) || !findEntry(parentOf(newPath), newParent)) {
        error.code = VFSError::NotDirectory;
        return false;
    }

    if (oldParent.firstCluster == newParent.firstCluster) {
        u32 size = clusterSize();
        u8* clusterData = new u8[size];

        if (!readCluster(oldEntry.directoryEntryCluster, clusterData)) {
            delete[] clusterData;
            error.code = VFSError::IOError;
            return false;
        }

        markEntryAndLongNameDeleted(clusterData, oldEntry.directoryEntryOffset);
        auto* raw = reinterpret_cast<fat32::DirectoryEntryRaw*>(clusterData + oldEntry.directoryEntryOffset);
        memory::copy(reinterpret_cast<u8*>(raw->name), reinterpret_cast<const u8*>(shortName), 11);
        raw->attributes = oldEntry.attributes;
        raw->firstClusterHigh = static_cast<u16>((oldEntry.firstCluster >> 16) & 0xFFFF);
        raw->firstClusterLow = static_cast<u16>(oldEntry.firstCluster & 0xFFFF);
        raw->fileSize = oldEntry.isDirectory() ? 0 : oldEntry.size;

        bool ok = writeCluster(oldEntry.directoryEntryCluster, clusterData);
        delete[] clusterData;

        if (!ok) {
            error.code = VFSError::IOError;
            return false;
        }

        return true;
    }

    fat32::DirectoryEntryInfo newEntry;
    if (!createDirectoryEntry(newParent.firstCluster, newName, oldEntry.attributes, newEntry)) {
        error.code = VFSError::IOError;
        return false;
    }

    newEntry.firstCluster = oldEntry.firstCluster;
    newEntry.size = oldEntry.size;

    if (!updateDirectoryEntry(newEntry)) {
        error.code = VFSError::IOError;
        return false;
    }

    u32 size = clusterSize();
    u8* clusterData = new u8[size];

    if (!readCluster(oldEntry.directoryEntryCluster, clusterData)) {
        delete[] clusterData;
        newEntry.firstCluster = 0;
        updateDirectoryEntry(newEntry);
        remove(newPath, error);
        error.code = VFSError::IOError;
        return false;
    }

    markEntryAndLongNameDeleted(clusterData, oldEntry.directoryEntryOffset);

    bool removedOldEntry = writeCluster(oldEntry.directoryEntryCluster, clusterData);
    delete[] clusterData;

    if (!removedOldEntry) {
        newEntry.firstCluster = 0;
        updateDirectoryEntry(newEntry);
        remove(newPath, error);
        error.code = VFSError::IOError;
        return false;
    }

    return true;
}

FAT32VNode::FAT32VNode(string name, VFSNodeType type, FAT32FileSystem* fs, u32 firstCluster, u64 size)
    : VNode(name, type, fs), nodeFirstCluster(firstCluster), nodeSize(size) {
}

bool FAT32VNode::stat(VFSStat& outStat) {
    outStat.name = nodeName;
    outStat.type = nodeType;
    outStat.size = nodeSize;
    outStat.blockSize = 512;
    return true;
}

void FAT32VNode::setFirstCluster(u32 firstCluster) {
    nodeFirstCluster = firstCluster;

    if (hasEntryInfo) {
        entryInfo.firstCluster = firstCluster;
    }
}

void FAT32VNode::setSize(u64 size) {
    nodeSize = size;

    if (hasEntryInfo) {
        entryInfo.size = static_cast<u32>(size);
    }
}

void FAT32VNode::setDirectoryEntry(const fat32::DirectoryEntryInfo& entry) {
    entryInfo = entry;
    hasEntryInfo = true;
}

FAT32FileHandle::FAT32FileHandle(FAT32VNode* node, VFSOpenMode mode)
    : FileHandle(node, mode), fatNode(node) {
}

usize FAT32FileHandle::read(void* buffer, usize size) {
    if (closed || !vfs::hasMode(openMode, VFSOpenMode::Read) || buffer == nullptr) {
        return 0;
    }

    if (offset >= fatNode->size()) {
        return 0;
    }

    FAT32FileSystem* fs = static_cast<FAT32FileSystem*>(fatNode->fileSystem());
    usize remaining = size;
    u64 available = fatNode->size() - offset;

    if (remaining > available) {
        remaining = static_cast<usize>(available);
    }

    u8* out = static_cast<u8*>(buffer);
    usize totalRead = 0;
    u32 clusterSize = fs->clusterSize();
    u8* clusterData = new u8[clusterSize];

    while (remaining > 0) {
        u32 cluster = 0;

        if (!fs->getClusterForOffset(fatNode->firstCluster(), offset, false, cluster)) {
            break;
        }

        if (!fs->readCluster(cluster, clusterData)) {
            break;
        }

        u32 clusterOffset = static_cast<u32>(offset % clusterSize);
        usize chunk = clusterSize - clusterOffset;

        if (chunk > remaining) {
            chunk = remaining;
        }

        memory::copy(out + totalRead, clusterData + clusterOffset, static_cast<int>(chunk));

        offset += chunk;
        totalRead += chunk;
        remaining -= chunk;
    }

    delete[] clusterData;
    return totalRead;
}

usize FAT32FileHandle::write(const void* buffer, usize size) {
    if (closed || !vfs::hasMode(openMode, VFSOpenMode::Write) || buffer == nullptr) {
        return 0;
    }

    FAT32FileSystem* fs = static_cast<FAT32FileSystem*>(fatNode->fileSystem());

    if (fatNode->firstCluster() < 2) {
        u32 firstCluster = 0;

        if (!fs->allocateCluster(firstCluster)) {
            return 0;
        }

        fatNode->setFirstCluster(firstCluster);
        dirty = true;
    }

    const u8* in = static_cast<const u8*>(buffer);
    usize remaining = size;
    usize totalWritten = 0;
    u32 clusterSize = fs->clusterSize();
    u8* clusterData = new u8[clusterSize];

    while (remaining > 0) {
        u32 cluster = 0;

        if (!fs->getClusterForOffset(fatNode->firstCluster(), offset, true, cluster)) {
            break;
        }

        u32 clusterOffset = static_cast<u32>(offset % clusterSize);
        usize chunk = clusterSize - clusterOffset;

        if (chunk > remaining) {
            chunk = remaining;
        }

        if (chunk != clusterSize) {
            if (!fs->readCluster(cluster, clusterData)) {
                break;
            }
        } else {
            memory::set(clusterData, static_cast<u8>(0), static_cast<int>(clusterSize));
        }

        memory::copy(clusterData + clusterOffset, in + totalWritten, static_cast<int>(chunk));

        if (!fs->writeCluster(cluster, clusterData)) {
            break;
        }

        offset += chunk;
        totalWritten += chunk;
        remaining -= chunk;

        if (offset > fatNode->size()) {
            fatNode->setSize(offset);
        }

        dirty = true;
    }

    delete[] clusterData;
    return totalWritten;
}

bool FAT32FileHandle::seek(u64 newOffset) {
    if (closed) {
        return false;
    }

    offset = newOffset;
    return true;
}

bool FAT32FileHandle::truncate(u64 size) {
    if (closed || !vfs::hasMode(openMode, VFSOpenMode::Write)) {
        return false;
    }

    FAT32FileSystem* fs = static_cast<FAT32FileSystem*>(fatNode->fileSystem());
    u32 oldFirstCluster = fatNode->firstCluster();

    if (size == 0) {
        if (oldFirstCluster >= 2 && !fs->freeClusterChain(oldFirstCluster)) {
            return false;
        }

        fatNode->setFirstCluster(0);
        fatNode->setSize(0);
        offset = 0;
        dirty = true;
        return flush();
    }

    u64 lastByte = size - 1;
    u32 lastCluster = 0;

    if (oldFirstCluster < 2) {
        u32 firstCluster = 0;

        if (!fs->allocateCluster(firstCluster)) {
            return false;
        }

        fatNode->setFirstCluster(firstCluster);
    }

    if (!fs->getClusterForOffset(fatNode->firstCluster(), lastByte, true, lastCluster)) {
        return false;
    }

    u32 next = 0;
    if (!fs->readFATEntry(lastCluster, next)) {
        return false;
    }

    if (!isEndOfChain(next) && next >= 2) {
        if (!fs->freeClusterChain(next)) {
            return false;
        }
    }

    if (!fs->writeFATEntry(lastCluster, 0x0FFFFFFF)) {
        return false;
    }

    fatNode->setSize(size);

    if (offset > size) {
        offset = size;
    }

    dirty = true;
    return flush();
}

u64 FAT32FileHandle::tell() const {
    return offset;
}

u64 FAT32FileHandle::size() const {
    return fatNode->size();
}

bool FAT32FileHandle::flush() {
    if (closed) {
        return false;
    }

    if (!dirty) {
        return true;
    }

    if (!fatNode->hasDirectoryEntry()) {
        dirty = false;
        return true;
    }

    FAT32FileSystem* fs = static_cast<FAT32FileSystem*>(fatNode->fileSystem());
    bool ok = fs->updateDirectoryEntry(fatNode->directoryEntry());

    if (ok) {
        dirty = false;
    }

    return ok;
}

bool FAT32FileHandle::close() {
    if (closed) {
        return true;
    }

    bool ok = flush();
    closed = true;
    return ok;
}

FAT32DirectoryHandle::FAT32DirectoryHandle(
    [[maybe_unused]] FAT32FileSystem* fs,
    Vector<fat32::DirectoryEntryInfo> entries
) : entries(entries) {
}

bool FAT32DirectoryHandle::readNext(DirectoryEntry& entry) {
    if (closed) {
        return false;
    }

    while (index < entries.size()) {
        fat32::DirectoryEntryInfo& fatEntry = entries[index].value();
        index++;

        entry.name = fatEntry.name;
        entry.type = fatEntry.isDirectory() ? VFSNodeType::Directory : VFSNodeType::File;
        entry.size = fatEntry.size;
        return true;
    }

    return false;
}

bool FAT32DirectoryHandle::rewind() {
    if (closed) {
        return false;
    }

    index = 0;
    return true;
}

bool FAT32DirectoryHandle::close() {
    closed = true;
    return true;
}
