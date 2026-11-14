#ifndef __POV_PARAMETRIC_BI_CUBIC_PATCH__
#define __POV_PARAMETRIC_BI_CUBIC_PATCH__

#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"
#include "environment/geometry/RayCastingGeometry.h"
#include "environment/geometry/surface/parametric/PatchScratch.h"

// PovRay bicubic_patch over a VITRAL SubdividedBezierPatch; the render trace
// and its per-thread hit cache are BicubicPatchTrace and PatchScratch.
class RayCastingParametricBiCubicPatch : public RayCastingGeometry {
  public:
    // patchType is the PovRay type 0 to 4 (see BezierPatchSubdivision).
    RayCastingParametricBiCubicPatch(int patchType, int uSteps, int vSteps,
        double flatnessValue, const Vector3Dd (&controlPoints)[4][4]);
    RayCastingParametricBiCubicPatch(const RayCastingParametricBiCubicPatch &other);
    ~RayCastingParametricBiCubicPatch() override;

    const SubdividedBezierPatch &getPatch() const { return patch; }
    const Geometry *getIntrinsicGeometry() const override { return &patch; }
    Geometry *getDetailSurface() override { return &patch; }
    SurfaceDetailFamily getDetailFamily() const override { return SurfaceDetailFamily::BicubicPatch; }
    SurfaceTraceState *getTraceState() override { return &traceState; }

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

  private:
    SubdividedBezierPatch patch;
    SurfaceTraceState traceState;

    static BezierPatchSubdivision subdivisionOf(int patchType);
};

#endif
