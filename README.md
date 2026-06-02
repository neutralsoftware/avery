# The Avery Kernel

![GitHub contributors](https://img.shields.io/github/contributors/neutralsoftware/avery)
![GitHub last commit](https://img.shields.io/github/last-commit/neutralsoftware/avery)
![Tests](https://github.com/neutralsoftware/avery/actions/workflows/build.yml/badge.svg)
![GitHub Repo stars](https://img.shields.io/github/stars/neutralsoftware/avery)

The Avery Kernel is a modern open-source operating system kernel written in C++. It serves as the foundation of mOS and
represents the second major generation of Avery.

Avery aims to be a clean, educational, and extensible kernel, built from the ground up with modern software engineering
practices while remaining approachable to contributors and learners alike.

## Roadmap (to Alpha)

### Foundation

- [x] Serial output
- [x] Framebuffer abstraction + character output
- [x] Panic functions + Assertion system
- [x] Logging system (both visual + serial)
- [ ] Halt loop
- [ ] Basics of the kernel + GDT, IDT and interrupt handlers
- [ ] PIC, timer and keyboard support
- [ ] Stack tracing support

### Memory

- [ ] Limine Memory map parsion
- [ ] Physical memory manager
- [ ] Virtual memory manager
- [ ] Kernel heap (malloc, free)

### Hardware discovery

- [ ] PCI enumeration
- [ ] PCI BAR support

### Storage

- [ ] Driver system
- [ ] Filesystem VFS layer
- [ ] FAT32 support
- [ ] ext2 support
- [ ] Read from filesystem
- [ ] Write to filesystem

### Processes

- [ ] Create user mode
- [ ] Basic scheduler
- [ ] Read ELF executables
- [ ] System calls
- [ ] Create userland (shell + init process)

## Roadmap (for Beta)

### Toolchain

- [ ] Migrate a C compiler
- [ ] Create a libc port
- [ ] Build C examples

### Hardware

- [ ] USB support
- [ ] Ethernet / networking

### Graphics

- [ ] Graphics abstraction
- [ ] Font renderer
- [ ] Simple compositor