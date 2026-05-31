#include <limine.h>

#include "core/regs.h"
#include "graphics/framebuffer.h"
#include "io/serial.h"

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

    io::serialWrite("Hello, World!");

    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        while (true) {
            asm("hlt");
        }
    }

    Framebuffer framebuffer = Framebuffer::createFromLimineRequest(framebuffer_request);
    framebuffer.drawCharacter({10, 10}, Color::red, Color::black, 'A', 10.0);

    while (true) {
        asm("hlt");
    }
}
