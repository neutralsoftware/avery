/*
* vfs.h
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Virtual Filesystem defintions
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef AVERY_VFS_H
#define AVERY_VFS_H
#include "../types.h"
#include "../drivers/driver.h"

enum class VFSNodeType {
    Unknown,
    File,
    Directory,
    Device,
    Symlink
};

enum class VFSOpenMode : u32 {
    Read = 1 << 0,
    Write = 1 << 1,
    Append = 1 << 2,
    Create = 1 << 3,
    Truncate = 1 << 4,
};

inline VFSOpenMode operator|(VFSOpenMode a, VFSOpenMode b) {
    return static_cast<VFSOpenMode>(static_cast<u32>(a) | static_cast<u32>(b));
}

struct VFSError {
    enum Code {
        None,
        NotFound,
        AlreadyExists,
        NotDirectory,
        IsDirectory,
        InvalidPath,
        PermissionDenied,
        ReadOnly,
        IOError,
        Unsupported,
        NoSpace,
    };

    Code code = None;

    bool ok() const {
        return code == None;
    }
};

namespace vfs {
    inline bool hasMode(VFSOpenMode mode, VFSOpenMode flag) {
        return static_cast<u32>(mode) & static_cast<u32>(flag);
    }
}

struct VFSStat {
    string name;
    VFSNodeType type = VFSNodeType::Unknown;

    u64 size = 0;
    u64 blockSize = 512;
    u64 createdTime = 0;
    u64 modifiedTime = 0;
    u64 accessedTime = 0;
};

class FileSystem;
class VNode;
class FileHandle;

class VNode {
public:
    VNode(string name, VFSNodeType type, FileSystem* fs) : nodeName(name), nodeType(type), ownerFs(fs) {
    }

    virtual ~VNode() = default;

    string name() const { return nodeName; }
    VFSNodeType type() const { return nodeType; }
    FileSystem* fileSystem() const { return ownerFs; }

    virtual bool stat(VFSStat& outStat) = 0;

protected:
    string nodeName;
    VFSNodeType nodeType;
    FileSystem* ownerFs;
};

class FileHandle {
public:
    FileHandle(VNode* node, VFSOpenMode mode) : handleNode(node), openMode(mode) {
    }

    virtual ~FileHandle() = default;

    VNode* node() const { return handleNode; }
    VFSOpenMode mode() const { return openMode; }

    virtual usize read(void* buffer, usize size) = 0;
    virtual usize write(const void* buffer, usize size) = 0;

    virtual bool seek(u64 offset) = 0;
    virtual bool truncate(u64 size) = 0;

    virtual u64 tell() const = 0;
    virtual u64 size() const = 0;

    virtual bool flush() = 0;
    virtual bool close() = 0;

protected:
    VNode* handleNode;
    VFSOpenMode openMode;
};

struct DirectoryEntry {
    string name;
    VFSNodeType type = VFSNodeType::Unknown;
    u64 size = 0;
};

class DirectoryHandle {
public:
    virtual ~DirectoryHandle() = default;

    virtual bool readNext(DirectoryEntry& entry) = 0;
    virtual bool rewind() = 0;
    virtual bool close() = 0;
};

class FileSystem {
public:
    virtual ~FileSystem() = default;

    virtual string name() const = 0;

    virtual bool mount(BlockDevice* device) = 0;
    virtual bool unmount() = 0;

    virtual bool isMounted() const = 0;
    virtual bool isReadOnly() const = 0;

    virtual VNode* rootNode() = 0;

    virtual FileHandle* open(const string& path, VFSOpenMode mode, VFSError& error) = 0;
    virtual DirectoryHandle* openDirectory(const string& path, VFSError& error) = 0;

    virtual bool stat(const string& path, VFSStat& outStat, VFSError& error) = 0;

    virtual bool createFile(const string& path, VFSError& error) = 0;
    virtual bool createDirectory(const string& path, VFSError& error) = 0;

    virtual bool remove(const string& path, VFSError& error) = 0;
    virtual bool rename(const string& oldPath, const string& newPath, VFSError& error) = 0;
};

struct MountPoint {
    string path;
    FileSystem* fs = nullptr;
};

namespace vfs {
    bool init();

    bool mount(const string& path, FileSystem* fs, BlockDevice* device);
    bool unmount(const string& path);

    FileSystem* fileSystemForPath(const string& path);
    string relativePathForMount(const string& path, const string& mountPath);

    FileHandle* open(const string& path, VFSOpenMode mode = VFSOpenMode::Read);
    DirectoryHandle* openDirectory(const string& path);

    bool stat(const string& path, VFSStat& outStat);

    bool exists(const string& path);
    bool isFile(const string& path);
    bool isDirectory(const string& path);

    bool createFile(const string& path);
    bool createDirectory(const string& path);

    bool remove(const string& path);
    bool rename(const string& oldPath, const string& newPath);

    Vector<MountPoint> mounts();
}


#endif //AVERY_VFS_H
