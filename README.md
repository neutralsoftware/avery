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
- [x] Halt loop
- [x] Basics of the kernel + GDT, IDT and interrupt handlers
- [x] PIC, timer and keyboard support
- [x] Stack tracing support

### Memory

- [x] Limine Memory map parsing
- [x] Physical memory manager
- [x] Virtual memory manager
- [x] Kernel heap (malloc, free)
- [x] Create classes and helpers with the heap

### Internal Driver Framework

- [x] Device objects
- [x] Driver registration
- [x] Driver lifecycle
- [ ] MMIO port I/O helpers
- [x] Block device abstraction
- [ ] Move to Limine 6 barely (APIC or x2APIC mapped to legacy PIC)

### Hardware discovery

- [ ] PCI enumeration
- [ ] PCI BAR support

### Storage

- [ ] ATA driver
- [ ] Block device layer
- [ ] VFS
- [ ] FAT32
- [ ] ext2
- [ ] Read & Write files

### Processes

- [ ] Create user mode
- [ ] Basic scheduler
- [ ] Read ELF executables
- [ ] System calls
- [ ] Create userland (shell + init process)

## Roadmap (for Beta)

### Beta 1

- [ ] Avery Kernel SDK
- [ ] Driver build system
- [ ] Driver templates
- [ ] Driver documentation
- [ ] Loadable modules
- [ ] Driver manifest format
- [ ] Sample PCI driver
- [ ] Sample block driver
- [ ] Sample character driver
- [ ] Stable kernel driver API

### Beta 2

- [ ] ACPI
- [ ] MADT parsing
- [ ] x2APIC and APIC
- [ ] APIC timer
- [ ] IPIs

### Beta 3

- [ ] xHCI
- [ ] USB enumeration
- [ ] USB keyboard
- [ ] USB mouse
- [ ] USB mass storage
- [ ] USB hubs

### Beta 4

- [ ] Ethernet driver (E1000 first)
- [ ] Ethernet layer
- [ ] ARP
- [ ] IPv4
- [ ] ICMP
- [ ] UCP
- [ ] TCP
- [ ] DNS
- [ ] DHCP

### Beta 5

- [ ] USB Bluetooth adapter support
- [ ] HCI layer
- [ ] L2CAP
- [ ] HID over Bluetooth
- [ ] Bluetooth keyboard and mouse

### Beta 6

- [ ] Intel HDA
- [ ] Audio Mixer
- [ ] PCM playback
- [ ] Basic Sound API

### Beta 7

- [ ] Better graphics abstraction
- [ ] Font renderer
- [ ] Mouse cursor
- [ ] Window manager
- [ ] Simple compositor
- [ ] Terminal app
- [ ] File manager

### Beta 8

- [ ] Port binutils
- [ ] Port Clang
- [ ] libc
- [ ] C examples
- [ ] Build programs for Avery

### Beta 9

- [ ] Self-hosting

### Beta 10

- [ ] Graphics API

### Beta 11

- [ ] Users/groups
- [ ] Permissions
- [ ] Process isolation
- [ ] ASLR
- [ ] NX
- [ ] Sandboxing
- [ ] Driver permissions/signing
