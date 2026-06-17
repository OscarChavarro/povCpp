#ifndef __VSDK_TOOLKIT_MEDIA_RGBACOLORPALETTE_H__
#define __VSDK_TOOLKIT_MEDIA_RGBACOLORPALETTE_H__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/media/MediaEntity.h"
class ColorRgba;

/**
This class represents an RGBA color palette, as an indexed set of colors.
evalLinear with no explicit positions interpolates uniformly across stops.
addColorAt / evalLinear with positions stored supports non-uniform stops.
*/
class RGBAColorPalette : public MediaEntity {

  protected:
    java::ArrayList<ColorRgba*> colors;
    java::ArrayList<double>     positions; // empty = uniform spacing

  public:
    RGBAColorPalette();
    virtual ~RGBAColorPalette();

    void init(int size);

    int size() const;
    bool hasPositions() const;

    ColorRgba* getColorAt(int i) const;
    double     getPositionAt(int i) const;

    virtual void setColorAt(int i, const ColorRgba& c);
    virtual void setColorAt(int i, double r, double g, double b, double a);

    virtual void addColor(const ColorRgba& c);
    virtual void addColor(double r, double g, double b, double a);
    virtual void addColorAt(double position, const ColorRgba& c);

    ColorRgba* evalNearest(double t) const;
    ColorRgba* evalLinear(double t) const;
};

#endif // __VSDK_TOOLKIT_MEDIA_RGBACOLORPALETTE_H__
