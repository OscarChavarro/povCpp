#ifndef __RASTER_TILE_AREA__
#define __RASTER_TILE_AREA__

class Image;

class RasterTileArea {
private:
    Image* image;
    int startX;
    int startY;
    int width;
    int height;

public:
    RasterTileArea();
    RasterTileArea(Image* image, int startX, int startY, int width, int height);

    Image* getImage() const;
    int getStartX() const;
    int getStartY() const;
    int getWidth() const;
    int getHeight() const;
    int getEndX() const;
    int getEndY() const;
};

#endif
