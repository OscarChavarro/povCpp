#ifndef __SURFACE_RAY_INTERSECTION__
#define __SURFACE_RAY_INTERSECTION__

class Geometry;
class Ray;
class RayHit;

class SurfaceRayIntersection {
public:
    static bool doIntersectionFirstHit(Geometry* geometry, const Ray& inRay, RayHit* outHit);
};

#endif
