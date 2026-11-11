#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidMinimumDistance.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistance.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistanceBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDualBound.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/lang/Double.h"

GeometryDistanceStatus AffineEllipsoidMinimumDistance::query(const Vector3Dd& point,
    double radius, const Matrix4x4d& forward, const Matrix4x4d& inverseHint, double maxWidth,
    GeometryContainment& side, double& lower, double& upper)
{
    if (!java::Double::isFinite(maxWidth) || !(maxWidth>0)) return GeometryDistanceStatus::InvalidInput;
    for (int i=0;i<4;++i) for (int j=0;j<4;++j)
        if (!java::Double::isFinite(inverseHint.get(i,j))) return GeometryDistanceStatus::InvalidInput;
    double estimate=0,error=0;
    Vector3Dd witness;
    const GeometryDistanceStatus candidate=AffineEllipsoidDistance::queryWithWitness(
        point,radius,forward,estimate,error,witness);
    if (candidate==GeometryDistanceStatus::InvalidInput) return candidate;
    Vector3Dd seed=candidate==GeometryDistanceStatus::Valid ? inverseHint.multiply(witness) : inverseHint.multiply(point);
    if (!java::Double::isFinite(seed.x()) || !java::Double::isFinite(seed.y()) ||
        !java::Double::isFinite(seed.z())) return GeometryDistanceStatus::Indeterminate;
    if (seed.x()==0 && seed.y()==0 && seed.z()==0) seed=Vector3Dd(1,0,0);
    GeometryContainment candidateSide=GeometryContainment::Indeterminate;
    double lo=0,hi=0;
    const GeometryDistanceStatus status=AffineEllipsoidDistanceBounds::query(
        point,radius,forward,seed,candidateSide,lo,hi);
    if (status!=GeometryDistanceStatus::Valid) return status;
    const double dual=AffineEllipsoidDualBound::lowerDistance(point,radius,forward,seed);
    if (dual>lo) lo=dual;
    if (lo>hi) return GeometryDistanceStatus::Indeterminate;
    const FloatingInterval width=FloatingInterval::exact(hi).subtract(FloatingInterval::exact(lo));
    if (!width.isValid() || width.getUpper()>maxWidth) return GeometryDistanceStatus::Indeterminate;
    side=candidateSide;
    lower=lo;
    upper=hi;
    return GeometryDistanceStatus::Valid;
}
