#include "java/lang/Double.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"

GeometryBounds::GeometryBounds() : kind(Kind::Unsupported), minimum(), maximum() {}
void GeometryBounds::setUnsupported() { kind = Kind::Unsupported; minimum = Vector3Dd(); maximum = Vector3Dd(); }
void GeometryBounds::setEmpty() { kind = Kind::Empty; minimum = Vector3Dd(); maximum = Vector3Dd(); }
void GeometryBounds::setUnbounded() { kind = Kind::Unbounded; minimum = Vector3Dd(); maximum = Vector3Dd(); }
void GeometryBounds::setIndeterminate() { kind = Kind::Indeterminate; minimum = Vector3Dd(); maximum = Vector3Dd(); }
bool GeometryBounds::setFinite(const Vector3Dd& lo, const Vector3Dd& hi)
{
    if (!java::Double::isFinite(lo.x()) || !java::Double::isFinite(lo.y()) ||
        !java::Double::isFinite(lo.z()) || !java::Double::isFinite(hi.x()) ||
        !java::Double::isFinite(hi.y()) || !java::Double::isFinite(hi.z()) ||
        lo.x() > hi.x() || lo.y() > hi.y() || lo.z() > hi.z()) {
        setIndeterminate();
        return false;
    }
    minimum = lo; maximum = hi; kind = Kind::Finite; return true;
}
GeometryBounds::Kind GeometryBounds::getKind() const { return kind; }
const Vector3Dd& GeometryBounds::getMinimum() const { return minimum; }
const Vector3Dd& GeometryBounds::getMaximum() const { return maximum; }

GeometryBounds GeometryBounds::transformed(const Matrix4x4d& placement) const
{
    GeometryBounds result;
    if (kind == Kind::Empty) { result.setEmpty(); return result; }
    if (kind == Kind::Unsupported) return result;
    if (kind == Kind::Indeterminate) { result.setIndeterminate(); return result; }
    for (int row = 0; row < 4; ++row) for (int column = 0; column < 4; ++column) {
        if (!java::Double::isFinite(placement.get(row, column))) {
            result.setIndeterminate(); return result;
        }
    }
    if (placement.get(3,0) != 0 || placement.get(3,1) != 0 ||
        placement.get(3,2) != 0 || placement.get(3,3) != 1) {
        result.setIndeterminate(); return result;
    }
    if (kind == Kind::Unbounded) { result.setUnbounded(); return result; }
    Vector3Dd lo, hi;
    bool first = true;
    for (int ix = 0; ix < 2; ++ix) for (int iy = 0; iy < 2; ++iy) for (int iz = 0; iz < 2; ++iz) {
        const Vector3Dd point(ix ? maximum.x() : minimum.x(),
            iy ? maximum.y() : minimum.y(), iz ? maximum.z() : minimum.z());
        const Vector3Dd corner = placement.multiply(point);
        if (!java::Double::isFinite(corner.x()) || !java::Double::isFinite(corner.y()) ||
            !java::Double::isFinite(corner.z())) {
            result.setIndeterminate(); return result;
        }
        if (first) { lo = hi = corner; first = false; continue; }
        lo = Vector3Dd(corner.x() < lo.x() ? corner.x() : lo.x(),
            corner.y() < lo.y() ? corner.y() : lo.y(), corner.z() < lo.z() ? corner.z() : lo.z());
        hi = Vector3Dd(corner.x() > hi.x() ? corner.x() : hi.x(),
            corner.y() > hi.y() ? corner.y() : hi.y(), corner.z() > hi.z() ? corner.z() : hi.z());
    }
    result.setFinite(lo, hi);
    return result;
}

GeometryBounds GeometryBounds::enclosing(const GeometryBounds& other) const
{
    GeometryBounds result;
    if (kind == Kind::Empty) return other;
    if (other.kind == Kind::Empty) return *this;
    if (kind == Kind::Indeterminate || other.kind == Kind::Indeterminate) {
        result.setIndeterminate(); return result;
    }
    if (kind == Kind::Unsupported || other.kind == Kind::Unsupported) return result;
    if (kind == Kind::Unbounded || other.kind == Kind::Unbounded) {
        result.setUnbounded(); return result;
    }
    result.setFinite(Vector3Dd(
        minimum.x() < other.minimum.x() ? minimum.x() : other.minimum.x(),
        minimum.y() < other.minimum.y() ? minimum.y() : other.minimum.y(),
        minimum.z() < other.minimum.z() ? minimum.z() : other.minimum.z()),
        Vector3Dd(maximum.x() > other.maximum.x() ? maximum.x() : other.maximum.x(),
        maximum.y() > other.maximum.y() ? maximum.y() : other.maximum.y(),
        maximum.z() > other.maximum.z() ? maximum.z() : other.maximum.z()));
    return result;
}
