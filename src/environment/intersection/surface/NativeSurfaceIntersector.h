#ifndef __NATIVE_SURFACE_INTERSECTOR__
#define __NATIVE_SURFACE_INTERSECTOR__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "environment/intersection/SurfaceIntersectionState.h"
#include "environment/intersection/ray/RayWithTracingState.h"
#include "environment/intersection/hit/IntersectionCandidate.h"
#include "environment/intersection/hit/GeometryIntersectionEmissionContext.h"

// POV render trace of VITRAL spheres and of any other VITRAL geometry through
// its own crossing and first-hit queries (the generic family). Crossings keep
// the geometry's provenance; containment honors the inversion flag.
class NativeSurfaceIntersector {
  public:
    static int emitSphereCrossings(Sphere &sphere, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context);
    static bool sphereFirstHit(Sphere &sphere, RayWithTracingState *ray,
        IntersectionCandidate &out, Material *materialOverride);
    static int sphereContainment(const Sphere &sphere, const SurfaceIntersectionState &state,
        const Vector3Dd &point, double tolerance);
    static int emitGenericCrossings(Geometry &geometry, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context);
    static bool genericFirstHit(Geometry &geometry, RayWithTracingState *ray,
        IntersectionCandidate &out, Material *materialOverride);
    static int genericContainment(const Geometry &geometry, const SurfaceIntersectionState &state,
        const Vector3Dd &point, double tolerance);

  private:
    static bool intersectSphere(const Sphere &sphere, const RayWithTracingState *ray,
        double *depth1, double *depth2);
};

#endif
