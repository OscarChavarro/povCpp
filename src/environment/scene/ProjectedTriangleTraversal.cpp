#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "java/lang/IllegalArgumentException.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/scene/NativeGeometryBinding.h"
#include "environment/scene/ProjectedTriangleTraversal.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

ProjectedTriangleTraversal::ProjectedTriangleTraversal(TriangleMesh* mesh) : geometry(mesh)
{
    GeometryBounds bounds;
    geometry->getBounds(bounds);
    if (bounds.getKind() != GeometryBounds::Kind::Empty && bounds.getKind() != GeometryBounds::Kind::Finite)
        throw java::IllegalArgumentException("Triangle mesh has invalid bounds or indices");
    for (int i = 0; i < geometry->getNumTriangles(); ++i) {
        initializeMetadata(false);
        computeTriangle(i, false);
    }
}
ProjectedTriangleTraversal::ProjectedTriangleTraversal(const ProjectedTriangleTraversal& other, TriangleMesh* mesh) :
    geometry(mesh), triangleDistance(other.triangleDistance), triangleNormals(other.triangleNormals),
    triangleDominantAxis(other.triangleDominantAxis), triangleProjectionSwap(other.triangleProjectionSwap),
    triangleInverted(other.triangleInverted),
    triangleDegenerate(other.triangleDegenerate), triangleVpCached(other.triangleVpCached),
    triangleVpNormDotOrigin(other.triangleVpNormDotOrigin)
{
    for (long int i = 0; i < triangleVpCached.size(); ++i) triangleVpCached[i] = false;
}
ProjectedTriangleTraversal::~ProjectedTriangleTraversal() {}

int
ProjectedTriangleTraversal::getTriangleCount() const
{
    return geometry->getNumTriangles();
}

bool
ProjectedTriangleTraversal::isDegenerate(int index) const
{
    return triangleDegenerate.get(index);
}

bool ProjectedTriangleTraversal::hasUnboundedFaces() const
{
    for (int i = 0; i < getTriangleCount(); ++i)
        if (triangleInverted.get(i) && !triangleDegenerate.get(i)) return true;
    return false;
}

void
ProjectedTriangleTraversal::getTrianglePoints(int index, Vector3Dd& p0, Vector3Dd& p1, Vector3Dd& p2) const
{
    const java::ArrayList<int>& indices = geometry->getTriangleIndexesView();
    const java::ArrayList<double>& vertices = geometry->getVertexPositionsView();
    const int a = indices[index*3]*3, b = indices[index*3+1]*3, c = indices[index*3+2]*3;
    p0 = Vector3Dd(vertices[a], vertices[a+1], vertices[a+2]);
    p1 = Vector3Dd(vertices[b], vertices[b+1], vertices[b+2]);
    p2 = Vector3Dd(vertices[c], vertices[c+1], vertices[c+2]);
}

Vector3Dd
ProjectedTriangleTraversal::getTriangleNormal(int index) const
{
    return triangleNormals[index];
}

Vector3Dd
ProjectedTriangleTraversal::getNativeNormal(int index) const
{
    const java::ArrayList<double>& normals = geometry->getTriangleNormalsView();
    if (normals.size() >= (long)(index+1)*3)
        return Vector3Dd(normals[index*3], normals[index*3+1], normals[index*3+2]);
    Vector3Dd p0, p1, p2;
    getTrianglePoints(index, p0, p1, p2);
    return p1.subtract(p0).crossProduct(p2.subtract(p0)).normalized();
}

void ProjectedTriangleTraversal::initializeMetadata(bool inverted)
{
    triangleDistance.add(0.0);
    triangleNormals.add(Vector3Dd());
    triangleDominantAxis.add(0);
    triangleProjectionSwap.add(false);
    triangleInverted.add(inverted);
    triangleDegenerate.add(false);
    triangleVpCached.add(false);
    triangleVpNormDotOrigin.add(0.0);
}

int
ProjectedTriangleTraversal::appendRaw(const Vector3Dd& p0, const Vector3Dd& p1,
    const Vector3Dd& p2, const Vector3Dd& normal)
{
    const int index = geometry->getNumTriangles();
    const int first = geometry->getNumVertices();
    if (geometry->getTriangleNormalsView().size() < (long)index*3) geometry->calculateNormals();
    java::ArrayList<double>& vertices = geometry->getVertexPositions();
    vertices.add(p0.x()); vertices.add(p0.y()); vertices.add(p0.z());
    vertices.add(p1.x()); vertices.add(p1.y()); vertices.add(p1.z());
    vertices.add(p2.x()); vertices.add(p2.y()); vertices.add(p2.z());
    java::ArrayList<int>& indices = geometry->getTriangleIndexes();
    indices.add(first); indices.add(first+1); indices.add(first+2);
    java::ArrayList<double>& normals = geometry->getTriangleNormals();
    normals.add(normal.x()); normals.add(normal.y()); normals.add(normal.z());
    return index;
}

void
ProjectedTriangleTraversal::computeTriangle(int index, bool initializeNativeNormal)
{
    Vector3Dd p1, p2, p3;
    getTrianglePoints(index, p1, p2, p3);

    Vector3Dd normal;
    double distance;
    if (!Triangle::supportingPlane(p1, p2, p3, 1.0e-9, normal, distance)) {
        triangleDegenerate[index] = true;
        return;
    }
    triangleNormals[index] = normal;
    if (initializeNativeNormal) {
        java::ArrayList<double>& normals = geometry->getTriangleNormals();
        normals[index*3] = -normal.x(); normals[index*3+1] = -normal.y(); normals[index*3+2] = -normal.z();
    }
    triangleDistance[index] = distance;
    const Triangle::ProjectionAxis axis = Triangle::dominantProjection(normal);
    triangleDominantAxis[index] = static_cast<int>(axis);
    triangleProjectionSwap[index] = Triangle::requiresProjectionSwap(p1, p2, p3, axis);
}

int
ProjectedTriangleTraversal::addTriangle(
    const Vector3Dd &p1, const Vector3Dd &p2, const Vector3Dd &p3, bool inverted)
{
    const int index = appendRaw(p1, p2, p3, Vector3Dd());
    initializeMetadata(inverted);
    computeTriangle(index, true);
    return index;
}

int
ProjectedTriangleTraversal::appendFrom(const ProjectedTriangleTraversal &source, int sourceIndex)
{
    Vector3Dd p0, p1, p2;
    source.getTrianglePoints(sourceIndex, p0, p1, p2);
    const int index = appendRaw(p0, p1, p2, source.getNativeNormal(sourceIndex));
    triangleDistance.add(source.triangleDistance.get(sourceIndex));
    triangleNormals.add(source.triangleNormals.get(sourceIndex));
    triangleDominantAxis.add(source.triangleDominantAxis.get(sourceIndex));
    triangleProjectionSwap.add(source.triangleProjectionSwap.get(sourceIndex));
    triangleInverted.add(source.triangleInverted.get(sourceIndex));
    triangleDegenerate.add(source.triangleDegenerate.get(sourceIndex));
    // Fresh cache: the plane-distance memoization is tied to whichever ray
    // last hit `source`, not meaningful for a newly-appended copy.
    triangleVpCached.add(false);
    triangleVpNormDotOrigin.add(0.0);
    return index;
}

bool
ProjectedTriangleTraversal::intersectTriangle(RayWithTracingState *ray, int index, double *depth)
{
    const Vector3Dd normal = getTriangleNormal(index);
    double normalDotOrigin;
    double normalDotDirection;
    GeometryStatistics &stats = *ray->getGeometryStatistics();

    stats.incrementRayTriangleTests();
    if (triangleDegenerate[index]) {
        return false;
    }

    const double distance = triangleDistance[index];

    if (ray->isPrimaryRayEnabled()) {
        if (!triangleVpCached[index]) {
            triangleVpNormDotOrigin[index] = normal.dotProduct(ray->getOrigin());
            triangleVpNormDotOrigin[index] += distance;
            triangleVpNormDotOrigin[index] *= -1.0;
            triangleVpCached[index] = true;
        }

        normalDotDirection = normal.dotProduct(ray->getDirection());
        if (InfinitePlane::intersectionDepth(triangleVpNormDotOrigin[index], normalDotDirection,
                GeometryConfig::SMALL_TOLERANCE, GeometryConfig::SMALL_TOLERANCE,
                GeometryConfig::MAX_DISTANCE, false, *depth) != RayFirstHitStatus::Hit) {
            return false;
        }
    } else {
        normalDotOrigin = normal.dotProduct(ray->getOrigin());
        normalDotOrigin += distance;
        normalDotOrigin *= -1.0;

        normalDotDirection = normal.dotProduct(ray->getDirection());
        if (InfinitePlane::intersectionDepth(normalDotOrigin, normalDotDirection,
                GeometryConfig::SMALL_TOLERANCE, GeometryConfig::SMALL_TOLERANCE,
                GeometryConfig::MAX_DISTANCE, false, *depth) != RayFirstHitStatus::Hit) {
            return false;
        }
    }

    Vector3Dd p1, p2, p3;
    getTrianglePoints(index, p1, p2, p3);
    if (triangleProjectionSwap[index]) { const Vector3Dd temporary = p1; p1 = p2; p2 = temporary; }
    const Vector3Dd point = ray->getOrigin().add(ray->getDirection().multiply(*depth));
    const bool inside = Triangle::containsProjected(p1, p2, p3, point,
        static_cast<Triangle::ProjectionAxis>(triangleDominantAxis[index]));
    if (inside == triangleInverted[index]) return false;
    stats.incrementRayTriangleTestsSucceeded();
    return true;
}

int ProjectedTriangleTraversal::emit(NativeGeometryBinding* owner, RayWithTracingState* ray,
    java::PriorityQueue<IntersectionCandidate>* queue, const GeometryIntersectionEmissionContext& context)
{
    int found = 0;
    for (int i = 0; i < getTriangleCount(); ++i) {
        double depth;
        if (!intersectTriangle(ray, i, &depth)) continue;
        IntersectionCandidate candidate;
        candidate.getIntersection().t = depth;
        candidate.getIntersection().point = ray->getDirection().multiply(depth).add(ray->getOrigin());
        candidate.getIntersection().normal = getTriangleNormal(i);
        candidate.getAttributes().setSurfaceIndex(i);
        candidate.getAttributes().setHitGeometry(owner);
        candidate.getAttributes().setMaterial(context.getMaterialOverride());
        candidate.getAttributes().pushDetailOwner(context.getDetailOwner());
        candidate.getAttributes().setMaterialUsesObjectLocalPoint(context.getMaterialUsesObjectLocalPoint());
        queue->offer(candidate);
        found = 1;
    }
    return found;
}

void ProjectedTriangleTraversal::invert()
{
    for (long int i = 0; i < triangleInverted.size(); ++i) triangleInverted[i] = !triangleInverted[i];
}

void ProjectedTriangleTraversal::doExtraInformation(RayCastingHit* hit) const
{
    if (!hit->needsNormal()) return;
    if (hit->n.length() > 1.0e-12) return;
    hit->n = Vector3Dd(0.0, 1.0, 0.0);
}
