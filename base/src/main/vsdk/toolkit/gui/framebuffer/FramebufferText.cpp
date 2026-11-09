#include "vsdk/toolkit/gui/framebuffer/FramebufferText.h"

static bool
isContinuation(char c) {
    return ((unsigned char)c & 0xC0) == 0x80;
}

int
FramebufferText::next(const java::String &text, int index) {
    if (index >= text.length()) {
        return text.length();
    }
    index++;
    while (index < text.length() && isContinuation(text.charAt(index))) {
        index++;
    }
    return index;
}

int
FramebufferText::previous(const java::String &text, int index) {
    if (index <= 0) {
        return 0;
    }
    index--;
    while (index > 0 && isContinuation(text.charAt(index))) {
        index--;
    }
    return index;
}

int
FramebufferText::characters(const java::String &text, int byteCount) {
    int count = 0;
    for (int i = 0; i < byteCount && i < text.length(); i++) {
        count += isContinuation(text.charAt(i)) ? 0 : 1;
    }
    return count;
}

int
FramebufferText::offsetOf(const java::String &text, int count) {
    int index = 0;
    for (int i = 0; i < count && index < text.length(); i++) {
        index = next(text, index);
    }
    return index;
}

java::String
FramebufferText::fromKey(char unicodeId) {
    unsigned char c = (unsigned char)unicodeId;
    char bytes[3] = {0, 0, 0};
    if (c < 0x20 || c == 0x7F || (c >= 0x80 && c < 0xA0)) {
        return java::String("");
    }
    if (c < 0x80) {
        bytes[0] = (char)c;
    } else {
        bytes[0] = (char)(0xC0 | c >> 6);
        bytes[1] = (char)(0x80 | (c & 0x3F));
    }
    return java::String(bytes);
}
