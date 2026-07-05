#ifndef __POV_RAY_HIT__
#define __POV_RAY_HIT__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/material/Material.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/DetailMask.h"
#include "environment/geometry/element/RayOperationOwner.h"
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
    static constexpr int MAX_DETAIL_OWNERS = 8;

    // Same bit values as RayWithSegments::DETAIL_* (see DetailMask.h): the
    // hit is the mask's authority at doExtraInformation time (VITRAL style -
    // see Sphere::doExtraInformation in VITRAL), even though the ray remains
    // the mask's pre-hit transport in povCpp.
    static constexpr int DETAIL_NONE = DetailMask::NONE;
    static constexpr int DETAIL_POINT = DetailMask::POINT;
    static constexpr int DETAIL_NORMAL = DetailMask::NORMAL;
    static constexpr int DETAIL_UV = DetailMask::UV;
    static constexpr int DETAIL_TANGENT = DetailMask::TANGENT;
    static constexpr int DETAIL_ALL = DetailMask::ALL;

    // --- core, same names as vsdk RayHit ---
    Vector3Dd p;
    Vector3Dd n;
    Vector3Dd t;          // surface tangent; povCpp does not compute this yet
    double u = 0.0;        // texture coordinates; povCpp computes these in the
    double v = 0.0;        // texture pipeline, not stored on the candidate
    double hitDistance = 0.0;
    int requiredDetailMask = DETAIL_NONE;

    // --- POV-specific extensions (no equivalent in vsdk RayHit) ---
    Geometry *hitGeometry = nullptr;
    RayOperationOwner *hitBody = nullptr;
    RayOperationOwner *detailOwners[MAX_DETAIL_OWNERS] = {};
    int detailOwnerCount = 0;
    Material *material = nullptr;
    Material *objectTexture = nullptr;
    ColorRgba *objectColor = nullptr;
    bool noShadowFlag = false;
    bool materialUsesObjectLocalPoint = false;

    static PovRayHit fromCandidate(const IntersectionCandidate &candidate, int requiredDetailMask);
    RayOperationOwner *popDetailOwner();
    RayOperationOwner *popDetailOwnerBack();
    bool needsPoint() const { return (requiredDetailMask & DETAIL_POINT) != 0; }
    bool needsNormal() const { return (requiredDetailMask & DETAIL_NORMAL) != 0; }
    bool needsUv() const { return (requiredDetailMask & DETAIL_UV) != 0; }
    bool needsTangent() const { return (requiredDetailMask & DETAIL_TANGENT) != 0; }
};

inline PovRayHit
PovRayHit::fromCandidate(const IntersectionCandidate &candidate, int requiredDetailMask)
{
    PovRayHit hit;
    const Intersection &intersection = candidate.getIntersection();
    const IntersectionAttributes &attributes = candidate.getAttributes();

    hit.p = intersection.point;
    hit.n = intersection.normal;
    hit.hitDistance = intersection.t;
    hit.requiredDetailMask = requiredDetailMask;

    hit.hitGeometry = attributes.getHitGeometry();
    hit.hitBody = attributes.getHitBody();
    hit.detailOwnerCount = attributes.getDetailOwnerCount();
    for (int i = 0; i < hit.detailOwnerCount; i++) {
        hit.detailOwners[i] = attributes.getDetailOwnerAt(i);
    }
    hit.material = attributes.getMaterial();
    hit.objectTexture = attributes.getObjectTexture();
    hit.objectColor = attributes.getObjectColor();
    hit.noShadowFlag = attributes.getNoShadowFlag();
    hit.materialUsesObjectLocalPoint = attributes.getMaterialUsesObjectLocalPoint();

    return hit;
}

inline RayOperationOwner *
PovRayHit::popDetailOwner()
{
    if (detailOwnerCount <= 0) {
        return nullptr;
    }
    RayOperationOwner *owner = detailOwners[0];
    for (int i = 1; i < detailOwnerCount; i++) {
        detailOwners[i - 1] = detailOwners[i];
    }
    detailOwners[--detailOwnerCount] = nullptr;
    return owner;
}

inline RayOperationOwner *
PovRayHit::popDetailOwnerBack()
{
    if (detailOwnerCount <= 0) {
        return nullptr;
    }
    RayOperationOwner *owner = detailOwners[--detailOwnerCount];
    detailOwners[detailOwnerCount] = nullptr;
    return owner;
}

#endif
