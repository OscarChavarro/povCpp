#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgPointAttribution.h"
CsgPointAttribution::CsgPointAttribution() : owners(4), complete(false) {}
CsgPointAttribution::CsgPointAttribution(const CsgPointAttribution& other)
    : owners(other.owners), complete(other.complete), versions(other.versions) {}
CsgPointAttribution& CsgPointAttribution::operator=(const CsgPointAttribution& other)
{ if (this!=&other) { owners=other.owners; complete=other.complete; versions=other.versions; } return *this; }
CsgVersionStamp& CsgPointAttribution::getVersions() { return versions; }
const CsgVersionStamp& CsgPointAttribution::getVersions() const { return versions; }
CsgPointAttribution::~CsgPointAttribution() {}
void CsgPointAttribution::clear() { owners.clear(); complete=false; versions.clear(); }
void CsgPointAttribution::begin() { owners.clear(); complete=true; versions.clear(); }
bool CsgPointAttribution::add(const CsgPointWitness& owner)
{ if (!complete || !owner.isAvailable() || !owners.add(owner)) { clear(); return false; } return true; }
bool CsgPointAttribution::append(const CsgPointAttribution& other)
{
    if (!complete || !other.isComplete()) { clear(); return false; }
    for (long int i=0;i<other.owners.size();++i) if (!owners.add(other.owners[i])) { clear(); return false; }
    return true;
}
bool CsgPointAttribution::prepend(const CsgOperandView* operand)
{
    if (!complete) return false;
    for (long int i=0;i<owners.size();++i) if (!owners[i].prepend(operand)) { clear(); return false; }
    return true;
}
void CsgPointAttribution::reverseOrientation()
{ if (complete) for (long int i=0;i<owners.size();++i) owners[i].reverseOrientation(); }
bool CsgPointAttribution::isComplete() const { return complete && owners.size()>0; }
long int CsgPointAttribution::getCount() const { return isComplete() ? owners.size() : 0; }
const CsgPointWitness* CsgPointAttribution::getOwner(long int index) const
{ return isComplete() && index>=0 && index<owners.size() ? &owners[index] : nullptr; }
