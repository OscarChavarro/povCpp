#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWitness.h"
CsgPointWitness::CsgPointWitness() : path(4), reversed(false) {}
CsgPointWitness::CsgPointWitness(const CsgPointWitness& other) : path(other.path), reversed(other.reversed) {}
CsgPointWitness& CsgPointWitness::operator=(const CsgPointWitness& other)
{
    // Reuses the existing storage, see RayInstancePath.
    if (this!=&other) {
        path.clear();
        for (long int i=0;i<other.path.size();++i) path.add(other.path[i]);
        reversed=other.reversed;
    }
    return *this;
}
CsgPointWitness::~CsgPointWitness() {}
void CsgPointWitness::clear() { path.clear(); reversed=false; }
bool CsgPointWitness::prepend(const CsgOperandView* operand)
{
    if (!operand || !path.add(operand)) return false;
    for (long int i=path.size()-1;i>0;--i) path[i]=path[i-1];
    path[0]=operand;
    return true;
}
void CsgPointWitness::reverseOrientation() { reversed=!reversed; }
bool CsgPointWitness::isAvailable() const { return path.size()>0; }
bool CsgPointWitness::isReversed() const { return reversed; }
long int CsgPointWitness::getLength() const { return path.size(); }
const CsgOperandView* CsgPointWitness::getOperand(long int index) const
{ return index>=0 && index<path.size() ? path[index] : nullptr; }
