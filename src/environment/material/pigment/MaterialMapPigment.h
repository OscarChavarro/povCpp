#ifndef __MATERIAL_MAP_PIGMENT__
#define __MATERIAL_MAP_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

// Marker pigment for material_map textures. RayShaderPipeline intercepts these before any
// colorAt call and swaps in one of the material's own materialMapVariants; colorAt() is only
// reached when the image lookup misses (index == -1), in which case the original
// SolidTextureFixturesFacade::colorAt switch had no matching case and left color untouched,
// which this no-op replicates. needsTransform() stays true (inherited default) so that
// rotate/scale/translate keep positioning the material_map image lookup correctly.
class MaterialMapPigment : public SolidTexturePigment {
  public:
    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
};

#endif
