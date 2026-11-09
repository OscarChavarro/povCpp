#ifndef __NORMAL_MAP__
#define __NORMAL_MAP__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/media/MediaEntity.h"
class Vector3Dd;
class IndexedColorImageUncompressed;
class RGBImageUncompressed;
class RGBAImageUncompressed;

/**
This class represents a normal map, containing normal vectors for each
pixel position.
*/
class NormalMap : public MediaEntity {

private:
    int xSize;
    int ySize;
    java::ArrayList<Vector3Dd*> data;

    Vector3Dd* bumpMapScale;

public:
    NormalMap();
    NormalMap(const NormalMap& other);
    virtual ~NormalMap();

    bool init(int width, int height);

    int getXSize() const;

    int getYSize() const;

    Vector3Dd* getBumpMapScale() const;

    void setBumpMapScale(const Vector3Dd& scale);

    void putNormal(int i, int j, const Vector3Dd& n);

    Vector3Dd* getNormal(int u, int v) const;

    /**
    Provide a bilinear interpolation scheme for normal vectors.
    @param u u coordinate in [0, 1]
    @param v v coordinate in [0, 1]
    @return interpolated normal
    */
    Vector3Dd* getNormalBiLinear(double u, double v) const;

    RGBImageUncompressed* exportToRgbImage() const;

    /**
    Similar to exportToRgbImage, but each pixel is equivalent to a magnitude
    of displacement from <0, 0, 1> normal
    @return a new gray level image owned by the caller, or null if it could
    not be allocated
    */
    RGBImageUncompressed* exportToRgbImageGradient() const;

    /**
    Similar to exportToRgbImageGradient, but as a translucent (alpha 128)
    RGBA image
    @return a new image owned by the caller, or null if it could not be
    allocated
    */
    RGBAImageUncompressed* exportToRgbaImageGradient() const;
    Vector3Dd importBumpMap(IndexedColorImageUncompressed* inBumpmap, const Vector3Dd& inScale);
    NormalMap* clone() const;
};

#endif
