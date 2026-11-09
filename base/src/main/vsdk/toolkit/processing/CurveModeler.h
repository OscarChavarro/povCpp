#ifndef __CURVE_MODELER__
#define __CURVE_MODELER__

#include "vsdk/toolkit/processing/ProcessingElement.h"

class ParametricCurve;

/**
Utility class with static geometry algorithms, mostly for creating and
modifying geometric entities.

It complements:
- `ComputationalGeometry` (geometric queries)
- `SimpleTestGeometryLibrary` (sample geometry generation)

This class does not render anything. It only manipulates geometric data
structures and delegates visualization concerns to renderers.
*/
class CurveModeler : public ProcessingElement {
public:
    /**
    Creates a 3D line from point (x1, y1, z1) to point (x2, y2, z2).
    @return a new curve, owned by the caller
    */
    static ParametricCurve* createLine(double x1, double y1, double z1,
                                       double x2, double y2, double z2);
};

#endif
