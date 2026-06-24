#ifndef __RASTER_TILE_GENERATOR__
#define __RASTER_TILE_GENERATOR__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/render/raytracing/RasterTileGenerationStrategy.h"

class Image;
class RasterTileArea;

class RasterTileGenerator {
  private:
    RasterTileGenerationStrategy strategy;
    Image* image;
    int x0;
    int y0;
    int width;
    int height;
    int numberOfThreads;
    java::ArrayList<RasterTileArea> tiles;

    java::ArrayList<RasterTileArea> generateTiles() const;
    java::ArrayList<RasterTileArea> generateLinearTiles() const;
    java::ArrayList<RasterTileArea> generateSerialTile() const;

  public:
    RasterTileGenerator(
        RasterTileGenerationStrategy strategy,
        Image* image,
        int width,
        int height,
        int numberOfThreads);

    RasterTileGenerator(
        RasterTileGenerationStrategy strategy,
        Image* image,
        int x0,
        int y0,
        int width,
        int height,
        int numberOfThreads);

    const java::ArrayList<RasterTileArea>& getTiles() const;
};

#endif
