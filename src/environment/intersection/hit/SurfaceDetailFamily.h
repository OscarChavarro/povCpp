#ifndef __SURFACE_DETAIL_FAMILY__
#define __SURFACE_DETAIL_FAMILY__

// Geometry family of a VITRAL surface in the POV render: it selects the trace
// intersector (SurfaceIntersector) and the detail resolver (SurfaceDetailResolver).
// None marks a geometry without a family intersector (a POV CSG, traced by its own
// evaluator); Empty is the light geometry, which has no crossings and no
// interior.
enum class SurfaceDetailFamily : unsigned char {
    None,
    Quadric,
    Box,
    Plane,
    Sphere,
    Mesh,
    Polynomial,
    Blob,
    HeightField,
    BicubicPatch,
    Generic,
    Empty
};

#endif
