#ifndef VITRAL_FRAMEBUFFER_LABEL_H
#define VITRAL_FRAMEBUFFER_LABEL_H

#include "java/lang/String.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferControl.h"

// Static text, one line per '\n'. Lines wider than the label are cut at the right or, with setElideLeft(), shown
// with "..." in place of their beginning (to keep the end of a path in sight).
class FramebufferLabel : public FramebufferControl {
public:
    explicit FramebufferLabel(const java::String &text);
    void setText(const java::String &text) { label = text; }
    const java::String &getText() const { return label; }
    void setElideLeft(bool elide) { elideLeft = elide; }

    int preferredWidth(const Framebuffer &surface) const override;
    int preferredHeight(const Framebuffer &surface) const override;
    void draw(Framebuffer &surface, const FramebufferTheme &theme, bool focused) override;

private:
    java::String label;
    bool elideLeft;

    int lines() const;
};

#endif
