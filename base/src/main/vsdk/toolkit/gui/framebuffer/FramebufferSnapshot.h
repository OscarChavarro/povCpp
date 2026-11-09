#ifndef VITRAL_FRAMEBUFFER_SNAPSHOT_H
#define VITRAL_FRAMEBUFFER_SNAPSHOT_H

#include <stdint.h>

class Framebuffer;

// Saves exact native pixel values, including palette indexes, for one clipped rectangle.
class FramebufferSnapshot {
public:
    FramebufferSnapshot();
    ~FramebufferSnapshot();
    FramebufferSnapshot(const FramebufferSnapshot &) = delete;
    FramebufferSnapshot &operator=(const FramebufferSnapshot &) = delete;
    bool capture(Framebuffer &surface, int x, int y, int width, int height);
    void restore();
    void discard();

private:
    Framebuffer *surface;
    uint32_t *pixels;
    int left;
    int top;
    int width;
    int height;
};

#endif
