#ifndef __POV_COLOR_MAP_SPAN__
#define __POV_COLOR_MAP_SPAN__

#include "vsdk/toolkit/common/color/ColorRgba.h"

class PovColorMapSpan {
  public:
    PovColorMapSpan();

    double getStart() const;
    void setStart(double value);
    double getEnd() const;
    void setEnd(double value);
    const ColorRgba &getStartColor() const;
    void setStartColor(const ColorRgba &value);
    const ColorRgba &getEndColor() const;
    void setEndColor(const ColorRgba &value);

  private:
    double start;
    double end;
    ColorRgba startColor;
    ColorRgba endColor;
};

#endif
