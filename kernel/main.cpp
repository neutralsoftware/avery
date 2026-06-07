#include <limine.h>

#include "tests.h"
#include "../include/kernel/console.h"
#include "core/regs.h"
#include "core/systems.h"
#include "drivers/driver.h"
#include "fs/fat32.h"
#include "fs/vfs.h"
#include "graphics/framebuffer.h"
#include "io/serial.h"
#include "kernel/debug.h"
#include "kernel/memory.h"
#include "kernel/exec/elf.h"
#include "kernel/exec/process.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
    .response = nullptr,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hddm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = nullptr
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = nullptr,
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

namespace {
    void printAvery() {
        out::println("The Avery Kernel");
        out::println("Version Alpha 1 (Development Edition)");
        out::println("Made by Max Van den Eynde in 2026");
    }

    string substring(const string& input, usize start, usize end) {
        string result;

        if (end > input.length()) {
            end = input.length();
        }

        for (usize i = start; i < end; i++) {
            result.append(input[i].value());
        }

        return result;
    }

    bool isSpace(char c) {
        return c == ' ' || c == '\t';
    }

    string trimCopy(string value) {
        value.trim();
        return value;
    }

    string joinPath(const string& left, const string& right) {
        if (right.empty()) {
            return left;
        }

        if (right[0].value() == '/') {
            return right;
        }

        if (left == "/") {
            string result = "/";
            result.append(right.cStr());
            return result;
        }

        string result = left;
        result.append("/");
        result.append(right.cStr());
        return result;
    }

    string parentPath(const string& path) {
        if (path == "/") {
            return "/";
        }

        usize end = path.length();
        while (end > 1 && path[end - 1].value() == '/') {
            end--;
        }

        usize slash = end;
        while (slash > 1 && path[slash - 1].value() != '/') {
            slash--;
        }

        if (slash <= 1) {
            return "/";
        }

        return substring(path, 0, slash - 1);
    }

    string normalizePath(const string& currentDirectory, const string& path) {
        string trimmed = trimCopy(path);

        if (trimmed.empty()) {
            return currentDirectory;
        }

        string current = currentDirectory;
        if (trimmed[0].value() == '/') {
            current = "/";
        }

        usize index = 0;

        while (index < trimmed.length()) {
            while (index < trimmed.length() && trimmed[index].value() == '/') {
                index++;
            }

            usize start = index;
            while (index < trimmed.length() && trimmed[index].value() != '/') {
                index++;
            }

            if (start == index) {
                continue;
            }

            string part = substring(trimmed, start, index);

            if (part == ".") {
                continue;
            }

            if (part == "..") {
                current = parentPath(current);
                continue;
            }

            current = joinPath(current, part);
        }

        return current;
    }

    bool removeRecursive(const string& path) {
        if (vfs::isDirectory(path)) {
            DirectoryHandle* dir = vfs::openDirectory(path);

            if (!dir) {
                return false;
            }

            DirectoryEntry entry;
            while (dir->readNext(entry)) {
                if (entry.name == "." || entry.name == "..") {
                    continue;
                }

                string child = joinPath(path, entry.name);

                if (!removeRecursive(child)) {
                    dir->close();
                    return false;
                }
            }

            dir->close();
        }

        return vfs::remove(path);
    }

    void listDirectory(const string& path) {
        DirectoryHandle* dir = vfs::openDirectory(path);

        if (!dir) {
            out::println("ls: could not open directory");
            return;
        }

        DirectoryEntry entry;
        while (dir->readNext(entry)) {
            out::println(entry.name, entry.type == VFSNodeType::Directory ? "/" : "");
        }

        dir->close();
    }

    void writeFile(const string& path, const string& text) {
        FileHandle* file = vfs::open(path, VFSOpenMode::Write | VFSOpenMode::Create | VFSOpenMode::Truncate);

        if (!file) {
            out::println("write: could not open file");
            return;
        }

        file->write(text.cStr(), text.length());
        file->flush();
        file->close();
    }

    void readFile(const string& path) {
        FileHandle* file = vfs::open(path, VFSOpenMode::Read);

        if (!file) {
            out::println("read: could not open file");
            return;
        }

        char buffer[129];

        while (true) {
            usize bytesRead = file->read(buffer, 128);

            if (bytesRead == 0) {
                break;
            }

            buffer[bytesRead] = '\0';
            out::print(buffer);
        }

        file->close();
        out::println();
    }

    bool parseQuotedRedirect(const string& input, string& text, string& path) {
        usize index = 0;

        while (index < input.length() && isSpace(input[index].value())) {
            index++;
        }

        if (index >= input.length() || input[index].value() != '"') {
            return false;
        }

        index++;
        usize start = index;

        while (index < input.length() && input[index].value() != '"') {
            index++;
        }

        if (index >= input.length()) {
            return false;
        }

        text = substring(input, start, index);
        index++;

        while (index < input.length() && isSpace(input[index].value())) {
            index++;
        }

        if (index >= input.length() || input[index].value() != '>') {
            return false;
        }

        index++;
        path = trimCopy(substring(input, index, input.length()));
        return !path.empty();
    }

    bool parseQuotedText(const string& input, string& text) {
        string trimmed = trimCopy(input);

        if (trimmed.length() >= 2 && trimmed[0].value() == '"' && trimmed[trimmed.length() - 1].value() == '"') {
            text = substring(trimmed, 1, trimmed.length() - 1);
            return true;
        }

        text = trimmed;
        return true;
    }

    void runCommand(const string& input, string& currentDirectory) {
        string command = trimCopy(input);

        if (command.empty()) {
            return;
        }

        if (command == "clear") {
            out::clear();
            return;
        }

        if (command == "avery") {
            printAvery();
            return;
        }

        if (command == "ls") {
            listDirectory(currentDirectory);
            return;
        }

        if (command.startsWith("ls ")) {
            command.removePrefix("ls ");
            listDirectory(normalizePath(currentDirectory, command));
            return;
        }

        if (command.startsWith("cd ")) {
            command.removePrefix("cd ");
            string path = normalizePath(currentDirectory, command);

            if (!vfs::isDirectory(path)) {
                out::println("cd: not a directory");
                return;
            }

            currentDirectory = path;
            return;
        }

        if (command.startsWith("new -d ")) {
            command.removePrefix("new -d ");

            if (!vfs::createDirectory(normalizePath(currentDirectory, command))) {
                out::println("new: could not create directory");
            }

            return;
        }

        if (command.startsWith("new ")) {
            command.removePrefix("new ");

            if (!vfs::createFile(normalizePath(currentDirectory, command))) {
                out::println("new: could not create file");
            }

            return;
        }

        if (command.startsWith("write ")) {
            command.removePrefix("write ");

            string text;
            string path;

            if (!parseQuotedRedirect(command, text, path)) {
                out::println("write: expected write \"text\" > file");
                return;
            }

            writeFile(normalizePath(currentDirectory, path), text);
            return;
        }

        if (command.startsWith("read ")) {
            command.removePrefix("read ");
            readFile(normalizePath(currentDirectory, command));
            return;
        }

        if (command.startsWith("del -R ")) {
            command.removePrefix("del -R ");

            if (!removeRecursive(normalizePath(currentDirectory, command))) {
                out::println("del: could not remove path");
            }

            return;
        }

        if (command.startsWith("del ")) {
            command.removePrefix("del ");

            if (!vfs::remove(normalizePath(currentDirectory, command))) {
                out::println("del: could not remove path");
            }

            return;
        }

        if (command.startsWith("echo ")) {
            command.removePrefix("echo ");

            string text;
            parseQuotedText(command, text);
            out::println(text);
            return;
        }

        if (command.startsWith("exec")) {
            command.removePrefix("exec ");

            string path = normalizePath(currentDirectory, command);
            debug::log("Exec command requested path ", path);
            if (!vfs::isDirectory(path)) {
                FileHandle* handle = vfs::open(path);
                debug::log("Exec opened file ", path, " size ", handle->size());
                auto data = new u8[handle->size()];
                handle->read(data, handle->size());
                elf::File elfFile;

                auto parseResult = elf::parse(data, handle->size(), &elfFile);

                debug::log("Exec ELF parse result ", static_cast<u32>(parseResult));
                ASSERT(parseResult == elf::Result::Ok);

                Process* process = Process::createFromElf(elfFile);

                if (!process) {
                    debug::error("Exec failed to create process for path ", path);
                    return;
                }

                debug::log("Exec switching to process ", process->pid, " entry ", process->executable.entry,
                           " stack top ", process->executable.userStackTop, " address space ",
                           process->addressSpace.pml4);
                core::enterUserMode(process->executable.entry, process->executable.userStackTop);
            }
        }

        out::setColor(Color::red, Color::black);
        out::println("Incorrect command, try again");
        out::setColor(Color::white, Color::black);
    }
}

extern "C" [[noreturn]] void _start() {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        while (true) {
            asm("hlt");
        }
    }

    memory::setHHDM(hddm_request);

    regs::enableSSE();
    core::initSystems(memmap_request);
    drivers::init();
    debug::log("Drivers initialized");

    vfs::mountRoot<FAT32FileSystem>();

    Framebuffer framebuffer = Framebuffer::createFromLimineRequest(framebuffer_request);
    out::initFramebufferConsole(framebuffer);

    out::setColor(Color::white, Color::black);
    out::clear();
    printAvery();

    string currentDirectory = "/";

    while (true) {
        out::print(currentDirectory);
        string input = in::getLine("> ");
        runCommand(input, currentDirectory);
    }
}
