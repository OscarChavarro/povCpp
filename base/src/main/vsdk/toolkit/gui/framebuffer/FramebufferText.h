#ifndef VITRAL_FRAMEBUFFER_TEXT_H
#define VITRAL_FRAMEBUFFER_TEXT_H

#include "java/lang/String.h"

// Character positions in the UTF-8 texts of the framebuffer controls. Indexes are byte offsets that always fall on
// the first byte of a character.
class FramebufferText {
public:
    // The offset of the character after (or before) the one at index, clamped to the text.
    static int next(const java::String &text, int index);
    static int previous(const java::String &text, int index);
    // The number of characters in the first byteCount bytes.
    static int characters(const java::String &text, int byteCount);
    // The offset of the character number count, or the length of the text when it is shorter.
    static int offsetOf(const java::String &text, int count);
    // The UTF-8 encoding of a key's character: keyboards deliver Latin-1 bytes. Empty for control characters.
    static java::String fromKey(char unicodeId);
};

#endif
