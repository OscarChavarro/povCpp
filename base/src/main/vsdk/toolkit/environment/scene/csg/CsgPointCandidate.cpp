#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgPointCandidate.h"
CsgPointCandidate::CsgPointCandidate() : result(GeometryContainment::InvalidInput) {}
CsgPointCandidate::CsgPointCandidate(const CsgPointCandidate& other)
    : result(other.result), witness(other.witness), proof(other.proof), attribution(other.attribution) {}
CsgPointCandidate& CsgPointCandidate::operator=(const CsgPointCandidate& other)
{ if (this!=&other) { result=other.result; witness=other.witness; proof=other.proof; attribution=other.attribution; } return *this; }
CsgPointCandidate::~CsgPointCandidate() {}
