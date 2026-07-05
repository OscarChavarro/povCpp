#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "environment/geometry/surface/TriangleMesh.h"

static constexpr int X_AXIS = 0;
static constexpr int Y_AXIS = 1;
static constexpr int Z_AXIS = 2;

TriangleMesh::TriangleMesh()
{
}

TriangleMesh::TriangleMesh(const TriangleMesh &other) :
    vertices(other.vertices),
    triangles(other.triangles)
{
}

int
TriangleMesh::getTriangleCount() const
{
    return (int)triangles.size();
}

bool
TriangleMesh::isDegenerate(int index) const
{
    return triangles.get(index).degenerateFlag;
}

int
TriangleMesh::max3Axis(double x, double y, double z)
{
    return (x > y) ? ((x > z) ? 1 : 3) : ((y > z) ? 2 : 3);
}

void
TriangleMesh::computeTriangle(int index)
{
    Triangle &triangle = triangles[index];
    const Vector3Dd &p1 = vertices[triangle.v0];
    const Vector3Dd &p2 = vertices[triangle.v1];
    const Vector3Dd &p3 = vertices[triangle.v2];

    Vector3Dd v1 = p1.subtract(p2);
    Vector3Dd v2 = p3.subtract(p2);
    triangle.normal = v1.crossProduct(v2);
    const double length = triangle.normal.length();

    if (length < 1.0e-9) {
        triangle.degenerateFlag = true;
        return;
    }

    triangle.normal = triangle.normal.multiply(1.0 / length);
    triangle.distance = -triangle.normal.dotProduct(p1);

    switch (TriangleMesh::max3Axis(
        java::Math::abs(triangle.normal.x()),
        java::Math::abs(triangle.normal.y()),
        java::Math::abs(triangle.normal.z()))) {
    case 1:
        triangle.dominantAxis = X_AXIS;
        break;
    case 2:
        triangle.dominantAxis = Y_AXIS;
        break;
    case 3:
        triangle.dominantAxis = Z_AXIS;
        break;
    }

    bool swap = false;
    switch (triangle.dominantAxis) {
    case X_AXIS:
        swap = (p2.y() - p3.y()) * (p2.z() - p1.z()) <
            (p2.z() - p3.z()) * (p2.y() - p1.y());
        break;
    case Y_AXIS:
        swap = (p2.x() - p3.x()) * (p2.z() - p1.z()) <
            (p2.z() - p3.z()) * (p2.x() - p1.x());
        break;
    case Z_AXIS:
        swap = (p2.x() - p3.x()) * (p2.y() - p1.y()) <
            (p2.y() - p3.y()) * (p2.x() - p1.x());
        break;
    }
    if (swap) {
        int tmp = triangle.v0;
        triangle.v0 = triangle.v1;
        triangle.v1 = tmp;
    }
}

int
TriangleMesh::addTriangle(
    const Vector3Dd &p1, const Vector3Dd &p2, const Vector3Dd &p3, bool inverted)
{
    Triangle triangle;
    triangle.v0 = (int)vertices.size();
    vertices.add(p1);
    triangle.v1 = (int)vertices.size();
    vertices.add(p2);
    triangle.v2 = (int)vertices.size();
    vertices.add(p3);
    triangle.inverted = inverted;

    const int index = (int)triangles.size();
    triangles.add(triangle);
    computeTriangle(index);
    return index;
}

int
TriangleMesh::appendFrom(const TriangleMesh &source, int sourceIndex)
{
    Triangle triangle = source.triangles.get(sourceIndex);
    const Vector3Dd sourceP0 = source.vertices.get(triangle.v0);
    const Vector3Dd sourceP1 = source.vertices.get(triangle.v1);
    const Vector3Dd sourceP2 = source.vertices.get(triangle.v2);

    triangle.v0 = (int)vertices.size();
    vertices.add(sourceP0);
    triangle.v1 = (int)vertices.size();
    vertices.add(sourceP1);
    triangle.v2 = (int)vertices.size();
    vertices.add(sourceP2);
    triangle.vpCached = 0;

    const int index = (int)triangles.size();
    triangles.add(triangle);
    return index;
}

bool
TriangleMesh::intersectTriangle(RayWithTracingState *ray, int index, double *depth)
{
    Triangle &triangle = triangles[index];
    double normalDotOrigin;
    double normalDotDirection;
    double s;
    double t;
    Statistics &stats = *ray->getStatistics();

    stats.getGeometryStatistics()->incrementRayTriangleTests();
    if (triangle.degenerateFlag) {
        return false;
    }

    if (ray->isPrimaryRayEnabled()) {
        if (!triangle.vpCached) {
            triangle.vpNormDotOrigin = triangle.normal.dotProduct(ray->getOrigin());
            triangle.vpNormDotOrigin += triangle.distance;
            triangle.vpNormDotOrigin *= -1.0;
            triangle.vpCached = true;
        }

        normalDotDirection = triangle.normal.dotProduct(ray->getDirection());
        if ((normalDotDirection < Config::SMALL_TOLERANCE) &&
            (normalDotDirection > -Config::SMALL_TOLERANCE)) {
            return false;
        }

        *depth = triangle.vpNormDotOrigin / normalDotDirection;
    } else {
        normalDotOrigin = triangle.normal.dotProduct(ray->getOrigin());
        normalDotOrigin += triangle.distance;
        normalDotOrigin *= -1.0;

        normalDotDirection = triangle.normal.dotProduct(ray->getDirection());
        if ((normalDotDirection < Config::SMALL_TOLERANCE) &&
            (normalDotDirection > -Config::SMALL_TOLERANCE)) {
            return false;
        }

        *depth = normalDotOrigin / normalDotDirection;
    }

    if ((*depth < Config::SMALL_TOLERANCE) || (*depth > Config::MAX_DISTANCE)) {
        return false;
    }

    const Vector3Dd &p1 = vertices[triangle.v0];
    const Vector3Dd &p2 = vertices[triangle.v1];
    const Vector3Dd &p3 = vertices[triangle.v2];

    switch (triangle.dominantAxis) {
    case X_AXIS:
        s = ray->getOrigin().y() + *depth * ray->getDirection().y();
        t = ray->getOrigin().z() + *depth * ray->getDirection().z();

        if (((p2.y() - s) * (p2.z() - p1.z())) < ((p2.z() - t) * (p2.y() - p1.y()))) {
            if ((int)triangle.inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return true;
            }
            return false;
        }
        if (((p3.y() - s) * (p3.z() - p2.z())) < ((p3.z() - t) * (p3.y() - p2.y()))) {
            if ((int)triangle.inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return true;
            }
            return false;
        }
        if (((p1.y() - s) * (p1.z() - p3.z())) < ((p1.z() - t) * (p1.y() - p3.y()))) {
            if ((int)triangle.inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return true;
            }
            return false;
        }
        if (!(int)triangle.inverted) {
            stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
            return true;
        }
        return false;

    case Y_AXIS:
        s = ray->getOrigin().x() + *depth * ray->getDirection().x();
        t = ray->getOrigin().z() + *depth * ray->getDirection().z();

        if ((p2.x() - s) * (p2.z() - p1.z()) < (p2.z() - t) * (p2.x() - p1.x())) {
            if ((int)triangle.inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return true;
            }
            return false;
        }
        if ((p3.x() - s) * (p3.z() - p2.z()) < (p3.z() - t) * (p3.x() - p2.x())) {
            if ((int)triangle.inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return true;
            }
            return false;
        }
        if ((p1.x() - s) * (p1.z() - p3.z()) < (p1.z() - t) * (p1.x() - p3.x())) {
            if ((int)triangle.inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return true;
            }
            return false;
        }
        if (!(int)triangle.inverted) {
            stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
            return true;
        }
        return false;

    case Z_AXIS:
        s = ray->getOrigin().x() + *depth * ray->getDirection().x();
        t = ray->getOrigin().y() + *depth * ray->getDirection().y();

        if ((p2.x() - s) * (p2.y() - p1.y()) < (p2.y() - t) * (p2.x() - p1.x())) {
            if ((int)triangle.inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return true;
            }
            return false;
        }
        if ((p3.x() - s) * (p3.y() - p2.y()) < (p3.y() - t) * (p3.x() - p2.x())) {
            if ((int)triangle.inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return true;
            }
            return false;
        }
        if ((p1.x() - s) * (p1.y() - p3.y()) < (p1.y() - t) * (p1.x() - p3.x())) {
            if ((int)triangle.inverted) {
                stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
                return true;
            }
            return false;
        }
        if (!(int)triangle.inverted) {
            stats.getGeometryStatistics()->incrementRayTriangleTestsSucceeded();
            return true;
        }
        return false;
    }
    return false;
}

int
TriangleMesh::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    int foundAny = 0;
    for (int i = 0; i < (int)triangles.size(); i++) {
        double depth;
        if (!intersectTriangle(ray, i, &depth)) {
            continue;
        }

        IntersectionCandidate localElement;
        localElement.getIntersection().t = depth;
        localElement.getIntersection().point =
            ray->getDirection().multiply(depth).add(ray->getOrigin());
        localElement.getIntersection().normal = triangles[i].normal;
        localElement.getAttributes().setHitGeometry(this);
        localElement.getAttributes().setMaterial(materialOverride);
        depthQueue->offer(localElement);
        foundAny = 1;
    }
    return foundAny;
}

int
TriangleMesh::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    int foundAny = 0;
    for (int i = 0; i < (int)triangles.size(); i++) {
        double depth;
        if (!intersectTriangle(ray, i, &depth)) {
            continue;
        }

        IntersectionCandidate localElement;
        localElement.getIntersection().t = depth;
        localElement.getIntersection().point =
            ray->getDirection().multiply(depth).add(ray->getOrigin());
        localElement.getIntersection().normal = triangles[i].normal;
        localElement.getAttributes().setHitGeometry(this);
        localElement.getAttributes().setMaterial(context.materialOverride);
        localElement.getAttributes().pushDetailOwner(context.detailOwner);
        localElement.getAttributes().setMaterialUsesObjectLocalPoint(
            context.materialUsesObjectLocalPoint);
        depthQueue->offer(localElement);
        foundAny = 1;
    }
    return foundAny;
}

int
TriangleMesh::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    (void)point;
    (void)distanceTolerance;
    return OUTSIDE;
}

void
TriangleMesh::doExtraInformation(const RayWithTracingState &ray, double t, PovRayHit *hit)
{
    (void)ray;
    (void)t;
    if (!hit->needsNormal()) {
        return;
    }
    if (hit->n.length() > 1.0e-12) {
        return;
    }
    hit->n = Vector3Dd(0.0, 1.0, 0.0);
}

void *
TriangleMesh::copy()
{
    return new TriangleMesh(*this);
}

void
TriangleMesh::invertGeometry()
{
    for (long int i = 0; i < triangles.size(); i++) {
        triangles[i].inverted ^= true;
    }
}

AxisAlignedBoundingBox
TriangleMesh::getMinMax() const
{
    AxisAlignedBoundingBox box = AxisAlignedBoundingBox::empty();
    for (long int i = 0; i < triangles.size(); i++) {
        const Triangle &triangle = triangles.get(i);
        box = box.expandedBy(vertices.get(triangle.v0));
        box = box.expandedBy(vertices.get(triangle.v1));
        box = box.expandedBy(vertices.get(triangle.v2));
    }
    return box;
}
