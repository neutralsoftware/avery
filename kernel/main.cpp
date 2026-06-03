#include <limine.h>

#include "../include/kernel/console.h"
#include "core/regs.h"
#include "core/systems.h"
#include "drivers/pit.h"
#include "graphics/framebuffer.h"
#include "io/serial.h"
#include "kernel/debug.h"
#include "kernel/memory.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

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

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

extern "C" [[noreturn]] void _start() {
    memory::setHHDM(hddm_request);

    regs::enableSSE();
    core::initSystems();

    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        while (true) {
            asm("hlt");
        }
    }

    Framebuffer framebuffer = Framebuffer::createFromLimineRequest(framebuffer_request);
    out::initFramebufferConsole(framebuffer);

    out::println("The Avery Kernel");
    out::println("Version Alpha 1 (Development Edition)");
    out::println("Made by Neutral Software in 2026");

    volatile u32 a = 12;
    volatile u32 b = 0;

    volatile u32 c = a / b;
    out::printNumber(reinterpret_cast<u32>(c));

    while (true) {
        asm("hlt");
    }
}
