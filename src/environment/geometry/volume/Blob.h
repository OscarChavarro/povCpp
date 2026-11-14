#ifndef __BLOB__
#define __BLOB__

#include "environment/geometry/RayCastingGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"

// PovRay adapter of the VITRAL BlobSolid: it keeps the PovRay candidate
// emission, shadow-ray root threshold, influence start, containment margin
// and inversion flag.
class Blob : public RayCastingGeometry {
  private:
    BlobSolid solid;
    SurfaceTraceState traceState;

  public:
    explicit Blob(const BlobSolid &solidValue);
    Blob(const Blob &other);
    ~Blob() override;

    int getSturmFlag() const { return solid.usesSturmSequences() ? 1 : 0; }
    const BlobSolid &getSolid() const { return solid; }
    bool isInverted() const { return traceState.isInverted(); }
    // The VITRAL solid for typed queries; inversion is reported separately.
    const Geometry *getIntrinsicGeometry() const override { return &solid; }
    Geometry *getDetailSurface() override { return &solid; }
    SurfaceDetailFamily getDetailFamily() const override { return SurfaceDetailFamily::Blob; }
    SurfaceTraceState *getTraceState() override { return &traceState; }
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
};

#endif
