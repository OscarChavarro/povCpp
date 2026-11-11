#include "java/lang/Double.h"
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
