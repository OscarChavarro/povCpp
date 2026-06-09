#ifndef __VSDK_TOOLKIT_MEDIA_RGBCOLORPALETTE_H__
#define __VSDK_TOOLKIT_MEDIA_RGBCOLORPALETTE_H__

#include "vsdk/toolkit/media/MediaEntity.h"
#include "java/util/ArrayList.h"

class ColorRgb;

/**
This class represents a color palette, as an indexed set of colors.
*/
class RGBColorPalette : public MediaEntity {

protected:
    java::ArrayList<ColorRgb*> colors;

public:
    RGBColorPalette();
    virtual ~RGBColorPalette();

    void init(int size);

    int size() const;

    void buildGrayLevelsTable();

    ColorRgb* getColorAt(int i) const;

    virtual void setColorAt(int i, ColorRgb* c);
    virtual void setColorAt(int i, double r, double g, double b);

    virtual void addColor(ColorRgb* c);
    virtual void addColor(double r, double g, double b);

    ColorRgb* evalNearest(double t) const;

    ColorRgb* evalLinear(double t) const;

    int selectNearestIndexToRgb(const ColorRgb& c) const;
};

#endif // __VSDK_TOOLKIT_MEDIA_RGBCOLORPALETTE_H__
