#ifndef VSDK_PROJECTED_TRIANGLE_TRAVERSAL_H
#define VSDK_PROJECTED_TRIANGLE_TRAVERSAL_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/RayCastingGeometry.h"

class TriangleMesh;

// POV traversal policy over a borrowed VITRAL mesh; owns no geometry.
// While this policy is attached, change the mesh only through this policy so
// that its per-face projection and viewpoint caches stay synchronized.
class ProjectedTriangleTraversal {
private:
    TriangleMesh* geometry;
    java::ArrayList<double> triangleDistance;
    java::ArrayList<Vector3Dd> triangleNormals;
    java::ArrayList<int> triangleDominantAxis;
    java::ArrayList<bool> triangleProjectionSwap;
    java::ArrayList<bool> triangleInverted;
    java::ArrayList<bool> triangleDegenerate;
    java::ArrayList<bool> triangleVpCached;
    java::ArrayList<double> triangleVpNormDotOrigin;
    ProjectedTriangleTraversal(const ProjectedTriangleTraversal&) = delete;
    ProjectedTriangleTraversal& operator=(const ProjectedTriangleTraversal&) = delete;

    int appendRaw(const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2,
        const Vector3Dd& normal);
    void getTrianglePoints(int index, Vector3Dd& p0, Vector3Dd& p1, Vector3Dd& p2) const;
    Vector3Dd getTriangleNormal(int index) const;
    Vector3Dd getNativeNormal(int index) const;
    void initializeMetadata(bool inverted);
    void computeTriangle(int index, bool initializeNativeNormal);
    bool intersectTriangle(RayWithTracingState* ray, int index, double* depth);
public:
    explicit ProjectedTriangleTraversal(TriangleMesh* mesh);
    ProjectedTriangleTraversal(const ProjectedTriangleTraversal& other, TriangleMesh* mesh);
    ~ProjectedTriangleTraversal();
    int addTriangle(const Vector3Dd& p0, const Vector3Dd& p1, const Vector3Dd& p2,
        bool inverted = false);
    int appendFrom(const ProjectedTriangleTraversal& source, int index);
    int getTriangleCount() const;
    bool isDegenerate(int index) const;
    bool hasUnboundedFaces() const;
    void invert();
    int emit(RayWithTracingState* ray,
        java::PriorityQueue<IntersectionCandidate>* queue,
        const GeometryIntersectionEmissionContext& context);
};
#endif
