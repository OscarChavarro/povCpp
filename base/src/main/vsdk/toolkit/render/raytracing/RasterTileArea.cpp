#include "java/lang/IllegalArgumentException.h"
#include "vsdk/toolkit/media/Image.h"
#include "vsdk/toolkit/render/raytracing/RasterTileArea.h"
RasterTileArea::RasterTileArea() : image(nullptr), startX(0), startY(0), width(0), height(0) {}

RasterTileArea::RasterTileArea(Image* imageIn, int startXIn, int startYIn, int widthIn, int heightIn)
    : image(imageIn), startX(startXIn), startY(startYIn), width(widthIn), height(heightIn)
{
    if ( image == 0 ) throw java::IllegalArgumentException("image can not be null");
    if ( startX < 0 || startY < 0 ) throw java::IllegalArgumentException("tile origin must be >= 0");
    if ( width <= 0 || height <= 0 ) throw java::IllegalArgumentException("tile size must be > 0");
    if ( startX + width > image->getXSize() || startY + height > image->getYSize() ) {
        throw java::IllegalArgumentException("tile bounds must be inside target image");
    }
}

Image* RasterTileArea::getImage() const { return image; }
int RasterTileArea::getStartX() const { return startX; }
int RasterTileArea::getStartY() const { return startY; }
int RasterTileArea::getWidth() const { return width; }
int RasterTileArea::getHeight() const { return height; }
int RasterTileArea::getEndX() const { return startX + width; }
int RasterTileArea::getEndY() const { return startY + height; }
