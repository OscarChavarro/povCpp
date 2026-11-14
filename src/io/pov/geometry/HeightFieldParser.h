#ifndef __HEIGHT_FIELD_PARSER__
#define __HEIGHT_FIELD_PARSER__

#include "io/pov/context/ParserContext.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"

class HeightFieldParser {
  public:
    static SimpleBodyBuilder *parseHeightField();
    static SimpleBodyBuilder *parseHeightField(ParserContext &ctx);

  private:
    static constexpr int GIF = 0;
    static constexpr int POT = 1;
    static constexpr int TGA = 2;
    // Height of points on the image border or at or below the water level.
    static constexpr double HIDDEN_HEIGHT = -10000.0;

    // Raises the floor of the grid box to the water level.
    static void setWaterLevel(HeightFieldSolid *solid, double level);
    // PovRay's image rules: border and water level points at HIDDEN_HEIGHT,
    // POT and TGA fractional heights.
    static double imageHeight(const HeightFieldSolid *solid, double height);
    static void findHfMinMax(HeightFieldSolid *solid,
        const IndexedColorImageHDRUncompressed *image, int imageType);
    static void findHfMinMax(HeightFieldSolid *solid,
        const RGBAImageHDRUncompressed *image, int imageType);
};

#endif
