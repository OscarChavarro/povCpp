#ifndef __FREETYPE2_FONT_READER__
#define __FREETYPE2_FONT_READER__

#include "java/lang/String.h"
#include "vsdk/toolkit/io/geometry/FontReader.h"

/**
Concrete `FontReader` over the FreeType 2 library: the C++ counterpart of
Java's `vsdk.toolkit.render.awt.AwtFontReader`.

Java does not read the font file itself: it hands the file to AWT
(`Font.createFont`, `deriveFont(10)`), asks a `GlyphVector` for the outline of
the glyph and walks it with a `PathIterator`. AWT gets that outline from
FreeType: an unhinted load of the glyph at ten pixels per em, decomposed with
`FT_Outline_Decompose`, each 26.6 coordinate turned into a `float` divided by
64 with Y negated. This class asks FreeType for the same outline and, from
there on, follows `AwtFontReader.extractGlyph` statement by statement: the
`BREAK` before each contour, `CORNER` for a line, `QUAD` with the endpoint
before the control point, the `BEZIER` control attachment, the
duplicate-endpoint filter, the ten-point size divided back out and the Y flip.

Built only when the base library is configured `WITH_FREETYPE`.
*/
class Freetype2FontReader : public FontReader {
public:
    Freetype2FontReader();
    virtual ~Freetype2FontReader();

    /**
    Given a font file and a character, this method return a parametric
    curve representing a glyph. If something goes wrong, this method
    returns null. The character string is usually a lone character,
    but under some circumstantes this chatacter is acommpanied with
    a context (i.e. in arabic languages where glyph selection depends
    on a bounding form).

    As AWT does for Java, the glyph is loaded `factor` points in size, and
    later scaled down by a factor of 1/`factor`.
    @param fontFile path of the font file
    @param characterAndItsContext the character (UTF-8), possibly with context
    @return a new curve owned by the caller, or null
    */
    virtual ParametricCurve* extractGlyph(const java::String& fontFile,
        const java::String& characterAndItsContext) override;

private:
    /** FreeType handles, kept opaque to spare clients the FreeType headers. */
    void* library;
    void* currentFace;
    java::String fileName;

    void releaseFace();
};

#endif
