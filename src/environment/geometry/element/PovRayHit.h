#ifndef __POV_RAY_HIT__
#define __POV_RAY_HIT__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/material/Material.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/IntersectionCandidate.h"

// Read-only projection of IntersectionCandidate onto a record shaped like
// VITRAL's RayHit: the core fields below use exactly its names (p/n/t/u/v/
// hitDistance), so the two hit records can be diffed field by field. The
// remaining fields are povCpp/POV-Ray-specific extensions that VITRAL's
// RayHit does not have (see vitralNormalizationAnalysis.md, sec. 10).
//
// This does not replace Intersection/IntersectionAttributes as the storage
// format travelling through the depth queues; it is only built at the
// shading edge, from an already-resolved IntersectionCandidate.
class PovRayHit {
  public:
    // --- core, same names as vsdk RayHit ---
    Vector3Dd p;
    Vector3Dd n;
    Vector3Dd t;          // surface tangent; povCpp does not compute this yet
    double u = 0.0;        // texture coordinates; povCpp computes these in the
    double v = 0.0;        // texture pipeline, not stored on the candidate
    double hitDistance = 0.0;

    // --- POV-specific extensions (no equivalent in vsdk RayHit) ---
    Geometry *hitGeometry = nullptr;
    Material *material = nullptr;
    Material *objectTexture = nullptr;
    ColorRgba *objectColor = nullptr;
    bool noShadowFlag = false;

    static PovRayHit fromCandidate(const IntersectionCandidate &candidate);
};

inline PovRayHit
PovRayHit::fromCandidate(const IntersectionCandidate &candidate)
{
    PovRayHit hit;
    const Intersection &intersection = candidate.getIntersection();
    const IntersectionAttributes &attributes = candidate.getAttributes();

    hit.p = intersection.point;
    hit.n = intersection.normal;
    hit.hitDistance = intersection.t;

    hit.hitGeometry = attributes.getHitGeometry();
    hit.material = attributes.getMaterial();
    hit.objectTexture = attributes.getObjectTexture();
    hit.objectColor = attributes.getObjectColor();
    hit.noShadowFlag = attributes.getNoShadowFlag();

    return hit;
}

#endif
