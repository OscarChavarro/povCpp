#ifndef __TRANSLATE_GIZMO_LINE_SEGMENT__
#define __TRANSLATE_GIZMO_LINE_SEGMENT__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

/**
Immutable colored line segment, as used by translate gizmo guides.
*/
class TranslateGizmoLineSegment {
private:
    Vector3Dd startPoint;
    Vector3Dd endPoint;
    ColorRgb segmentColor;

public:
    TranslateGizmoLineSegment() {}
    TranslateGizmoLineSegment(const Vector3Dd& start, const Vector3Dd& end,
                              const ColorRgb& color)
        : startPoint(start), endPoint(end), segmentColor(color) {}

    const Vector3Dd& start() const { return startPoint; }
    const Vector3Dd& end() const { return endPoint; }
    const ColorRgb& color() const { return segmentColor; }
};

#endif
