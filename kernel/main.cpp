#include <limine.h>

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

static void putc(char c) {
    asm volatile("outb %0, $0xe9" : : "a"(c));
}

static void print(const char* s) {
    while (*s) putc(*s++);
}

extern "C" [[noreturn]] void _start() {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        while (true) {
            asm("hlt");
        }
    }

    print("Framebuffer acquired\n");

    if (framebuffer_request.response == nullptr
        || framebuffer_request.response->framebuffer_count < 1) {
        while (true) {
            asm("hlt");
        }
    }

    struct limine_framebuffer* framebuffer = framebuffer_request.response->framebuffers[0];

    volatile uint32_t* fb_ptr = static_cast<uint32_t*>(framebuffer->address);
    for (unsigned int y = 0; y < framebuffer->height; y++) {
        for (unsigned int x = 0; x < framebuffer->width; x++) {
            uint32_t nX = x * 255 / framebuffer->width;
            uint32_t nY = y * 255 / framebuffer->height;
            fb_ptr[y * (framebuffer->pitch / 4) + x] = (nY << 8) | nX;
        }
    }

    while (true) {
        asm("hlt");
    }
}
