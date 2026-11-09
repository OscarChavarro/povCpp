#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/FunctionalExplicitSurface.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshGroup.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/SurfaceRayIntersection.h"
static bool rayIntersectsGeometryBounds(Geometry* geometry, const Ray& inRay)
{
    double* mm = geometry->getMinMax();
    Vector3Dd size(mm[3]-mm[0], mm[4]-mm[1], mm[5]-mm[2]);
    Vector3Dd center(
        (mm[3]+mm[0])/2,
        (mm[4]+mm[1])/2,
        (mm[5]+mm[2])/2
    );
    delete [] mm;

    Box boundingVolume(size);
    Ray localRay(
        inRay.getOrigin().subtract(center),
        inRay.getDirection(),
        inRay.getT()
    );
    return boundingVolume.doIntersectionFirstHit(localRay, 0);
}

bool SurfaceRayIntersection::doIntersectionFirstHit(Geometry* geometry, const Ray& inRay, RayHit* outHit)
{
    if ( geometry == 0 ) {
        return false;
    }

    if ( dynamic_cast<TriangleMesh*>(geometry) != 0 ||
         dynamic_cast<TriangleMeshGroup*>(geometry) != 0 ||
         dynamic_cast<FunctionalExplicitSurface*>(geometry) != 0 ) {
        if ( !rayIntersectsGeometryBounds(geometry, inRay) ) {
            return false;
        }
    }

    return geometry->doIntersectionFirstHit(inRay, outHit);
}
