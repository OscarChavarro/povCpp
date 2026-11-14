#include "TestSlabGeometry.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"

Geometry* TestSlabGeometry::copyGeometry() const { return new TestSlabGeometry(reversed); }

RayCrossingSupport TestSlabGeometry::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    const double x = ray.getOrigin().x();
    const double dx = ray.getDirection().x();
    if (dx == 0) return RayCrossingSupport::Supported;
    double a = (-1.0 - x) / dx, b = (1.0 - x) / dx;
    if (a > b) { const double tmp = a; a = b; b = tmp; }
    RayCrossing crossing(a, this, RayCrossing::Kind::Unclassified);
    crossing.normalReversed = reversed;
    if (a >= query.minT && a <= query.maxT) sink.crossing(crossing);
    crossing.t = b;
    if (b >= query.minT && b <= query.maxT) sink.crossing(crossing);
    return RayCrossingSupport::Supported;
}

RayFirstHitStatus TestSlabGeometry::queryFirstHit(const Ray& ray,
    const RayCrossingQuery& query, RayHit* hit, RayBoundary* boundary) const
{
    const double dx = ray.getDirection().x();
    if (dx == 0) return RayFirstHitStatus::Miss;
    double a = (-1.0 - ray.getOrigin().x()) / dx, b = (1.0 - ray.getOrigin().x()) / dx;
    if (a > b) { const double tmp = a; a = b; b = tmp; }
    if (a < query.minT) a = b;
    if (a < query.minT || a > query.maxT) return RayFirstHitStatus::Miss;
    if (hit != nullptr) hit->setHitDistance(a);
    if (boundary != nullptr) *boundary = RayBoundary(a, this);
    return RayFirstHitStatus::Hit;
}

bool TestSlabGeometry::doIntersectionFirstHit(const Ray& ray, RayHit* hit)
{
    return queryFirstHit(ray, RayCrossingQuery(0, 1e30), hit) == RayFirstHitStatus::Hit;
}

void TestSlabGeometry::doExtraInformation(const Ray& ray, double t, RayHit* hit)
{
    hit->beginDetailEvaluation();
    if (hit->needsNormal()) {
        const double x = ray.getOrigin().x() + t * ray.getDirection().x();
        hit->normal = Vector3Dd(x < 0 ? -1 : 1, 0, 0);
        hit->markDetailAvailable(RayHit::DETAIL_NORMAL);
    }
}

GeometryContainment TestSlabGeometry::classifyPoint(const Vector3Dd& point, double tolerance) const
{
    if (tolerance < 0) return GeometryContainment::InvalidInput;
    const double x = point.x() < 0 ? -point.x() : point.x();
    if (x < 1 - tolerance) return GeometryContainment::Inside;
    if (x > 1 + tolerance) return GeometryContainment::Outside;
    return GeometryContainment::Boundary;
}

void TestSlabGeometry::getBounds(GeometryBounds& bounds) const { bounds.setUnbounded(); }
double* TestSlabGeometry::getMinMax() { return nullptr; }
