#ifndef __TRIANGLE_MESH__
#define __TRIANGLE_MESH__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"

class TriangleMesh : public Geometry {
  public:
    TriangleMesh();
    TriangleMesh(const TriangleMesh &other);

    int addTriangle(const Vector3Dd &p1, const Vector3Dd &p2, const Vector3Dd &p3,
        bool inverted = false);
    int appendFrom(const TriangleMesh &source, int sourceIndex);
    int getTriangleCount() const;
    bool isDegenerate(int index) const;
    AxisAlignedBoundingBox getMinMax() const override;
    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doIntersectionForAllRayCrossingsAnnotated(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) override;
    bool hasNativeAnnotatedCrossings() const override { return true; }
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void doExtraInformation(const RayWithTracingState &ray, double t, PovRayHit *hit) override;
    void *copy() override;
    void invertGeometry() override;

  private:
    java::ArrayList<Vector3Dd> vertices;
    java::ArrayList<Triangle> triangles;

    // Per-triangle intersection-support data, parallel to `triangles` (same
    // index). Computing intersections is a Geometry (i.e. TriangleMesh)
    // responsibility, not Triangle's, so all of this - including the
    // per-primary-ray plane-distance cache - lives here instead of on
    // Triangle.
    java::ArrayList<double> triangleDistance;
    java::ArrayList<int> triangleDominantAxis;
    java::ArrayList<bool> triangleInverted;
    java::ArrayList<bool> triangleDegenerate;
    java::ArrayList<bool> triangleVpCached;
    java::ArrayList<double> triangleVpNormDotOrigin;

    static int max3Axis(double x, double y, double z);
    void computeTriangle(int index);
    bool intersectTriangle(RayWithTracingState *ray, int index, double *depth);
};

#endif
