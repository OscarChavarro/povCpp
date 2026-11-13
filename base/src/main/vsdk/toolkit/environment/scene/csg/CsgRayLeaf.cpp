#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeaf.h"
CsgRayLeaf::CsgRayLeaf() : placements(4) {}
CsgRayLeaf::CsgRayLeaf(const CsgRayLeaf& other)
    : path(other.path), classification(other.classification), placements(other.placements) {}
CsgRayLeaf& CsgRayLeaf::operator=(const CsgRayLeaf& other)
{
    if (this!=&other) {
        path=other.path; classification=other.classification;
        placements.clear();
        for (long int i=0;i<other.placements.size();++i) placements.add(other.placements[i]);
    }
    return *this;
}
void CsgRayLeaf::reset() { path.clear(); classification.clear(); placements.clear(); }
CsgRayLeaf::~CsgRayLeaf() {}
CsgPointWitness& CsgRayLeaf::getPath() { return path; }
const CsgPointWitness& CsgRayLeaf::getPath() const { return path; }
RaySolidClassification& CsgRayLeaf::getClassification() { return classification; }
const RaySolidClassification& CsgRayLeaf::getClassification() const { return classification; }
bool CsgRayLeaf::prependOperand(const CsgOperandView* operand) { return path.prepend(operand); }
bool CsgRayLeaf::appendPlacement(const CsgOperandView* operand, const Matrix4x4d& forward, const Matrix4x4d& inverse)
{ return operand && placements.add(CsgRayPlacement(operand,forward,inverse)); }
long int CsgRayLeaf::getPlacementCount() const { return placements.size(); }
const CsgRayPlacement* CsgRayLeaf::getPlacement(long int index) const
{ return index>=0 && index<placements.size() ? &placements[index] : nullptr; }
void CsgRayLeaf::reverseOrientation()
{
    path.reverseOrientation();
    for (long int i=0;i<classification.boundaries.size();++i)
        classification.boundaries[i].normalReversed=!classification.boundaries[i].normalReversed;
}
