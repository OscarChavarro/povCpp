#ifndef VITRAL_FRAMEBUFFER_MENU_READER_H
#define VITRAL_FRAMEBUFFER_MENU_READER_H

#include "java/lang/String.h"

class Widget;

// Reads the menubar from a Vitral GUI JSON document into an existing Widget.
class FramebufferMenuReader {
public:
    static bool read(const java::String &json, Widget &widget);
};

#endif
