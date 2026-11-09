#include "java/lang/String.h"
#include <cstdio>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/curve/ParametricCurve.h"
#include "vsdk/toolkit/io/geometry/Freetype2FontReader.h"

namespace {

/** `java.awt.geom.PathIterator` segment types. */
const int SEG_MOVETO = 0;
const int SEG_LINETO = 1;
const int SEG_QUADTO = 2;
const int SEG_CUBICTO = 3;
const int SEG_CLOSE = 4;

/** One outline segment, as `PathIterator.currentSegment` reports it. */
struct OutlineSegment {
    int type;
    double coords[6];
};

struct DecomposeState {
    java::ArrayList<OutlineSegment>* segments;
    bool open;
};

/** AWT's `F26Dot6ToFloat`, with its Y negation, into a path coordinate. */
void setCoordinates(OutlineSegment& segment, int index, const FT_Vector* v)
{
    segment.coords[index] = (double)((float)v->x / 64.0f);
    segment.coords[index + 1] = (double)(-((float)v->y / 64.0f));
}

void closeIfOpen(DecomposeState* state)
{
    if ( state->open ) {
        OutlineSegment close = { SEG_CLOSE, { 0, 0, 0, 0, 0, 0 } };
        state->segments->add(close);
        state->open = false;
    }
}

int moveTo(const FT_Vector* to, void* user)
{
    DecomposeState* state = (DecomposeState*)user;
    closeIfOpen(state);
    OutlineSegment segment = { SEG_MOVETO, { 0, 0, 0, 0, 0, 0 } };
    setCoordinates(segment, 0, to);
    state->segments->add(segment);
    state->open = true;
    return 0;
}

int lineTo(const FT_Vector* to, void* user)
{
    DecomposeState* state = (DecomposeState*)user;
    OutlineSegment segment = { SEG_LINETO, { 0, 0, 0, 0, 0, 0 } };
    setCoordinates(segment, 0, to);
    state->segments->add(segment);
    return 0;
}

int conicTo(const FT_Vector* control, const FT_Vector* to, void* user)
{
    DecomposeState* state = (DecomposeState*)user;
    OutlineSegment segment = { SEG_QUADTO, { 0, 0, 0, 0, 0, 0 } };
    setCoordinates(segment, 0, control);
    setCoordinates(segment, 2, to);
    state->segments->add(segment);
    return 0;
}

int cubicTo(const FT_Vector* control1, const FT_Vector* control2,
    const FT_Vector* to, void* user)
{
    DecomposeState* state = (DecomposeState*)user;
    OutlineSegment segment = { SEG_CUBICTO, { 0, 0, 0, 0, 0, 0 } };
    setCoordinates(segment, 0, control1);
    setCoordinates(segment, 2, control2);
    setCoordinates(segment, 4, to);
    state->segments->add(segment);
    return 0;
}

/**
Decodes the first code point of an UTF-8 string, as Java takes the first
character of `characterAndItsContext` for the glyph vector.
*/
unsigned long firstCodePoint(const java::String& text)
{
    const unsigned char* s = (const unsigned char*)text.c_str();
    if ( s == 0 || s[0] == 0 ) {
        return 0;
    }
    if ( s[0] < 0x80 ) {
        return s[0];
    }
    if ( (s[0] & 0xE0) == 0xC0 && s[1] != 0 ) {
        return ((unsigned long)(s[0] & 0x1F) << 6) | (s[1] & 0x3F);
    }
    if ( (s[0] & 0xF0) == 0xE0 && s[1] != 0 && s[2] != 0 ) {
        return ((unsigned long)(s[0] & 0x0F) << 12) |
            ((unsigned long)(s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    }
    if ( (s[0] & 0xF8) == 0xF0 && s[1] != 0 && s[2] != 0 && s[3] != 0 ) {
        return ((unsigned long)(s[0] & 0x07) << 18) |
            ((unsigned long)(s[1] & 0x3F) << 12) |
            ((unsigned long)(s[2] & 0x3F) << 6) | (s[3] & 0x3F);
    }
    return s[0];
}

bool shouldAddEndpoint(const Vector3Dd* lastAddedEndpoint,
    const Vector3Dd& candidateEndpoint)
{
    return lastAddedEndpoint == 0 ||
        lastAddedEndpoint->subtract(candidateEndpoint).length() >= VSDK::EPSILON;
}

void attachBezierControlToPreviousPoint(ParametricCurve* curve,
    const Vector3Dd& controlPoint)
{
    if ( curve->getPointSize() == 0 ) {
        return;
    }

    int lastIndex = curve->getPointSize() - 1;
    java::ArrayList<Vector3Dd> previous = curve->getPointVector(lastIndex);
    if ( previous.size() == 0 ) {
        return;
    }

    if ( previous.size() >= 3 ) {
        previous.set(2, controlPoint);
        curve->setPointAt(previous, lastIndex);
        return;
    }

    // C++ port note: Java leaves the missing tangent slot null; a zero
    // vector stands for it here
    java::ArrayList<Vector3Dd> expanded;
    expanded.add(previous.get(0));
    expanded.add(previous.size() > 1 ? previous.get(1) : Vector3Dd());
    expanded.add(controlPoint);
    curve->setPointAt(expanded, lastIndex);
}

}

Freetype2FontReader::Freetype2FontReader()
    : library(0), currentFace(0)
{
    FT_Library ftLibrary;
    if ( FT_Init_FreeType(&ftLibrary) == 0 ) {
        library = ftLibrary;
    }
}

Freetype2FontReader::~Freetype2FontReader()
{
    releaseFace();
    if ( library != 0 ) {
        FT_Done_FreeType((FT_Library)library);
    }
}

void Freetype2FontReader::releaseFace()
{
    if ( currentFace != 0 ) {
        FT_Done_Face((FT_Face)currentFace);
        currentFace = 0;
    }
}

ParametricCurve* Freetype2FontReader::extractGlyph(const java::String& fontFile,
    const java::String& characterAndItsContext)
{
    //-----------------------------------------------------------------
    float factor = 10.0f;
    if ( currentFace == 0 || fileName.length() == 0 || !(fontFile == fileName) ) {
        fileName = fontFile;
        releaseFace();
        FT_Face face;
        if ( library == 0 ||
             FT_New_Face((FT_Library)library, fontFile.c_str(), 0, &face) != 0 ) {
            fprintf(stderr, "Error loading font file %s\n", fontFile.c_str());
            return 0;
        }
        // AWT's `deriveFont(10)`: ten points at 72 dpi, ten pixels per em
        if ( FT_Set_Char_Size(face, 0, (FT_F26Dot6)(factor * 64), 72, 72) != 0 ) {
            FT_Done_Face(face);
            fprintf(stderr, "Error loading font file %s\n", fontFile.c_str());
            return 0;
        }
        currentFace = face;
    }

    FT_Face face = (FT_Face)currentFace;

    //- Glyph analisys -------------------------------------------
    // AWT's `FontRenderContext(identity, antialiased, fractional metrics)`
    // loads the outline without hinting
    FT_UInt glyphIndex = FT_Get_Char_Index(face, firstCodePoint(characterAndItsContext));
    if ( FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) != 0 ||
         face->glyph->format != FT_GLYPH_FORMAT_OUTLINE ) {
        fprintf(stderr, "Glyph outline is null for [%s] in font %s\n",
            characterAndItsContext.c_str(), fontFile.c_str());
        return 0;
    }

    java::ArrayList<OutlineSegment> p;
    DecomposeState state;
    state.segments = &p;
    state.open = false;
    FT_Outline_Funcs funcs;
    funcs.move_to = moveTo;
    funcs.line_to = lineTo;
    funcs.conic_to = conicTo;
    funcs.cubic_to = cubicTo;
    funcs.shift = 0;
    funcs.delta = 0;
    if ( FT_Outline_Decompose(&face->glyph->outline, &funcs, &state) != 0 ) {
        fprintf(stderr, "Glyph outline is null for [%s] in font %s\n",
            characterAndItsContext.c_str(), fontFile.c_str());
        return 0;
    }
    closeIfOpen(&state);

    ParametricCurve* curve = new ParametricCurve();
    bool endIt = false;
    int code;
    bool hasLastAddedEndpoint = false;
    Vector3Dd lastAddedEndpoint;

    for ( size_t s = 0; s < p.size(); s++ ) {
        const double* coords = p[s].coords;
        int type = p[s].type;
        java::ArrayList<Vector3Dd> pointParameters;

        code = 0;
        switch ( type ) {
          case SEG_CUBICTO:
            code = 4;
            break;
          case SEG_LINETO:
            code = 1;
            break;
          case SEG_MOVETO:
            code = 0;
            break;
          case SEG_QUADTO:
            code = 2;
            break;
          case SEG_CLOSE:
            code = 3;
            break;
          default:
            printf("AwtFontReader.extractGlyph: UNKNOWN");
            break;
        }

        if ( !endIt ) {
            switch ( code ) {
              case 0:
                curve->addPoint(java::ArrayList<Vector3Dd>(), ParametricCurve::BREAK);
                hasLastAddedEndpoint = false;

                pointParameters.add(Vector3Dd(coords[0]/factor, -coords[1]/factor, 0));
                if ( shouldAddEndpoint(hasLastAddedEndpoint ? &lastAddedEndpoint : 0,
                         pointParameters.get(0)) ) {
                    curve->addPoint(pointParameters, ParametricCurve::CORNER);
                    lastAddedEndpoint = pointParameters.get(0);
                    hasLastAddedEndpoint = true;
                }
                break;
              case 1:
                pointParameters.add(Vector3Dd(coords[0]/factor, -coords[1]/factor, 0));
                if ( shouldAddEndpoint(hasLastAddedEndpoint ? &lastAddedEndpoint : 0,
                         pointParameters.get(0)) ) {
                    curve->addPoint(pointParameters, ParametricCurve::CORNER);
                    lastAddedEndpoint = pointParameters.get(0);
                    hasLastAddedEndpoint = true;
                }
                break;
              case 2:
                // Note the inverse order of awt with respect to VSDK!
                pointParameters.add(Vector3Dd(coords[2]/factor, -coords[3]/factor, 0));
                pointParameters.add(Vector3Dd(coords[0]/factor, -coords[1]/factor, 0));
                if ( shouldAddEndpoint(hasLastAddedEndpoint ? &lastAddedEndpoint : 0,
                         pointParameters.get(0)) ) {
                    curve->addPoint(pointParameters, ParametricCurve::QUAD);
                    lastAddedEndpoint = pointParameters.get(0);
                    hasLastAddedEndpoint = true;
                }
                break;
              case 3:
                //endIt = true;
                break;
              case 4:
                pointParameters.add(Vector3Dd(coords[4]/factor, -coords[5]/factor, 0));
                pointParameters.add(Vector3Dd(coords[2]/factor, -coords[3]/factor, 0));
                if ( shouldAddEndpoint(hasLastAddedEndpoint ? &lastAddedEndpoint : 0,
                         pointParameters.get(0)) ) {
                    attachBezierControlToPreviousPoint(curve,
                        Vector3Dd(coords[0]/factor, -coords[1]/factor, 0));
                    curve->addPoint(pointParameters, ParametricCurve::BEZIER);
                    lastAddedEndpoint = pointParameters.get(0);
                    hasLastAddedEndpoint = true;
                }
                break;
              default:
                break;
            }
        }
    }

    if ( curve->getPointSize() < 2 ) {
        fprintf(stderr, "Glyph [%s] in font %s produced too few curve segments\n",
            characterAndItsContext.c_str(), fontFile.c_str());
        delete curve;
        return 0;
    }

    return curve;
}
