#include "vsdk/toolkit/environment/scene/csg/CsgPointEvidence.h"
CsgPointEvidence::CsgPointEvidence() : equivalence(false), classification(GeometryContainment::Indeterminate), point(), tolerance(0) {}
CsgPointEvidence::CsgPointEvidence(const CsgPointWitness& p, GeometryContainment c,
    const Vector3Dd& q, double t) : path(p), equivalence(false), classification(c), point(q), tolerance(t) {}
CsgPointWitness& CsgPointEvidence::getPath() { return path; }
const CsgPointWitness& CsgPointEvidence::getPath() const { return path; }
GeometryContainment CsgPointEvidence::getClassification() const { return classification; }
const Vector3Dd& CsgPointEvidence::getPoint() const { return point; }
double CsgPointEvidence::getTolerance() const { return tolerance; }

CsgPointEvidence::CsgPointEvidence(const CsgPointWitness& source, const CsgPointWitness& equivalent,
    const Vector3Dd& q, double t) : path(source), equivalentPath(equivalent), equivalence(true),
    classification(GeometryContainment::Indeterminate), point(q), tolerance(t) {}
bool CsgPointEvidence::isEquivalence() const { return equivalence; }
const CsgPointWitness& CsgPointEvidence::getEquivalentPath() const { return equivalentPath; }
bool CsgPointEvidence::prepend(const CsgOperandView* operand)
{ return path.prepend(operand) && (!equivalence || equivalentPath.prepend(operand)); }
void CsgPointEvidence::reverseOrientation()
{ path.reverseOrientation(); if (equivalence) equivalentPath.reverseOrientation(); }
