#ifndef VITRAL_FRAMEBUFFER_IMAGE_VIEW_H
#define VITRAL_FRAMEBUFFER_IMAGE_VIEW_H

#include <stdint.h>
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferControl.h"

// Shows an image of 0xRRGGBB pixels (rows from the top), centered and reduced to fit when it is larger than the
// control.
class FramebufferImageView : public FramebufferControl {
public:
    FramebufferImageView(int width, int height, const uint32_t *rgb);

    int preferredWidth(const Framebuffer &/*surface*/) const override { return imageWidth + 4; }
    int preferredHeight(const Framebuffer &/*surface*/) const override { return imageHeight + 4; }
    bool isStretchable() const override { return true; }
    void draw(Framebuffer &surface, const FramebufferTheme &theme, bool focused) override;

private:
    int imageWidth;
    int imageHeight;
    java::ArrayList<uint32_t> pixels;
};

#endif
