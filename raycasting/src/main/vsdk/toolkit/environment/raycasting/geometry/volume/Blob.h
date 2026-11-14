#ifndef __BLOB__
#define __BLOB__

#include "vsdk/toolkit/environment/raycasting/geometry/RayCastingGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"

// PovRay adapter of the VITRAL BlobSolid: it keeps the PovRay candidate
// emission, shadow-ray root threshold, influence start, containment margin
// and inversion flag.
class Blob : public RayCastingGeometry {
  private:
    static constexpr double INSIDE_TOLERANCE = 1.0e-6;
    static constexpr double SHADOW_ROOT_MIN_DISTANCE = 0.05;
    static constexpr double INFLUENCE_START = 0.01;

    BlobSolid solid;
    bool inverted;

    int traceCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        const GeometryIntersectionEmissionContext *context);

  public:
    explicit Blob(const BlobSolid &solidValue);
    Blob(const Blob &other);
    ~Blob() override;

    int getSturmFlag() const { return solid.usesSturmSequences() ? 1 : 0; }
    const BlobSolid &getSolid() const { return solid; }
    bool isInverted() const { return inverted; }
    // The VITRAL solid for typed queries; inversion is reported separately.
    const Geometry *getIntrinsicGeometry() const override { return &solid; }
    // Typed ray classification of the VITRAL solid, complemented when inverted.
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    Blob *copyWithSturmFlag(int flag) const;

    BoundingVolumeHierarchy *createBoundingVolume() const override;

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
    void *copy() override;
    void invertGeometry() override;

  protected:
    void computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
};

#endif
