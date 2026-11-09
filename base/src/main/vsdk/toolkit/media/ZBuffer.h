#ifndef __Z_BUFFER__
#define __Z_BUFFER__

#include "vsdk/toolkit/media/MediaEntity.h"

class IndexedColorImageUncompressed;
class RGBAImageUncompressed;
class RGBColorPalette;
class RGBImageUncompressed;
/**
This class represents a depth map. A depth map can be used to:
  - Represent how far a pixel in the color buffer is from the camera. This
    is useful as part of visualization algorithms like ZBuffer.
  - Represent Light maps. Light maps are useful in two pass algorithms for
    shadow approximation.
*/
class ZBuffer : public MediaEntity {

private:
    float* depth;
    int xSize;
    int ySize;

public:
    /**
    Constructs a Z buffer with the specified parameters; the depth data is
    initialized to value 0.0
    @param width The width of the z buffer
    @param height The height of the z buffer
    */
    ZBuffer(int width, int height);

    /**
    Creates a z buffer given the z buffer's raw data. The stored z buffer will have
    an inverse row order than that of the parameter's data
    @param dep The z buffer data to store in this z buffer
    @param width The width of the z buffer and the parameter input data
    @param height The height of the z buffer and the parameter input data
    */
    ZBuffer(float* dep, int width, int height);

    virtual ~ZBuffer();

    /**
    Returns the width of this z buffer
    @return The width of this z buffer
    */
    int getXSize() const;

    /**
    Returns the height of this z buffer
    @return The height of this z buffer
    */
    int getYSize() const;

    /**
    This method returns the Z buffer data stored in this depth map
    @return The Z buffer data stored in this depth map
    */
    float* getZBuffer() const;

    /**
    This method returns the depth value at the specified position
    @param x x coordinate
    @param y y coordinate
    @return the depth value at position (x, y)
    */
    float getDepth(int x, int y) const;

    /**
    This method sets the depth value at the specified position
    @param x x coordinate
    @param y y coordinate
    @param value the depth value to set
    */
    void setDepth(int x, int y, float value);

    /**
    Returns a copy of this z buffer
    @return a new copy of this z buffer
    */
    ZBuffer* clone() const;

    /**
    This method converts this Z buffer into a gray level image, with depths
    clamped to [0, 1].
    @return a new image, owned by the caller
    */
    IndexedColorImageUncompressed* exportIndexedColorImage() const;

    /**
    This method converts this Z buffer into an RGBImageUncompressed using the
    specified ColorPalette
    @param p The color palete used to convert this z buffer into an
    RGBImageUncompressed
    @return a new image that represents this z buffer, owned by the caller
    */
    RGBImageUncompressed* exportRGBImage(const RGBColorPalette* p) const;

    /**
    This method converts this Z buffer into an RGBAImageUncompressed using
    the specified ColorPalette
    @param p The color palete used to convert this Z buffer into an
    RGBAImageUncompressed
    @return a new image that represents this Z buffer, owned by the caller
    */
    RGBAImageUncompressed* exportRGBAImage(const RGBColorPalette* p) const;
};

#endif
