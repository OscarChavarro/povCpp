#include <cstring>
#include <gtest/gtest.h>

#ifdef VITRAL_WITH_FREETYPE

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/curve/ParametricCurve.h"
#include "vsdk/toolkit/io/geometry/Freetype2FontReader.h"

/*
Regression coverage for `Freetype2FontReader`, the C++ counterpart of Java's
`AwtFontReader`. The expected values are the exact bits Java produces with
AWT for the same font and character (cross-port glyph oracle of the boolean
pipeline port plan).
*/
namespace {

const char* const NOTO_SANS_LIGHT = VITRAL_REPOSITORY_ROOT "/etc/fonts/notoSansLight.ttf";

unsigned long long bitsOf(double value)
{
    unsigned long long bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

}

TEST(Freetype2FontReaderTest, GlyphAMatchesJavaAwtOutlineBits) {
    Freetype2FontReader reader;
    ParametricCurve* curve = reader.extractGlyph(NOTO_SANS_LIGHT, "A");
    ASSERT_NE((ParametricCurve*)0, curve);

    // Outer contour (corners), then the BREAK, then the inner contour, whose
    // first segments are quadratic (endpoint before control point)
    const unsigned long long outer[9][2] = {
        { 0x3fe1800000000000ULL, 0x0000000000000000ULL },
        { 0x3fdce66666666666ULL, 0x3fcf666666666666ULL },
        { 0x3fc4000000000000ULL, 0x3fcf666666666666ULL },
        { 0x3fae666666666666ULL, 0x0000000000000000ULL },
        { 0x0000000000000000ULL, 0x0000000000000000ULL },
        { 0x3fd2000000000000ULL, 0x3fe6f33333333333ULL },
        { 0x3fd5666666666666ULL, 0x3fe6f33333333333ULL },
        { 0x3fe3733333333333ULL, 0x0000000000000000ULL },
        { 0x3fe1800000000000ULL, 0x0000000000000000ULL }
    };
    ASSERT_EQ(20, curve->getPointSize());
    for ( int i = 0; i < 9; i++ ) {
        EXPECT_EQ(ParametricCurve::CORNER, curve->getPointType(i));
        const java::ArrayList<Vector3Dd>& p = curve->getPointVector(i);
        ASSERT_EQ(1, p.size());
        EXPECT_EQ(outer[i][0], bitsOf(p.get(0).x())) << "point " << i;
        EXPECT_EQ(outer[i][1], bitsOf(p.get(0).y())) << "point " << i;
    }
    EXPECT_EQ(ParametricCurve::BREAK, curve->getPointType(9));
    EXPECT_EQ(ParametricCurve::QUAD, curve->getPointType(11));
    const java::ArrayList<Vector3Dd>& quad = curve->getPointVector(11);
    ASSERT_EQ(2, quad.size());
    EXPECT_EQ(0x3fd5000000000000ULL, bitsOf(quad.get(0).x()));
    EXPECT_EQ(0x3fe2a66666666666ULL, bitsOf(quad.get(0).y()));
    EXPECT_EQ(0x3fd5666666666666ULL, bitsOf(quad.get(1).x()));
    EXPECT_EQ(0x3fe219999999999aULL, bitsOf(quad.get(1).y()));
    delete curve;
}

TEST(Freetype2FontReaderTest, MissingFontFileGivesNull) {
    Freetype2FontReader reader;
    EXPECT_EQ((ParametricCurve*)0,
        reader.extractGlyph("/nonexistent/font.ttf", "A"));
}

#endif
