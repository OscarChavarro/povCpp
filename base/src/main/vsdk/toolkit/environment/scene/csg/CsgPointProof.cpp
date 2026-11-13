#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgPointProof.h"
CsgPointProof::CsgPointProof() : evidence(4), available(false) {}
void CsgPointProof::clear() { evidence.clear(); available=false; }
void CsgPointProof::begin() { evidence.clear(); available=true; }
bool CsgPointProof::add(const CsgPointEvidence& value)
{ if (!available || !evidence.add(value)) { clear(); return false; } return true; }
bool CsgPointProof::append(const CsgPointProof& other)
{
    if (!available || !other.available) { clear(); return false; }
    for (long int i=0;i<other.evidence.size();++i) if (!evidence.add(other.evidence[i])) { clear(); return false; }
    return true;
}
bool CsgPointProof::prepend(const CsgOperandView* operand)
{
    if (!available) return false;
    for (long int i=0;i<evidence.size();++i)
        if (!evidence[i].prepend(operand)) { clear(); return false; }
    return true;
}
void CsgPointProof::reverseOrientation()
{ if (available) for (long int i=0;i<evidence.size();++i) evidence[i].reverseOrientation(); }
bool CsgPointProof::isAvailable() const { return available && evidence.size()>0; }
long int CsgPointProof::getCount() const { return isAvailable() ? evidence.size() : 0; }
const CsgPointEvidence* CsgPointProof::getEvidence(long int index) const
{ return isAvailable() && index>=0 && index<evidence.size() ? &evidence[index] : nullptr; }
