#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "java/lang/IllegalStateException.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"

QuadricSurface::QuadricSurface(const Vector3Dd& s, const Vector3Dd& m,
    const Vector3Dd& l, double c) : square(s), mixed(m), linear(l), constant(c) {}
Geometry* QuadricSurface::copyGeometry() const { return new QuadricSurface(*this); }

RayFirstHitStatus QuadricSurface::roots(const Ray& ray, const RayCrossingQuery& query,
    double& first, double& second) const {
    const Vector3Dd& o = ray.getOrigin();
    const Vector3Dd& d = ray.getDirection();
    if (!java::Double::isFinite(o.x()) || !java::Double::isFinite(o.y()) || !java::Double::isFinite(o.z()) ||
        !java::Double::isFinite(d.x()) || !java::Double::isFinite(d.y()) || !java::Double::isFinite(d.z()) ||
        (d.x()==0 && d.y()==0 && d.z()==0) || java::Double::isNaN(query.minT) ||
        java::Double::isNaN(query.maxT) || query.minT > query.maxT) return RayFirstHitStatus::InvalidInput;
    if (query.minimumChordSquared != 0 || query.minimumExteriorApproach != java::Double::NEGATIVE_INFINITY)
        return RayFirstHitStatus::Unsupported;
    const double a = square.x()*d.x()*d.x() + square.y()*d.y()*d.y() + square.z()*d.z()*d.z() +
        mixed.x()*d.x()*d.y() + mixed.y()*d.x()*d.z() + mixed.z()*d.y()*d.z();
    const double b = gradient(o).dotProduct(d);
    const double c = evaluate(o);
    if (!java::Double::isFinite(a) || !java::Double::isFinite(b) || !java::Double::isFinite(c))
        return RayFirstHitStatus::Indeterminate;
    if (a==0 && b==0 && c==0) return RayFirstHitStatus::Indeterminate;
    if (a != 0 && !java::Double::isFinite(b*b - 4.0*a*c)) return RayFirstHitStatus::Indeterminate;
    if (!solve(a,b,c,first,second)) return RayFirstHitStatus::Miss;
    if (!java::Double::isFinite(first) || !java::Double::isFinite(second)) return RayFirstHitStatus::Indeterminate;
    if (first > second) { const double tmp=first; first=second; second=tmp; }
    return RayFirstHitStatus::Hit;
}
void QuadricSurface::getBounds(GeometryBounds& result) const
{
    if (!java::Double::isFinite(square.x()) || !java::Double::isFinite(square.y()) ||
        !java::Double::isFinite(square.z()) || !java::Double::isFinite(mixed.x()) ||
        !java::Double::isFinite(mixed.y()) || !java::Double::isFinite(mixed.z()) ||
        !java::Double::isFinite(linear.x()) || !java::Double::isFinite(linear.y()) ||
        !java::Double::isFinite(linear.z()) || !java::Double::isFinite(constant)) {
        result.setIndeterminate(); return;
    }
    // POV's positive-definite ellipsoid bound. Other signs and singular or
    // poorly conditioned matrices remain conservatively unbounded.
    const double a11 = square.x(), a22 = square.y(), a33 = square.z();
    const double a12 = mixed.x()*0.5, a13 = mixed.y()*0.5, a23 = mixed.z()*0.5;
    const double minor1 = a11;
    const double minor2 = a11*a22 - a12*a12;
    const double det = a11*(a22*a33 - a23*a23) - a12*(a12*a33 - a23*a13) +
        a13*(a12*a23 - a22*a13);
    if (!java::Double::isFinite(minor2) || !java::Double::isFinite(det)) {
        result.setIndeterminate(); return;
    }
    if (minor1 <= 1e-12 || minor2 <= 1e-12 || det <= 1e-12) {
        result.setUnbounded(); return;
    }
    const double invDet = 1.0/det;
    const double inv11 = (a22*a33 - a23*a23)*invDet;
    const double inv12 = (a13*a23 - a12*a33)*invDet;
    const double inv13 = (a12*a23 - a13*a22)*invDet;
    const double inv22 = (a11*a33 - a13*a13)*invDet;
    const double inv23 = (a12*a13 - a11*a23)*invDet;
    const double inv33 = (a11*a22 - a12*a12)*invDet;
    if (!java::Double::isFinite(inv11) || !java::Double::isFinite(inv12) ||
        !java::Double::isFinite(inv13) || !java::Double::isFinite(inv22) ||
        !java::Double::isFinite(inv23) || !java::Double::isFinite(inv33)) {
        result.setIndeterminate(); return;
    }
    const Vector3Dd center(
        -0.5*(inv11*linear.x()+inv12*linear.y()+inv13*linear.z()),
        -0.5*(inv12*linear.x()+inv22*linear.y()+inv23*linear.z()),
        -0.5*(inv13*linear.x()+inv23*linear.y()+inv33*linear.z()));
    const Vector3Dd aCenter(
        a11*center.x()+a12*center.y()+a13*center.z(),
        a12*center.x()+a22*center.y()+a23*center.z(),
        a13*center.x()+a23*center.y()+a33*center.z());
    const double radiusSquared = center.dotProduct(aCenter)-constant;
    if (!java::Double::isFinite(radiusSquared)) { result.setIndeterminate(); return; }
    if (radiusSquared <= 1e-12 || inv11 <= 0 || inv22 <= 0 || inv33 <= 0) {
        result.setUnbounded(); return;
    }
    const Vector3Dd extent(java::Math::sqrt(radiusSquared*inv11),
        java::Math::sqrt(radiusSquared*inv22), java::Math::sqrt(radiusSquared*inv33));
    result.setFinite(center.subtract(extent), center.add(extent));
}
FloatingInterval QuadricSurface::enclose(const FloatingInterval& x, const FloatingInterval& y,
    const FloatingInterval& z) const
{
    FloatingInterval result = FloatingInterval::exact(constant);
    result = result.add(FloatingInterval::exact(linear.x()).multiply(x));
    result = result.add(FloatingInterval::exact(linear.y()).multiply(y));
    result = result.add(FloatingInterval::exact(linear.z()).multiply(z));
    result = result.add(FloatingInterval::exact(square.x()).multiply(x.square()));
    result = result.add(FloatingInterval::exact(square.y()).multiply(y.square()));
    result = result.add(FloatingInterval::exact(square.z()).multiply(z.square()));
    result = result.add(FloatingInterval::exact(mixed.x()).multiply(x.multiply(y)));
    result = result.add(FloatingInterval::exact(mixed.y()).multiply(x.multiply(z)));
    return result.add(FloatingInterval::exact(mixed.z()).multiply(y.multiply(z)));
}

GeometryContainment QuadricSurface::classifyBoxCertified(const Vector3Dd& center,
    const Vector3Dd& halfWidths) const
{
    const double coefficients[10] = {square.x(), square.y(), square.z(), mixed.x(), mixed.y(),
        mixed.z(), linear.x(), linear.y(), linear.z(), constant};
    for (int i = 0; i < 10; ++i)
        if (!java::Double::isFinite(coefficients[i])) return GeometryContainment::InvalidInput;
    if (!FloatingInterval::isSupportedEnvironment()) return GeometryContainment::Unsupported;
    const FloatingInterval x = FloatingInterval::around(center.x(), halfWidths.x());
    const FloatingInterval y = FloatingInterval::around(center.y(), halfWidths.y());
    const FloatingInterval z = FloatingInterval::around(center.z(), halfWidths.z());
    if (!x.isValid() || !y.isValid() || !z.isValid()) return GeometryContainment::InvalidInput;
    const FloatingInterval range = enclose(x, y, z);
    if (!range.isValid()) return GeometryContainment::Indeterminate;
    if (range.getUpper() < 0) return GeometryContainment::Inside;
    if (range.getLower() > 0) return GeometryContainment::Outside;
    return GeometryContainment::Indeterminate;
}

GeometryContainment QuadricSurface::classifyPointCertified(const Vector3Dd& point, double tolerance) const
{
    if (!java::Double::isFinite(tolerance) || tolerance < 0 || !java::Double::isFinite(point.x()) ||
        !java::Double::isFinite(point.y()) || !java::Double::isFinite(point.z()))
        return GeometryContainment::InvalidInput;
    const double coefficients[10] = {square.x(), square.y(), square.z(), mixed.x(), mixed.y(),
        mixed.z(), linear.x(), linear.y(), linear.z(), constant};
    for (int i = 0; i < 10; ++i)
        if (!java::Double::isFinite(coefficients[i])) return GeometryContainment::InvalidInput;
    if (!FloatingInterval::isSupportedEnvironment()) return GeometryContainment::Unsupported;
    const double coordinates[3] = {point.x(), point.y(), point.z()};
    FloatingInterval box[3];
    for (int i = 0; i < 3; ++i) {
        box[i] = FloatingInterval::around(coordinates[i], tolerance);
        if (!box[i].isValid()) return GeometryContainment::Indeterminate;
    }
    const FloatingInterval range = enclose(box[0], box[1], box[2]);
    if (!range.isValid()) return GeometryContainment::Indeterminate;
    if (range.getUpper() < 0) return GeometryContainment::Inside;
    if (range.getLower() > 0) return GeometryContainment::Outside;
    if (tolerance == 0) return GeometryContainment::Indeterminate;
    // F is continuous and the ball is convex: certified opposite signs at two
    // points of the ball prove a boundary point on the segment joining them.
    const Vector3Dd g = gradient(point);
    const double length = g.length();
    if (!(length > 0) || !java::Double::isFinite(length)) return GeometryContainment::Indeterminate;
    const Vector3Dd unit = g.multiply(1.0 / length);
    const double steps[2] = {tolerance * (1.0 - 1.0 / 1048576.0), tolerance * 0.5};
    for (int s = 0; s < 2; ++s) {
        int signs[2] = {0, 0};
        for (int side = 0; side < 2; ++side) {
            const Vector3Dd q = point.add(unit.multiply(side == 0 ? -steps[s] : steps[s]));
            FloatingInterval offset = FloatingInterval::exact(0);
            offset = offset.add(FloatingInterval::exact(q.x()).subtract(FloatingInterval::exact(point.x())).square());
            offset = offset.add(FloatingInterval::exact(q.y()).subtract(FloatingInterval::exact(point.y())).square());
            offset = offset.add(FloatingInterval::exact(q.z()).subtract(FloatingInterval::exact(point.z())).square());
            const FloatingInterval distance = offset.squareRoot();
            if (!distance.isValid() || distance.getUpper() > tolerance) continue;
            const FloatingInterval value = enclose(FloatingInterval::exact(q.x()),
                FloatingInterval::exact(q.y()), FloatingInterval::exact(q.z()));
            if (!value.isValid()) continue;
            signs[side] = value.getUpper() < 0 ? -1 : value.getLower() > 0 ? 1 : 0;
        }
        if (signs[0] * signs[1] < 0) return GeometryContainment::Boundary;
        // The centre itself may be the second point.
        const FloatingInterval centre = enclose(FloatingInterval::exact(point.x()),
            FloatingInterval::exact(point.y()), FloatingInterval::exact(point.z()));
        if (centre.isValid()) {
            const int sign = centre.getUpper() < 0 ? -1 : centre.getLower() > 0 ? 1 : 0;
            if (sign != 0 && (signs[0] == -sign || signs[1] == -sign)) return GeometryContainment::Boundary;
        }
    }
    return GeometryContainment::Indeterminate;
}

GeometryContainment QuadricSurface::classifyPoint(const Vector3Dd& point, double tolerance) const {
    if (!java::Double::isFinite(tolerance) || tolerance < 0) return GeometryContainment::InvalidInput;
    if (tolerance != 0) return classifyPointCertified(point, tolerance);
    const double value = evaluate(point);
    if (!java::Double::isFinite(value)) return GeometryContainment::Indeterminate;
    return value < 0 ? GeometryContainment::Inside : value > 0 ? GeometryContainment::Outside : GeometryContainment::Boundary;
}
RayFirstHitStatus QuadricSurface::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* hit, RayBoundary* boundary) const {
    if (hit) hit->clear();
    if (boundary) *boundary = RayBoundary();
    double a,b; const RayFirstHitStatus status=roots(ray,query,a,b);
    if (status != RayFirstHitStatus::Hit) return status;
    const double t=a>=query.minT ? a : b;
    if (t<query.minT || t>query.maxT) return RayFirstHitStatus::Miss;
    if (boundary) *boundary=RayBoundary(t,this);
    if (hit) { hit->setHitDistance(t); if (hit->shouldStoreRay()) hit->setRay(ray.withT(t));
        if (hit->needsAnySurfaceData()) const_cast<QuadricSurface*>(this)->doExtraInformation(ray,t,hit); }
    return RayFirstHitStatus::Hit;
}
RayFirstHitStatus QuadricSurface::querySurfaceDetail(const Ray& ray,
    const RayBoundary& boundary, RayHit* hit) const
{
    if (hit) hit->clear();
    if (!hit || boundary.geometry != this || !java::Double::isFinite(boundary.t))
        return RayFirstHitStatus::InvalidInput;
    if (boundary.getInstanceCount() != 0) return RayFirstHitStatus::Unsupported;
    const RayFirstHitStatus status = queryFirstHit(ray,
        RayCrossingQuery(boundary.t, boundary.t), nullptr);
    if (status != RayFirstHitStatus::Hit) return status;
    hit->setHitDistance(boundary.t);
    if (hit->shouldStoreRay()) hit->setRay(ray.withT(boundary.t));
    const_cast<QuadricSurface*>(this)->doExtraInformation(ray, boundary.t, hit);
    if (boundary.normalReversed && hit->needsNormal() &&
        (hit->getAvailableDetailMask() & RayHit::DETAIL_NORMAL))
        hit->normal = hit->normal.multiply(-1);
    return RayFirstHitStatus::Hit;
}
RayCrossingSupport QuadricSurface::emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
    RayCrossingSink& sink) const {
    double a,b; const RayFirstHitStatus status=roots(ray,query,a,b);
    if (status == RayFirstHitStatus::Miss) return RayCrossingSupport::Supported;
    if (status == RayFirstHitStatus::InvalidInput) return RayCrossingSupport::InvalidInput;
    if (status == RayFirstHitStatus::Unsupported) return RayCrossingSupport::Unsupported;
    if (status != RayFirstHitStatus::Hit) return RayCrossingSupport::Indeterminate;
    const Vector3Dd direction=ray.getDirection();
    const bool emitFirst = a>=query.minT && a<=query.maxT;
    const bool emitSecond = a!=b && b>=query.minT && b<=query.maxT;
    double slopeFirst = 0, slopeSecond = 0;
    if (emitFirst) {
        slopeFirst=gradient(ray.getOrigin().add(direction.multiply(a))).dotProduct(direction);
        if (!java::Double::isFinite(slopeFirst) || (a != b && slopeFirst == 0)) return RayCrossingSupport::Indeterminate;
    }
    if (emitSecond) {
        slopeSecond=gradient(ray.getOrigin().add(direction.multiply(b))).dotProduct(direction);
        if (!java::Double::isFinite(slopeSecond) || slopeSecond == 0) return RayCrossingSupport::Indeterminate;
    }
    if (emitFirst) sink.crossing(RayCrossing(a,this,slopeFirst == 0 ? RayCrossing::Kind::Tangent :
        slopeFirst<0 ? RayCrossing::Kind::Entering : RayCrossing::Kind::Leaving));
    if (emitSecond) sink.crossing(RayCrossing(b,this,
        slopeSecond<0 ? RayCrossing::Kind::Entering : RayCrossing::Kind::Leaving));
    return RayCrossingSupport::Supported;
}
RaySolidQueryStatus QuadricSurface::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    result.clear();
    if (java::Double::isNaN(query.minT) || java::Double::isNaN(query.maxT) ||
        query.minT >= query.maxT) return RaySolidQueryStatus::InvalidInput;
    double first = 0, second = 0;
    const RayFirstHitStatus status = roots(ray, query, first, second);
    if (status == RayFirstHitStatus::InvalidInput) return RaySolidQueryStatus::InvalidInput;
    if (status == RayFirstHitStatus::Unsupported) return RaySolidQueryStatus::Unsupported;
    if (status == RayFirstHitStatus::Indeterminate) return RaySolidQueryStatus::Indeterminate;
    if (status == RayFirstHitStatus::Miss) {
        const double value = evaluate(ray.getOrigin());
        if (!java::Double::isFinite(value) || value == 0) return RaySolidQueryStatus::Indeterminate;
        return result.insideSegments.add(value < 0) ? RaySolidQueryStatus::Supported :
            RaySolidQueryStatus::Indeterminate;
    }
    const Vector3Dd direction = ray.getDirection();
    const double slopeFirst = gradient(ray.getOrigin().add(direction.multiply(first))).dotProduct(direction);
    if (!java::Double::isFinite(slopeFirst)) return RaySolidQueryStatus::Indeterminate;
    if (first == second && slopeFirst == 0) {
        // A tangent does not change interval membership. F's quadratic term
        // determines whether the ray lies outside or inside on both sides.
        const double a = square.x()*direction.x()*direction.x() +
            square.y()*direction.y()*direction.y() + square.z()*direction.z()*direction.z() +
            mixed.x()*direction.x()*direction.y() + mixed.y()*direction.x()*direction.z() +
            mixed.z()*direction.y()*direction.z();
        if (!java::Double::isFinite(a) || a == 0) return RaySolidQueryStatus::Indeterminate;
        return result.insideSegments.add(a < 0) ? RaySolidQueryStatus::Supported :
            RaySolidQueryStatus::Indeterminate;
    }
    if (slopeFirst == 0) return RaySolidQueryStatus::Indeterminate;
    double slopeSecond = 0;
    if (first != second) {
        slopeSecond = gradient(ray.getOrigin().add(direction.multiply(second))).dotProduct(direction);
        if (!java::Double::isFinite(slopeSecond) || slopeSecond == 0 ||
            (slopeFirst < 0) == (slopeSecond < 0)) return RaySolidQueryStatus::Indeterminate;
    }
    bool inside = slopeFirst > 0;
    if (query.minT > first) inside = slopeFirst < 0;
    if (first != second && query.minT > second) inside = slopeSecond < 0;
    if (!result.insideSegments.add(inside)) return RaySolidQueryStatus::Indeterminate;
    if (first >= query.minT && first <= query.maxT) {
        if (!result.boundaries.add(RaySolidBoundary(first, this)) ||
            !result.insideSegments.add(slopeFirst < 0)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
    }
    if (first != second && second >= query.minT && second <= query.maxT) {
        if (!result.boundaries.add(RaySolidBoundary(second, this)) ||
            !result.insideSegments.add(slopeSecond < 0)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
    }
    return RaySolidQueryStatus::Supported;
}
bool QuadricSurface::doIntersectionFirstHit(const Ray& ray, RayHit* hit) {
    return queryFirstHit(ray,RayCrossingQuery(1e-9,java::Double::POSITIVE_INFINITY),hit)==RayFirstHitStatus::Hit;
}
void QuadricSurface::doExtraInformation(const Ray& ray, double t, RayHit* hit) {
    if (!hit) return;
    hit->beginDetailEvaluation();
    const Vector3Dd point=ray.getOrigin().add(ray.getDirection().multiply(t));
    if (hit->needsPoint()) { hit->point=point; hit->markDetailAvailable(RayHit::DETAIL_POINT); }
    if (hit->needsNormal()) {
        const Vector3Dd normal=gradient(point); const double length=normal.length();
        if (java::Double::isFinite(length) && length>0) {
            hit->normal=normal.multiply(1.0/length); hit->markDetailAvailable(RayHit::DETAIL_NORMAL);
        }
    }
}
int QuadricSurface::doContainmentTest(const Vector3Dd& p, double tolerance) {
    // Legacy integer adapter over the typed world-distance contract.
    const GeometryContainment result = classifyPoint(p, tolerance);
    if (result == GeometryContainment::Inside) return INSIDE;
    if (result == GeometryContainment::Boundary) return LIMIT;
    if (result == GeometryContainment::Outside) return OUTSIDE;
    throw java::IllegalStateException("Quadric containment cannot be represented by the legacy integer result");
}
double* QuadricSurface::getMinMax() {
    GeometryBounds bounds;
    getBounds(bounds);
    if (bounds.getKind() == GeometryBounds::Kind::Finite)
        return new double[6]{bounds.getMinimum().x(), bounds.getMinimum().y(), bounds.getMinimum().z(),
            bounds.getMaximum().x(), bounds.getMaximum().y(), bounds.getMaximum().z()};
    return new double[6]{-1e308,-1e308,-1e308,1e308,1e308,1e308};
}
