#ifndef __FONT_READER__
#define __FONT_READER__

#include "java/lang/String.h"
#include "vsdk/toolkit/io/PersistenceElement.h"

class ParametricCurve;

/**
This is an abstract class to serve as a base in an abstract factory design
pattern that serves to load font file data from TrueType, Type1 and
other common font encoding schema.

The basic functionality that this must provide, is to extract a glyph
from a specified character from a font file.

C++ counterpart of Java's `vsdk.toolkit.io.geometry.FontReader`. Java's
concrete factory is `vsdk.toolkit.render.awt.AwtFontReader`; the C++ one is
`Freetype2FontReader`.
*/
class FontReader : public PersistenceElement {
public:
    virtual ~FontReader() {}

    /**
    @param fontFile path of the font file
    @param characterAndItsContext the character (UTF-8), possibly with context
    @return a new curve with the glyph outline, owned by the caller, or null
    if something goes wrong
    */
    virtual ParametricCurve* extractGlyph(const java::String& fontFile,
        const java::String& characterAndItsContext) = 0;
};

#endif
