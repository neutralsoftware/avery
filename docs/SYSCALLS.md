# System Calls for The Avery Kernel

The following table illustrates the system calls planned for the kernel.

## General Direction

A system call has the signature:

```c++
u64 syscall(u64 number, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5);
```

Where the registers are the arguments:

```asm
rax = syscall number

rdi = arg0
rsi = arg1
rdx = arg2
r10 = arg3
r8 = arg4
r9 = arg5

syscall

```

## System Calls

| Name          | Code  | Argument Signature                                                   | Description                                                        | Notes                                                                           |
|---------------|-------|----------------------------------------------------------------------|--------------------------------------------------------------------|---------------------------------------------------------------------------------|
| **Exit**      | `0x0` | `void exit(int code)`                                                | Terminates the execution of the current program                    | -                                                                               |
| **Write**     | `0x1` | `usize write(int fd, const void* buffer, usize count)`               | Writes to the specified file descriptor                            | For `fd = 1` is stdout and `fd = 2` is stderr                                   | 
| **Read**      | `0x2` | `usize read(int fd, void* buffer, usize count)`                      | Reads from the specified file descriptor                           | -                                                                               |
| **Memmap**    | `0x3` | `void* mmap(void* address, usize length, int protection, int flags)` | Maps a address with some flags                                     | -                                                                               |
| **Memunmap**  | `0x4` | `int munmap(void* address, usize)`                                   | Unmaps the address with some length                                | -                                                                               |
| **Get PID**   | `0x5` | `pid getpid(void)`                                                   | Obtains the current process ID                                     | -                                                                               |
| **Yield**     | `0x6` | `void yield(void)`                                                   | Forces the scheduler to switch tasks                               | -                                                                               |
| **Execute**   | `0x7` | `int exec(cstring path, string argv[])`                              | Loads and creates a new process from the specified path            | -                                                                               |
| **Open**      | `0x8` | `int open(cstring path, int flags)`                                  | Returns a file descriptor for a file                               | Refer to the flags further down in the document                                 |
| **Close**     | `0x9` | `int close(int fd)`                                                  | Closes a file descriptor                                           | -                                                                               |
| **Seek**      | `0xA` | `i64 seek(int fd, i64 offset, int whence)`                           | Moves the position of a file descriptor according to the arguments | Refer to the whence flags further down                                          |
| **Debug Log** | `0xB` | `void debug_log(cstring string)`                                     | Logs to the development console                                    | **Can only be used in development and should not be used by external programs** |

### Flags for `open`

```c++
#define FL_ACCESSMODE   00000003

#define FL_READONLY    00000000
#define FL_WRITEONLY    00000001
#define FL_READWRITE      00000002

#define FL_CREATE     00000100
#define FL_EXCL      00000200
#define FL_TRUNC     00001000
#define FL_APPEND    00002000
```

### Whence for `seek`

```c++
#define SEEK_START 0
#define SEEK_CURRENT 1
#define SEEK_END 2
```
