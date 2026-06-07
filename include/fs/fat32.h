/*
* fat32.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: FAT32 systems
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_FAT32_H
#define AVERY_FAT32_H
#include "vfs.h"
#include "../types.h"

namespace fat32 {
    constexpr u32 EndOfChain = 0x0FFFFFF8;
    constexpr u32 BadCluster = 0x0FFFFFF7;
    constexpr u32 FreeCluster = 0x00000000;

    constexpr u8 AttributeReadOnly = 0x01;
    constexpr u8 AttributeHidden = 0x02;
    constexpr u8 AttributeSystem = 0x04;
    constexpr u8 AttributeVolumeID = 0x08;
    constexpr u8 AttributeDirectory = 0x10;
    constexpr u8 AttributeArchive = 0x20;
    constexpr u8 AttributeLongName = 0x0F;

    constexpr u8 EntryDeleted = 0xE5;

    struct BIOSParameterBlock {
        u8 jump[3];
        char oemName[8];

        u16 bytesPerSector;
        u8 sectorsPerCluster;
        u16 reservedSectorCount;
        u8 tableCount;
        u16 rootEntryCount;
        u16 totalSectors16;
        u8 mediaType;
        u16 tableSize16;
        u16 sectorsPerTrack;
        u16 headSideCount;
        u32 hiddenSectorCount;
        u32 totalSectors32;

        u32 tableSize32;
        u16 extendedFlags;
        u16 fatVersion;
        u32 rootCluster;
        u16 fsInfoSector;
        u16 backupBootSector;

        u8 reserved0[12];
        u8 driveNumber;
        u8 reserved1;
        u8 bootSignature;
        u32 volumeID;
        char volumeLabel[11];
        char fatTypeLabel[8];
    } __attribute__((__packed__));

    struct FSInfo {
        u32 leadSignature;
        u8 reserved0[480];
        u32 structureSignature;
        u32 freeClusterCount;
        u32 nextFreeCluster;
        u8 reserved1[12];
        u32 trailSignature;
    } __attribute__((packed));

    struct DirectoryEntryRaw {
        char name[11];
        u8 attributes;
        u8 ntReserved;
        u8 creationTimeTenths;
        u16 creationTime;
        u16 creationDate;
        u16 lastAccessDate;
        u16 firstClusterHigh;
        u16 writeTime;
        u16 writeDate;
        u16 firstClusterLow;
        u32 fileSize;
    } __attribute__((packed));

    struct LongDirectoryEntryRaw {
        u8 order;
        u16 name1[5];
        u8 attributes;
        u8 type;
        u8 checksum;
        u16 name2[6];
        u16 firstClusterLow;
        u16 name3[2];
    } __attribute__((packed));

    struct DirectoryEntryInfo {
        string name;
        u8 attributes = 0;

        u32 firstCluster = 0;
        u32 size = 0;

        u32 parentDirectoryCluster = 0;
        u32 directoryEntryCluster = 0;
        u32 directoryEntryOffset = 0;
        u32 directoryEntryIndex = 0;

        bool dirty = false;

        bool isDirectory() const {
            return attributes & AttributeDirectory;
        }

        bool isFile() const {
            return !isDirectory();
        }
    };
}

class FAT32FileSystem;

class FAT32VNode final : public VNode {
public:
    FAT32VNode(
        string name,
        VFSNodeType type,
        FAT32FileSystem* fs,
        u32 firstCluster,
        u64 size
    );

    bool stat(VFSStat& outStat) override;

    u32 firstCluster() const { return nodeFirstCluster; }
    u64 size() const { return nodeSize; }

    void setFirstCluster(u32 firstCluster);
    void setSize(u64 size);

    bool hasDirectoryEntry() const { return hasEntryInfo; }
    const fat32::DirectoryEntryInfo& directoryEntry() const { return entryInfo; }
    void setDirectoryEntry(const fat32::DirectoryEntryInfo& entry);

private:
    u32 nodeFirstCluster;
    u64 nodeSize;
    fat32::DirectoryEntryInfo entryInfo;
    bool hasEntryInfo = false;
};

class FAT32FileHandle final : public FileHandle {
public:
    FAT32FileHandle(FAT32VNode* node, VFSOpenMode mode);

    usize read(void* buffer, usize size) override;
    usize write(const void* buffer, usize size) override;

    bool seek(u64 offset) override;
    bool truncate(u64 size) override;

    u64 tell() const override;
    u64 size() const override;

    bool flush() override;
    bool close() override;

    bool dirty = false;

private:
    FAT32VNode* fatNode;
    u64 offset = 0;
    bool closed = false;
};

class FAT32DirectoryHandle final : public DirectoryHandle {
public:
    FAT32DirectoryHandle(
        FAT32FileSystem* fs,
        Vector<fat32::DirectoryEntryInfo> entries
    );

    bool readNext(DirectoryEntry& entry) override;
    bool rewind() override;
    bool close() override;

private:
    Vector<fat32::DirectoryEntryInfo> entries;
    usize index = 0;
    bool closed = false;
};

class FAT32FileSystem final : public FileSystem {
public:
    FAT32FileSystem() = default;
    ~FAT32FileSystem() override = default;

    string name() const override {
        return "FAT32";
    }

    bool mount(BlockDevice* device) override;
    bool unmount() override;

    bool isMounted() const override {
        return mounted;
    }

    bool isReadOnly() const override {
        return readOnly;
    }

    VNode* rootNode() override;

    FileHandle* open(const string& path, VFSOpenMode mode, VFSError& error) override;
    DirectoryHandle* openDirectory(const string& path, VFSError& error) override;

    bool stat(const string& path, VFSStat& outStat, VFSError& error) override;

    bool createFile(const string& path, VFSError& error) override;
    bool createDirectory(const string& path, VFSError& error) override;

    bool remove(const string& path, VFSError& error) override;
    bool rename(const string& oldPath, const string& newPath, VFSError& error) override;

    BlockDevice* device() const {
        return backingDevice;
    }

    u32 rootCluster() const { return bpb.rootCluster; }
    u32 bytesPerSector() const { return bpb.bytesPerSector; }
    u32 sectorsPerCluster() const { return bpb.sectorsPerCluster; }
    u32 clusterSize() const { return bpb.bytesPerSector * bpb.sectorsPerCluster; }

    u32 firstDataSector() const { return fatFirstDataSector; }
    u32 firstFATSector() const { return fatFirstSector; }

    u64 clusterToSector(u32 cluster) const;
    bool readCluster(u32 cluster, void* buffer);
    bool writeCluster(u32 cluster, const void* buffer);

    bool readFATEntry(u32 cluster, u32& nextCluster);
    bool writeFATEntry(u32 cluster, u32 value);

    bool readDirectory(u32 firstCluster, Vector<fat32::DirectoryEntryInfo>& entries);
    bool findEntry(const string& path, fat32::DirectoryEntryInfo& entry);
    bool shortNameExists(u32 directoryCluster, const char shortName[11]);

    bool allocateCluster(u32& outCluster);
    bool freeClusterChain(u32 firstCluster);
    bool extendClusterChain(u32 startCluster, u32& newCluster);

    bool zeroCluster(u32 cluster);

    bool getClusterForOffset(
        u32 firstCluster,
        u64 offset,
        bool allocateIfMissing,
        u32& outCluster
    );

    bool updateDirectoryEntry(const fat32::DirectoryEntryInfo& entry);
    bool writeDirectoryEntry(
        u32 directoryCluster,
        u32 entryOffset,
        const fat32::DirectoryEntryRaw& raw
    );

    bool createDirectoryEntry(
        u32 parentDirectoryCluster,
        const string& name,
        u8 attributes,
        fat32::DirectoryEntryInfo& outEntry
    );

    bool findFreeDirectoryEntry(
        u32 directoryCluster,
        u32& outEntryCluster,
        u32& outEntryOffset
    );

    bool findFreeDirectoryEntries(
        u32 directoryCluster,
        u32 neededEntries,
        u32& outEntryCluster,
        u32& outEntryOffset
    );

    bool syncFSInfo();

private:
    bool readBootSector();
    bool validateBootSector() const;

    bool readFSInfo();
    bool loadRootNode();

    bool readSectors(u64 sector, u32 count, void* buffer) const;
    bool writeSectors(u64 sector, u32 count, const void* buffer) const;

    Vector<string> splitPath(const string& path) const;

private:
    BlockDevice* backingDevice = nullptr;

    bool mounted = false;
    bool readOnly = true;

    fat32::BIOSParameterBlock bpb{};
    fat32::FSInfo fsInfo{};

    u32 fatFirstSector = 0;
    u32 fatFirstDataSector = 0;
    u32 totalSectors = 0;
    u32 totalClusters = 0;

    u16 signature = 0;

    FAT32VNode* root = nullptr;
};


#endif //AVERY_FAT32_H
