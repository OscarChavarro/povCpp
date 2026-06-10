#ifndef __POV_COLOR_MAP_H__
#define __POV_COLOR_MAP_H__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"

/**
POV-ray color_map: a set of explicit-range spans, each interpolating between
two RGBA stops over [start, end]. Decorator over the library-level RGBAColorPalette
to encapsulate the span semantics specific to POV-ray scene parsing.
*/
class PovColorMap {
  public:
    struct Span {
        double start;
        double end;
        ColorRgba startColor;
        ColorRgba endColor;
    };

  private:
    java::ArrayList<Span*> _spans;
    bool _transparencyFlag;

  public:
    PovColorMap();
    ~PovColorMap();

    void addSpan(double start, double end,
                 const ColorRgba& startColor, const ColorRgba& endColor);

    int size() const;
    bool transparencyFlag() const;
    const Span* getSpanAt(int i) const;
    ColorRgba evalLinear(double t) const;
};

#endif // __POV_COLOR_MAP_H__
