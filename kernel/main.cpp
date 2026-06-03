#include <limine.h>

#include "../include/kernel/console.h"
#include "core/regs.h"
#include "core/systems.h"
#include "drivers/pit.h"
#include "graphics/framebuffer.h"
#include "io/serial.h"
#include "kernel/debug.h"

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
    .response = nullptr,
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

extern "C" [[noreturn]] void _start() {
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
    time::wait(3000);
    out::println("Waiting for something...");

    while (true) {
        asm("hlt");
    }
}
