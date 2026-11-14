#ifndef __BAKED_SURFACE_INTERSECTION__
#define __BAKED_SURFACE_INTERSECTION__

#include "environment/scene/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/intersection/SurfaceIntersector.h"

// Trace of a bake record's surface: the family intersector chosen when the record
// was built, with the record's trace state. A POV CSG has no family intersector and
// is traced by its own evaluator.
class BakedSurfaceIntersection {
  public:
    static int emitCrossings(Geometry *surface, SurfaceDetailFamily family,
        SurfaceIntersectionState *state, ConstructiveSolidGeometry *csg, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue, Material *materialOverride)
    {
        if (family == SurfaceDetailFamily::None) {
            return csg->doIntersectionForAllRayCrossings(ray, depthQueue, materialOverride);
        }
        return SurfaceIntersector::emitCrossings(surface, family, *state, ray, depthQueue,
            materialOverride, nullptr);
    }

    static int emitCrossingsAnnotated(Geometry *surface, SurfaceDetailFamily family,
        SurfaceIntersectionState *state, ConstructiveSolidGeometry *csg, RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context)
    {
        if (family == SurfaceDetailFamily::None) {
            return csg->doIntersectionForAllRayCrossingsAnnotated(ray, depthQueue, context);
        }
        return SurfaceIntersector::emitCrossings(surface, family, *state, ray, depthQueue,
            context.getMaterialOverride(), &context);
    }

    static bool firstHit(Geometry *surface, SurfaceDetailFamily family,
        SurfaceIntersectionState *state, ConstructiveSolidGeometry *csg, RayWithTracingState *ray,
        IntersectionCandidate &out, Material *materialOverride)
    {
        if (family == SurfaceDetailFamily::None) {
            (void)csg;
            return false;
        }
        return SurfaceIntersector::firstHit(surface, family, *state, ray, out, materialOverride);
    }

    static int containment(Geometry *surface, SurfaceDetailFamily family,
        const SurfaceIntersectionState *state, ConstructiveSolidGeometry *csg, const Vector3Dd &point,
        double distanceTolerance)
    {
        if (family == SurfaceDetailFamily::None) {
            return csg->doContainmentTest(point, distanceTolerance);
        }
        return SurfaceIntersector::containment(surface, family, *state, point, distanceTolerance);
    }
};

#endif
