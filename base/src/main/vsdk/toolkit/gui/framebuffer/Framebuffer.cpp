#include <new>
#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"

void
Framebuffer::readNativeArea(int x, int y, int w, int h, uint32_t *values) const {
    for (int row = 0; row < h; row++) {
        for (int column = 0; column < w; column++) {
            *values++ = nativePixel(x + column, y + row);
        }
    }
}

void
Framebuffer::writeNativeArea(int x, int y, int w, int h, const uint32_t *values) {
    for (int row = 0; row < h; row++) {
        for (int column = 0; column < w; column++) {
            putNativePixel(x + column, y + row, *values++);
        }
    }
}

// Scanline fill: for every row the crossings of the edges at the row's center, sorted, are paired into spans. The
// edges themselves are then drawn, so that thin and horizontal parts are not lost.
void
Framebuffer::fillPolygon(const int *xy, int count, uint32_t color) {
    if (!xy || count < 2) {
        return;
    }
    int top = xy[1];
    int bottom = xy[1];
    for (int i = 1; i < count; i++) {
        top = xy[2 * i + 1] < top ? xy[2 * i + 1] : top;
        bottom = xy[2 * i + 1] > bottom ? xy[2 * i + 1] : bottom;
    }
    int *crossings = count >= 3 ? new (std::nothrow) int[count] : nullptr;
    for (int y = top; crossings && y <= bottom; y++) {
        int found = 0;
        // Twice the row's center, so that the comparisons stay in integers.
        long center = 2L * y + 1;
        for (int i = 0; i < count; i++) {
            int j = (i + 1) % count;
            long y0 = 2L * xy[2 * i + 1];
            long y1 = 2L * xy[2 * j + 1];
            if ((y0 <= center) == (y1 <= center)) {
                continue;
            }
            long x0 = xy[2 * i];
            long x1 = xy[2 * j];
            crossings[found++] = (int)(x0 + (x1 - x0) * (center - y0) / (y1 - y0));
        }
        for (int i = 1; i < found; i++) {
            int value = crossings[i];
            int k = i - 1;
            while (k >= 0 && crossings[k] > value) {
                crossings[k + 1] = crossings[k];
                k--;
            }
            crossings[k + 1] = value;
        }
        for (int i = 0; i + 1 < found; i += 2) {
            fillRectangle(crossings[i], y, crossings[i + 1] - crossings[i] + 1, 1, color);
        }
    }
    delete[] crossings;
    for (int i = 0; i < count; i++) {
        int j = (i + 1) % count;
        line(xy[2 * i], xy[2 * i + 1], xy[2 * j], xy[2 * j + 1], color);
    }
}

void
Framebuffer::putImage(int x, int y, int width, int height, const uint32_t *rgb) {
    if (!rgb) {
        return;
    }
    for (int row = 0; row < height; row++) {
        for (int column = 0; column < width; column++) {
            pixel(x + column, y + row, rgb[(long)row * width + column]);
        }
    }
}

bool
Framebuffer::readImage(int, int, int, int, uint32_t *) const {
    return false;
}
