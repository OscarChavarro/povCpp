#ifndef VSDK_GEOMETRY_BOUNDS_H
#define VSDK_GEOMETRY_BOUNDS_H

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Matrix4x4d;

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
    // Conservative axis-aligned enclosure in the destination coordinates.
    // Non-affine or numerically invalid transforms are indeterminate.
    GeometryBounds transformed(const Matrix4x4d& placement) const;
    // Enclose both bounds; unknown states remain explicit.
    GeometryBounds enclosing(const GeometryBounds& other) const;
};
#endif
