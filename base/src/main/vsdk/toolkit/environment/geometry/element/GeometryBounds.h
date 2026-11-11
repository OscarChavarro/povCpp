#ifndef VSDK_GEOMETRY_BOUNDS_H
#define VSDK_GEOMETRY_BOUNDS_H

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

// Values are owned by this result, in the geometry's local coordinates.
class GeometryBounds {
public:
    enum class Kind { Unsupported, Empty, Finite, Unbounded, Indeterminate };
private:
    Kind kind;
    Vector3Dd minimum;
    Vector3Dd maximum;
public:
    GeometryBounds();
    void setUnsupported();
    void setEmpty();
    void setUnbounded();
    void setIndeterminate();
    bool setFinite(const Vector3Dd& minimum, const Vector3Dd& maximum);
    Kind getKind() const;
    const Vector3Dd& getMinimum() const;
    const Vector3Dd& getMaximum() const;
};
#endif
