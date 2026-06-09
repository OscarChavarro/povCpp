#ifndef __VSDK_TOOLKIT_MEDIA_RGBACOLORPALETTESPAN_H__
#define __VSDK_TOOLKIT_MEDIA_RGBACOLORPALETTESPAN_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"

class RGBAColorPaletteSpan {
  public:
    double start;
    double end;
    ColorRgba startColor;
    ColorRgba endColor;
};

#endif // __VSDK_TOOLKIT_MEDIA_RGBACOLORPALETTESPAN_H__
