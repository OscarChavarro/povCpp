#ifndef __POV_PARAMETRIC_BI_CUBIC_PATCH__
#define __POV_PARAMETRIC_BI_CUBIC_PATCH__

#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"
#include "environment/geometry/RayCastingGeometry.h"
#include "environment/geometry/surface/parametric/PatchScratch.h"

// PovRay bicubic_patch over a VITRAL SubdividedBezierPatch. PovRay shares a
// budget of MAX_BICUBIC_INTERSECTIONS hits per patch and thread between the
// rays traced since the last primary ray, and keeps their points and normals
// for the later normal query; that render state lives here.
class RayCastingParametricBiCubicPatch : public RayCastingGeometry {
  public:
    static constexpr int MAX_BICUBIC_INTERSECTIONS = SubdividedBezierPatch::MAXIMUM_HITS;

    // patchType is the PovRay type 0 to 4 (see BezierPatchSubdivision).
    RayCastingParametricBiCubicPatch(int patchType, int uSteps, int vSteps,
        double flatnessValue, const Vector3Dd (&controlPoints)[4][4]);
    RayCastingParametricBiCubicPatch(const RayCastingParametricBiCubicPatch &other);
    ~RayCastingParametricBiCubicPatch() override;

    const SubdividedBezierPatch &getPatch() const { return patch; }

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
    void doExtraInformation(const RayWithTracingState &ray, double t, RayCastingHit *hit) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void invertGeometry() override;

  protected:
    void computeSurfaceNormal(Vector3Dd *result, Vector3Dd *localIntersectionPoint) override;
    void computeSurfaceNormal(
        Vector3Dd *result,
        Vector3Dd *localIntersectionPoint,
        const RendererConfiguration *config) override;

  private:
    SubdividedBezierPatch patch;

    static PatchScratch &patchScratchFor(const RayCastingParametricBiCubicPatch *patch);
    static BezierPatchSubdivision subdivisionOf(int patchType);
    int traceCrossings(RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        const GeometryIntersectionEmissionContext *context);
};

#endif
