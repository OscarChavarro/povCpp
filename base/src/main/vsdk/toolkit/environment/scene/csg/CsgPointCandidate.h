#ifndef VSDK_CSG_POINT_CANDIDATE_H
#define VSDK_CSG_POINT_CANDIDATE_H
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWitness.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointProof.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointAttribution.h"
// Working value for one operand at a single point and tolerance.
class CsgPointCandidate {
public:
    GeometryContainment result;
    CsgPointWitness witness;
    CsgPointProof proof;
    CsgPointAttribution attribution;
    CsgPointCandidate();
    CsgPointCandidate(const CsgPointCandidate& other);
    CsgPointCandidate& operator=(const CsgPointCandidate& other);
    ~CsgPointCandidate();
};
#endif
