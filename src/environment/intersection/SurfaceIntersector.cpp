#include "java/lang/IllegalStateException.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "environment/intersection/SurfaceIntersector.h"
#include "environment/intersection/surface/NativeSurfaceIntersector.h"
#include "environment/intersection/ProjectedTriangleTraversal.h"
#include "environment/intersection/surface/QuadricIntersector.h"
#include "environment/intersection/surface/BoxIntersector.h"
#include "environment/intersection/surface/PlaneIntersector.h"
#include "environment/intersection/surface/PolynomialIntersector.h"
#include "environment/intersection/surface/BlobIntersector.h"
#include "environment/intersection/surface/HeightFieldIntersector.h"
#include "environment/intersection/surface/BicubicPatchIntersector.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/PolynomialSurface.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"

int
SurfaceIntersector::emitCrossings(Geometry *surface, SurfaceDetailFamily family,
    SurfaceIntersectionState &state, RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride, const GeometryIntersectionEmissionContext *context)
{
    switch (family) {
    case SurfaceDetailFamily::Quadric:
        return QuadricIntersector::emitCrossings(*static_cast<QuadricSurface *>(surface), state, ray,
            depthQueue, materialOverride, context);
    case SurfaceDetailFamily::Box:
        return BoxIntersector::emitCrossings(*static_cast<Box *>(surface), ray, depthQueue,
            materialOverride, context);
    case SurfaceDetailFamily::Plane:
        return PlaneIntersector::emitCrossings(*static_cast<PlanarHalfSpace *>(surface), state, ray,
            depthQueue, materialOverride, context);
    case SurfaceDetailFamily::Polynomial:
        return PolynomialIntersector::emitCrossings(*static_cast<PolynomialSurface *>(surface), ray,
            depthQueue, materialOverride, context);
    case SurfaceDetailFamily::Blob:
        return BlobIntersector::emitCrossings(*static_cast<BlobSolid *>(surface), ray, depthQueue,
            materialOverride, context);
    case SurfaceDetailFamily::HeightField:
        return HeightFieldIntersector::emitCrossings(*static_cast<HeightFieldSolid *>(surface), ray,
            depthQueue, materialOverride, context);
    case SurfaceDetailFamily::BicubicPatch:
        return BicubicPatchIntersector::emitCrossings(*static_cast<SubdividedBezierPatch *>(surface),
            ray, depthQueue, materialOverride, context);
    case SurfaceDetailFamily::Sphere:
    case SurfaceDetailFamily::Mesh:
    case SurfaceDetailFamily::Generic: {
        const GeometryIntersectionEmissionContext plain(materialOverride, nullptr, false);
        const GeometryIntersectionEmissionContext &effective = context != nullptr ? *context : plain;
        if (family == SurfaceDetailFamily::Mesh) {
            return state.getTriangleTraversal()->emit(ray, depthQueue, effective);
        }
        if (family == SurfaceDetailFamily::Sphere) {
            return NativeSurfaceIntersector::emitSphereCrossings(*static_cast<Sphere *>(surface), ray,
                depthQueue, effective);
        }
        return NativeSurfaceIntersector::emitGenericCrossings(*surface, ray, depthQueue, effective);
    }
    case SurfaceDetailFamily::Empty:
        return false;
    case SurfaceDetailFamily::None:
        break;
    }
    throw java::IllegalStateException("Surface has no family intersector");
}

bool
SurfaceIntersector::firstHit(Geometry *surface, SurfaceDetailFamily family,
    SurfaceIntersectionState &state, RayWithTracingState *ray, IntersectionCandidate &out,
    Material *materialOverride)
{
    switch (family) {
    case SurfaceDetailFamily::Quadric:
        return QuadricIntersector::firstHit(*static_cast<QuadricSurface *>(surface), state, ray, out,
            materialOverride);
    case SurfaceDetailFamily::Box:
        return BoxIntersector::firstHit(*static_cast<Box *>(surface), ray, out, materialOverride);
    case SurfaceDetailFamily::Plane:
        return PlaneIntersector::firstHit(*static_cast<PlanarHalfSpace *>(surface), state, ray, out,
            materialOverride);
    case SurfaceDetailFamily::Sphere:
        return NativeSurfaceIntersector::sphereFirstHit(*static_cast<Sphere *>(surface), ray, out,
            materialOverride);
    case SurfaceDetailFamily::Generic:
        return NativeSurfaceIntersector::genericFirstHit(*surface, ray, out, materialOverride);
    case SurfaceDetailFamily::Mesh:
    case SurfaceDetailFamily::Polynomial:
    case SurfaceDetailFamily::Blob:
    case SurfaceDetailFamily::HeightField:
    case SurfaceDetailFamily::BicubicPatch:
    case SurfaceDetailFamily::Empty:
        return false;
    case SurfaceDetailFamily::None:
        break;
    }
    throw java::IllegalStateException("Surface has no family intersector");
}

int
SurfaceIntersector::containment(Geometry *surface, SurfaceDetailFamily family,
    const SurfaceIntersectionState &state, const Vector3Dd &point, double distanceTolerance)
{
    switch (family) {
    case SurfaceDetailFamily::Quadric:
        return QuadricIntersector::containment(*static_cast<const QuadricSurface *>(surface), point,
            distanceTolerance);
    case SurfaceDetailFamily::Box:
        return BoxIntersector::containment(*static_cast<const Box *>(surface), state, point,
            distanceTolerance);
    case SurfaceDetailFamily::Plane:
        return PlaneIntersector::containment(*static_cast<const PlanarHalfSpace *>(surface), point,
            distanceTolerance);
    case SurfaceDetailFamily::Polynomial:
        return PolynomialIntersector::containment(*static_cast<const PolynomialSurface *>(surface),
            state, point, distanceTolerance);
    case SurfaceDetailFamily::Blob:
        return BlobIntersector::containment(*static_cast<const BlobSolid *>(surface), state, point,
            distanceTolerance);
    case SurfaceDetailFamily::HeightField:
        return HeightFieldIntersector::containment(*static_cast<const HeightFieldSolid *>(surface),
            point, distanceTolerance);
    case SurfaceDetailFamily::Sphere:
        return NativeSurfaceIntersector::sphereContainment(*static_cast<const Sphere *>(surface), state,
            point, distanceTolerance);
    case SurfaceDetailFamily::Generic:
        return NativeSurfaceIntersector::genericContainment(*surface, state, point, distanceTolerance);
    case SurfaceDetailFamily::Mesh:
    case SurfaceDetailFamily::BicubicPatch:
    case SurfaceDetailFamily::Empty:
        return static_cast<int>(Containment::OUTSIDE);
    case SurfaceDetailFamily::None:
        break;
    }
    throw java::IllegalStateException("Surface has no family intersector");
}

SurfaceDetailFamily
SurfaceIntersector::familyOf(const Geometry *surface)
{
    if (dynamic_cast<const QuadricSurface *>(surface) != nullptr) return SurfaceDetailFamily::Quadric;
    if (dynamic_cast<const PlanarHalfSpace *>(surface) != nullptr) return SurfaceDetailFamily::Plane;
    if (dynamic_cast<const Box *>(surface) != nullptr) return SurfaceDetailFamily::Box;
    if (dynamic_cast<const Sphere *>(surface) != nullptr) return SurfaceDetailFamily::Sphere;
    if (dynamic_cast<const PolynomialSurface *>(surface) != nullptr) return SurfaceDetailFamily::Polynomial;
    if (dynamic_cast<const BlobSolid *>(surface) != nullptr) return SurfaceDetailFamily::Blob;
    if (dynamic_cast<const HeightFieldSolid *>(surface) != nullptr) return SurfaceDetailFamily::HeightField;
    if (dynamic_cast<const SubdividedBezierPatch *>(surface) != nullptr) {
        return SurfaceDetailFamily::BicubicPatch;
    }
    return SurfaceDetailFamily::Generic;
}

void
SurfaceIntersector::initializeState(const Geometry *surface, SurfaceDetailFamily family,
    SurfaceIntersectionState &state)
{
    if (family == SurfaceDetailFamily::Quadric) {
        state.setNonZeroSquareTerm(
            QuadricIntersector::hasNonZeroSquareTerm(*static_cast<const QuadricSurface *>(surface)));
    }
}

void
SurfaceIntersector::invertGeometry(Geometry *surface, SurfaceDetailFamily family)
{
    if (family == SurfaceDetailFamily::Quadric) {
        QuadricIntersector::invert(*static_cast<QuadricSurface *>(surface));
    } else if (family == SurfaceDetailFamily::Plane) {
        PlaneIntersector::invert(*static_cast<PlanarHalfSpace *>(surface));
    } else if (invertsByState(family)) {
        throw java::IllegalStateException("This surface family is inverted by its body");
    }
}
