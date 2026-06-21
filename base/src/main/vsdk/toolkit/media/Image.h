#ifndef __VSDK_TOOLKIT_MEDIA_IMAGE_H__
#define __VSDK_TOOLKIT_MEDIA_IMAGE_H__

#include "vsdk/toolkit/media/MediaEntity.h"

/**
Abstract raster image interface. It exposes only the pixel-grid dimensions that
spatial subdivision needs (tiling, clipping, bounds checking) without committing
to a concrete pixel storage. Concrete image classes (for example
`RGBAImageHDRUncompressed`) implement it so that render-target consumers such as
`RasterTileArea` can refer to any image through this base type.
*/
class Image : public MediaEntity {

  public:
    virtual ~Image() = default;

    virtual int getXSize() const = 0;
    virtual int getYSize() const = 0;
};

#endif // __VSDK_TOOLKIT_MEDIA_IMAGE_H__
