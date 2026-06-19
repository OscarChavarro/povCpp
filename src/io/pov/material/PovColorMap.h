#ifndef __POV_COLOR_MAP__
#define __POV_COLOR_MAP__

#include "java/util/ArrayList.h"
#include "io/pov/material/PovColorMapSpan.h"

/**
POV-ray color_map: a set of explicit-range spans, each interpolating between
two RGBA stops over [start, end]. Decorator over the library-level RGBAColorPalette
to encapsulate the span semantics specific to POV-ray scene parsing.
*/
class PovColorMap {
  private:
    java::ArrayList<PovColorMapSpan*> _spans;
    bool _transparencyFlag;

  public:
    PovColorMap();
    ~PovColorMap();

    void addSpan(double start, double end,
                 const ColorRgba& startColor, const ColorRgba& endColor);

    int size() const;
    const PovColorMapSpan *getSpanAt(int i) const;
    ColorRgba evalLinear(double t) const;
};

#endif
