#ifndef __CALLIGRAPHIC_2_D_BUFFER__
#define __CALLIGRAPHIC_2_D_BUFFER__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/media/MediaEntity.h"
class Vector3Dd;
class RGBImageUncompressed;

/**
The Calligraphic2DBuffer class represents a set of elements suitable for a
vector graphics device, like calligraphic CRT, vectorized postscript
and conventional/legacy pen-plotters.

This class is to calligraphic devices like the Image class is to raster
devices.

The nature of this class is structurally 2D, so must not be treated as a
Geometry as doesn't live in 3D space. Nevertheless, could be used as an
argument or modifier for 3D Geometry, in the same way an Image could be
used as a map (i.e. texture or colormap, depthmap, bumpmap, etc).

This class doesn't impose any interpretation on coordinates, but it is
suggested that internal double coordinates be mapped to the range
<-1, -1, -1> to <1, 1, 1>.
*/
class Calligraphic2DBuffer : public MediaEntity {

private:
    java::ArrayList<double> lineData;

public:
    Calligraphic2DBuffer();
    virtual ~Calligraphic2DBuffer() = default;

    /**
    Erases all internal calligraphy contents
    */
    void init();

    /**
    Adds a 2D line to the calligraphic buffer. Note that z components in
    Vector coordinates are discarded.
    */
    void add2DLine(const Vector3Dd& p0, const Vector3Dd& p1);

    /**
    Adds a 2D line to the calligraphic buffer.
    */
    void add2DLine(double x0, double y0, double x1, double y1);

    Vector3Dd* get2DLinePoint0(int i) const;
    Vector3Dd* get2DLinePoint1(int i) const;

    int getNumLines() const;

    void exportRgbImage(RGBImageUncompressed* inOutRasterViewport);
};

#endif
