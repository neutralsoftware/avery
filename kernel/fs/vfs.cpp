/*
* vfs.cpp
* As part of the Avery project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: Virtual File System implmenetation
* Copyright (c) 2026 Max Van den Eynde
*/

#include "fs/vfs.h"

#include "kernel/debug.h"

namespace {
    Vector<MountPoint>* mountStoragePointer = nullptr;

    Vector<MountPoint>& mountsStorage() {
        if (mountStoragePointer == nullptr) {
            mountStoragePointer = new Vector<MountPoint>();
        }

        return *mountStoragePointer;
    }

    bool isValidMountPath(const string& path) {
        if (path.empty()) return false;
        if (path[0].value() != '/') return false;

        return true;
    }

    bool pathStartsWithMount(const string& path, const string& mountPath) {
        if (mountPath == "/") return path.startsWith("/");

        if (!path.startsWith(mountPath)) return false;

        if (path.length() == mountPath.length()) return true;

        return path[mountPath.length()].value() == '/';
    }

    usize mountMatchLength(const string& path, const string& mountPath) {
        if (!pathStartsWithMount(path, mountPath)) return 0;

        return mountPath.length();
    }
}

namespace vfs {
    bool init() {
        mountsStorage().clear();
        return true;
    }


    bool mount(const string& path, FileSystem* fs, BlockDevice* device) {
        ASSERT(fs != nullptr);
        ASSERT(device != nullptr);
        ASSERT(isValidMountPath(path));

        for (usize i = 0; i < mountsStorage().size(); i++) {
            if (mountsStorage()[i].value().path == path) {
                debug::warn("Cannot mount the filesystem because a disk has already been mounted");
                return false;
            };
        }

        if (!fs->mount(device)) {
            debug::warn("The filesystem did not allow the mounting of the disk");
            return false;
        }

        MountPoint mountPoint;
        mountPoint.path = path;
        mountPoint.fs = fs;

        mountsStorage().push(mountPoint);
        return true;
    }

    bool unmount(const string& path) {
        for (usize i = 0; i < mountsStorage().size(); i++) {
            if (mountsStorage()[i].value().path != path) continue;

            FileSystem* fs = mountsStorage()[i].value().fs;

            if (fs && !fs->unmount()) {
                return false;
            }

            mountsStorage().remove(i);
            return true;
        }

        return false;
    }

    FileSystem* fileSystemForPath(const string& path) {
        FileSystem* best = nullptr;
        usize bestLength = 0;

        for (usize i = 0; i < mountsStorage().size(); i++) {
            const MountPoint& mountPoint = mountsStorage()[i].value();

            usize length = mountMatchLength(path, mountPoint.path);

            if (length > bestLength) {
                best = mountPoint.fs;
                bestLength = length;
            }
        }

        return best;
    }

    string relativePathForMount(const string& path, const string& mountPath) {
        if (mountPath == "/") return path;

        if (!pathStartsWithMount(path, mountPath)) return path;

        if (path.length() == mountPath.length()) return "/";

        string relative;

        for (usize i = mountPath.length(); i < path.length(); i++) {
            relative.append(path[i].value());
        }

        if (relative.length() == 0) return "/";

        return relative;
    }

    FileHandle* open(const string& path, VFSOpenMode mode) {
        FileSystem* fs = fileSystemForPath(path);

        ASSERT(fs != nullptr);

        string mounthPath = "/";

        usize bestLength = 0;
        for (usize i = 0; i < mountsStorage().size(); i++) {
            usize length = mountMatchLength(path, mountsStorage()[i].value().path);

            if (length > bestLength) {
                bestLength = length;
                mounthPath = mountsStorage()[i].value().path;
            }
        }

        string relativePath = relativePathForMount(path, mounthPath);

        VFSError error;
        FileHandle* handle = fs->open(relativePath, mode, error);

        if (!error.ok()) {
            debug::error("Error while opening file of type ", error.code);
            return nullptr;
        }

        return handle;
    }

    DirectoryHandle* openDirectory(const string& path) {
        FileSystem* fs = fileSystemForPath(path);

        ASSERT(fs != nullptr);

        string mounthPath = "/";

        usize bestLength = 0;
        for (usize i = 0; i < mountsStorage().size(); i++) {
            usize length = mountMatchLength(path, mountsStorage()[i].value().path);

            if (length > bestLength) {
                bestLength = length;
                mounthPath = mountsStorage()[i].value().path;
            }
        }

        string relativePath = relativePathForMount(path, mounthPath);

        VFSError error;
        DirectoryHandle* handle = fs->openDirectory(relativePath, error);

        if (!error.ok()) {
            debug::error("Error while opening directory of type ", error.code);
            return nullptr;
        }

        return handle;
    }

    bool stat(const string& path, VFSStat& outStat) {
        FileSystem* fs = fileSystemForPath(path);

        ASSERT(fs != nullptr);

        string mountPath = "/";
        usize bestLength = 0;
        for (usize i = 0; i < mountsStorage().size(); i++) {
            usize length = mountMatchLength(path, mountsStorage()[i].value().path);

            if (length > bestLength) {
                bestLength = length;
                mountPath = mountsStorage()[i].value().path;
            }
        }

        string relativePath = relativePathForMount(path, mountPath);

        VFSError error;
        bool result = fs->stat(relativePath, outStat, error);

        return result && error.ok();
    }

    bool exists(const string& path) {
        VFSStat st;
        return stat(path, st);
    }

    bool isFile(const string& path) {
        VFSStat st;

        if (!stat(path, st)) return false;

        return st.type == VFSNodeType::File;
    }

    bool isDirectory(const string& path) {
        VFSStat st;

        if (!stat(path, st)) return false;

        return st.type == VFSNodeType::Directory;
    }

    bool createFile(const string& path) {
        FileSystem* fs = fileSystemForPath(path);

        if (!fs)
            return false;

        string mountPath = "/";

        usize bestLength = 0;
        for (usize i = 0; i < mountsStorage().size(); i++) {
            usize length = mountMatchLength(path, mountsStorage()[i].value().path);

            if (length > bestLength) {
                bestLength = length;
                mountPath = mountsStorage()[i].value().path;
            }
        }

        string relativePath = relativePathForMount(path, mountPath);

        VFSError error;
        return fs->createFile(relativePath, error) && error.ok();
    }

    bool createDirectory(const string& path) {
        FileSystem* fs = fileSystemForPath(path);

        if (!fs)
            return false;

        string mountPath = "/";

        usize bestLength = 0;
        for (usize i = 0; i < mountsStorage().size(); i++) {
            usize length = mountMatchLength(path, mountsStorage()[i].value().path);

            if (length > bestLength) {
                bestLength = length;
                mountPath = mountsStorage()[i].value().path;
            }
        }

        string relativePath = relativePathForMount(path, mountPath);

        VFSError error;
        return fs->createDirectory(relativePath, error) && error.ok();
    }

    bool remove(const string& path) {
        FileSystem* fs = fileSystemForPath(path);

        if (!fs)
            return false;

        string mountPath = "/";

        usize bestLength = 0;
        for (usize i = 0; i < mountsStorage().size(); i++) {
            usize length = mountMatchLength(path, mountsStorage()[i].value().path);

            if (length > bestLength) {
                bestLength = length;
                mountPath = mountsStorage()[i].value().path;
            }
        }

        string relativePath = relativePathForMount(path, mountPath);

        VFSError error;
        return fs->remove(relativePath, error) && error.ok();
    }

    bool rename(const string& oldPath, const string& newPath) {
        FileSystem* oldFs = fileSystemForPath(oldPath);
        FileSystem* newFs = fileSystemForPath(newPath);

        if (!oldFs || !newFs)
            return false;

        if (oldFs != newFs)
            return false;

        string oldMountPath = "/";
        string newMountPath = "/";

        usize bestOldLength = 0;
        usize bestNewLength = 0;

        for (usize i = 0; i < mountsStorage().size(); i++) {
            usize oldLength = mountMatchLength(oldPath, mountsStorage()[i].value().path);

            if (oldLength > bestOldLength) {
                bestOldLength = oldLength;
                oldMountPath = mountsStorage()[i].value().path;
            }

            usize newLength = mountMatchLength(newPath, mountsStorage()[i].value().path);

            if (newLength > bestNewLength) {
                bestNewLength = newLength;
                newMountPath = mountsStorage()[i].value().path;
            }
        }

        string relativeOldPath = relativePathForMount(oldPath, oldMountPath);
        string relativeNewPath = relativePathForMount(newPath, newMountPath);

        VFSError error;
        return oldFs->rename(relativeOldPath, relativeNewPath, error) && error.ok();
    }

    Vector<MountPoint> mounts() {
        return mountsStorage();
    }
}
