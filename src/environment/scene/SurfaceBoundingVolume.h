#ifndef __SURFACE_BOUNDING_VOLUME__
#define __SURFACE_BOUNDING_VOLUME__

#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "environment/geometry/SurfaceTraceState.h"
#include "environment/geometry/element/SurfaceDetailFamily.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BoundingVolumeHierarchy.h"

// Render culling volume of a traced VITRAL surface in its own frame: the
// VITRAL box of its bounds (AxisAlignedBoundingBox::fromGeometryBounds),
// except for PovRay's own rules where they differ: raw box corners, blob
// component spheres of every strength, the whole height field grid, meshes
// with unbounded faces, and bodies without a surface (lights). Inversion
// never changes it.
class SurfaceBoundingVolume {
  public:
    static BoundingVolumeHierarchy *create(const Geometry *surface, SurfaceDetailFamily family,
        const SurfaceTraceState &state);
};

#endif
