#include <new>
#include <stddef.h>
#include <stdint.h>
#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferSnapshot.h"

FramebufferSnapshot::FramebufferSnapshot() : surface(nullptr), pixels(nullptr), left(0), top(0), width(0), height(0) {}

FramebufferSnapshot::~FramebufferSnapshot() {
    discard();
}

bool
FramebufferSnapshot::capture(Framebuffer &target, int x, int y, int w, int h) {
    discard();
    if (w <= 0 || h <= 0 || x >= target.width() || y >= target.height()) {
        return false;
    }
    int64_t right = (int64_t)x + w;
    int64_t bottom = (int64_t)y + h;
    if (right <= 0 || bottom <= 0) {
        return false;
    }
    left = x < 0 ? 0 : x;
    top = y < 0 ? 0 : y;
    right = right > target.width() ? target.width() : right;
    bottom = bottom > target.height() ? target.height() : bottom;
    width = (int)right - left;
    height = (int)bottom - top;
    if ((size_t)width > SIZE_MAX / sizeof(uint32_t) / (size_t)height) {
        return false;
    }
    pixels = new (std::nothrow) uint32_t[(size_t)width * (size_t)height];
    if (!pixels) {
        return false;
    }
    surface = &target;
    target.readNativeArea(left, top, width, height, pixels);
    return true;
}

void
FramebufferSnapshot::restore() {
    if (!surface) {
        return;
    }
    surface->writeNativeArea(left, top, width, height, pixels);
    discard();
}

void
FramebufferSnapshot::discard() {
    delete[] pixels;
    pixels = nullptr;
    surface = nullptr;
    width = 0;
    height = 0;
}
