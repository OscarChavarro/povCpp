#ifndef __SOLID_TEXTURE_PIGMENT__
#define __SOLID_TEXTURE_PIGMENT__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageTexture.h"
#include "vsdk/toolkit/media/solidTexture/procedural/ColorTextureFixture.h"

// One concrete subclass per POV-Ray pigment pattern (wood, marble, checker, ...). `point`
// is already expressed in the owning material's texture space; callers (RayShaderPipeline,
// CheckerTexturePigment) are responsible for applying that material's own
// textureTransformationInverse via transformToObjectSpace() before calling colorAt.
class SolidTexturePigment {
  public:
    virtual ~SolidTexturePigment() {}

    virtual void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const = 0;
    virtual SolidTexturePigment *copy() const = 0;

    // Only CheckerTexturePigment overrides these, to recurse into its two sub-materials'
    // own texture-space transforms. Every other pigment is a leaf with nothing to transform.
    virtual void rotate(Vector3Dd *vector) {}
    virtual void scale(Vector3Dd *vector) {}
    virtual void translate(Vector3Dd *vector) {}

    // A flat colour pigment is the one pattern whose appearance does not depend on
    // texture-space position, so it is excluded from the transform gate (matches the
    // original colorPatternType != COLOUR_TEXTURE exclusion). Only ColourPigment overrides.
    virtual bool needsTransform() const { return true; }

    static RGBAColorPalette *cloneColorMap(const RGBAColorPalette *source);
    static Vector3Dd transformToObjectSpace(
        const Vector3Dd *point, const Matrix4x4d *transformationInverse);
};

#endif
