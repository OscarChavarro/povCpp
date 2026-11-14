#include "java/util/ArrayList.txx"
#include "java/lang/Math.h"
#include "java/lang/IllegalStateException.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/scene/ProjectedTriangleTraversal.h"
#include "environment/scene/SurfaceBoundingVolume.h"

BoundingVolumeHierarchy *
SurfaceBoundingVolume::create(const Geometry *surface, SurfaceDetailFamily family,
    const SurfaceTraceState &state)
{
    switch (family) {
    case SurfaceDetailFamily::Box:
    {
        const Box *box = static_cast<const Box *>(surface);
        return new AabbBoundingVolume(box->getRawBounds()[0], box->getRawBounds()[1]);
    }
    case SurfaceDetailFamily::Blob:
    {
        const BlobSolid *solid = static_cast<const BlobSolid *>(surface);
        if (solid->getComponentCount() == 0) {
            return new AabbBoundingVolume(AxisAlignedBoundingBox::unbounded());
        }
        // Local-space AABB from all component centers +/- radius
        AxisAlignedBoundingBox local = AxisAlignedBoundingBox::empty();
        for (long int i = 0; i < solid->getComponentCount(); i++) {
            const double r = java::Math::sqrt(solid->getComponent(i).getRadiusSquared());
            const Vector3Dd &c = solid->getComponent(i).getCenter();
            local = local.expandedBy(Vector3Dd(c.x() - r, c.y() - r, c.z() - r));
            local = local.expandedBy(Vector3Dd(c.x() + r, c.y() + r, c.z() + r));
        }
        return new AabbBoundingVolume(
            AxisAlignedBoundingBox::fromTransformedCorners(local.getMin(), local.getMax(), nullptr));
    }
    case SurfaceDetailFamily::HeightField:
    {
        const HeightFieldSolid *solid = static_cast<const HeightFieldSolid *>(surface);
        return new AabbBoundingVolume(AxisAlignedBoundingBox::fromTransformedCorners(
            solid->getMinimum(), solid->getMaximum(), nullptr));
    }
    case SurfaceDetailFamily::Mesh:
        if (state.getTriangleTraversal() != nullptr &&
            state.getTriangleTraversal()->hasUnboundedFaces()) {
            return new AabbBoundingVolume(AxisAlignedBoundingBox::unbounded());
        }
        [[fallthrough]];
    case SurfaceDetailFamily::Quadric:
    case SurfaceDetailFamily::Sphere:
    case SurfaceDetailFamily::Plane:
    case SurfaceDetailFamily::Polynomial:
    case SurfaceDetailFamily::BicubicPatch:
    case SurfaceDetailFamily::Generic:
    {
        GeometryBounds bounds;
        surface->getBounds(bounds);
        return new AabbBoundingVolume(AxisAlignedBoundingBox::fromGeometryBounds(bounds));
    }
    case SurfaceDetailFamily::Empty:
        return new AabbBoundingVolume(AxisAlignedBoundingBox::unbounded());
    case SurfaceDetailFamily::None:
        break;
    }
    throw java::IllegalStateException("Surface has no bounding volume kernel");
}
