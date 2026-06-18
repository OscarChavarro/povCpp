#ifndef __POV_COLOR_MAP_SPAN_H__
#define __POV_COLOR_MAP_SPAN_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"

class PovColorMapSpan {
  public:
    PovColorMapSpan();

    double getStart() const;
    void setStart(double value);
    double getEnd() const;
    void setEnd(double value);
    const ColorRgba &getStartColor() const;
    ColorRgba &getStartColor();
    void setStartColor(const ColorRgba &value);
    const ColorRgba &getEndColor() const;
    ColorRgba &getEndColor();
    void setEndColor(const ColorRgba &value);

  private:
    double start;
    double end;
    ColorRgba startColor;
    ColorRgba endColor;
};

#endif
