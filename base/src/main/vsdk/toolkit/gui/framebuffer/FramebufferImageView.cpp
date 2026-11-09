#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferImageView.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferPainter.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"

FramebufferImageView::FramebufferImageView(int w, int h, const uint32_t *rgb)
    : imageWidth(w > 0 ? w : 0), imageHeight(h > 0 ? h : 0) {
    if (rgb) {
        long count = (long)imageWidth * imageHeight;
        pixels.reserve(count);
        for (long i = 0; i < count; i++) {
            pixels.add(rgb[i]);
        }
    }
}

void
FramebufferImageView::draw(Framebuffer &surface, const FramebufferTheme &theme, bool) {
    FramebufferPainter painter(surface, theme);
    surface.beginDrawing();
    surface.fillRectangle(x, y, width, height, theme.face);
    int roomWidth = width - 4;
    int roomHeight = height - 4;
    if ((pixels.size() == 0) || roomWidth <= 0 || roomHeight <= 0) {
        surface.endDrawing();
        return;
    }
    // The largest size that keeps the proportions and fits, never larger than the image.
    int w = imageWidth;
    int h = imageHeight;
    if (w > roomWidth) {
        h = (int)((long)h * roomWidth / w);
        w = roomWidth;
    }
    if (h > roomHeight) {
        w = (int)((long)w * roomHeight / h);
        h = roomHeight;
    }
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;
    int left = x + 2 + (roomWidth - w) / 2;
    int topY = y + 2 + (roomHeight - h) / 2;
    painter.bevel(left - 2, topY - 2, w + 4, h + 4, false, false);
    if (w == imageWidth && h == imageHeight) {
        surface.putImage(left, topY, w, h, pixels.data());
    } else {
        java::ArrayList<uint32_t> reduced;
        reduced.reserve((long)w * h);
        for (int row = 0; row < h; row++) {
            long sourceRow = (long)row * imageHeight / h;
            for (int column = 0; column < w; column++) {
                reduced.add(pixels[sourceRow * imageWidth + (long)column * imageWidth / w]);
            }
        }
        surface.putImage(left, topY, w, h, reduced.data());
    }
    surface.endDrawing();
}
