#ifndef __VSDK_TOOLKIT_MEDIA_RGBACOLORPALETTE_H__
#define __VSDK_TOOLKIT_MEDIA_RGBACOLORPALETTE_H__

#include "vsdk/toolkit/media/MediaEntity.h"
#include "vsdk/toolkit/media/RGBAColorPaletteSpan.h"
#include "java/util/ArrayList.h"

class ColorRgba;

class RGBAColorPalette : public MediaEntity {

  protected:
    java::ArrayList<RGBAColorPaletteSpan*> spans;
    bool _transparencyFlag;

  public:
    RGBAColorPalette();
    virtual ~RGBAColorPalette();

    void addSpan(double start, double end,
                 const ColorRgba& startColor, const ColorRgba& endColor);

    int size() const;
    bool transparencyFlag() const;
    const RGBAColorPaletteSpan* getSpanAt(int i) const;

    ColorRgba evalLinear(double t) const;
};

#endif // __VSDK_TOOLKIT_MEDIA_RGBACOLORPALETTE_H__
