#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgRaySurfaceEvents.h"
CsgRaySurfaceEvents::CsgRaySurfaceEvents() : events(8) {}
CsgRaySurfaceEvents::CsgRaySurfaceEvents(const CsgRaySurfaceEvents& other)
    : leaves(other.leaves), events(other.events), versions(other.versions) {}
CsgRaySurfaceEvents& CsgRaySurfaceEvents::operator=(const CsgRaySurfaceEvents& other)
{
    if (this!=&other) {
        leaves=other.leaves; versions=other.versions; events.clear();
        for (long int i=0;i<other.events.size();++i) events.add(other.events[i]);
    }
    return *this;
}
CsgRaySurfaceEvents::~CsgRaySurfaceEvents() {}
void CsgRaySurfaceEvents::clear() { leaves.clear(); events.clear(); versions.clear(); }
CsgVersionStamp& CsgRaySurfaceEvents::getVersions() { return versions; }
const CsgVersionStamp& CsgRaySurfaceEvents::getVersions() const { return versions; }
CsgRayLeafBatch& CsgRaySurfaceEvents::getLeaves() { return leaves; }
const CsgRayLeafBatch& CsgRaySurfaceEvents::getLeaves() const { return leaves; }
long int CsgRaySurfaceEvents::size() const { return events.size(); }
const CsgRayEventReference* CsgRaySurfaceEvents::get(long int index) const
{ return index>=0 && index<events.size() ? &events[index] : nullptr; }
double CsgRaySurfaceEvents::getT(long int index) const
{
    const CsgRayEventReference* reference=get(index);
    if (!reference) return java::Double::NaN;
    const CsgRayLeaf* leaf=leaves.get(reference->leafIndex);
    if (!leaf || reference->boundaryIndex<0 ||
        reference->boundaryIndex>=leaf->getClassification().boundaries.size()) return java::Double::NaN;
    return leaf->getClassification().boundaries[reference->boundaryIndex].t;
}
bool CsgRaySurfaceEvents::add(const CsgRayEventReference& reference)
{
    const CsgRayLeaf* leaf=leaves.get(reference.leafIndex);
    if (!leaf || reference.boundaryIndex<0 ||
        reference.boundaryIndex>=leaf->getClassification().boundaries.size()) return false;
    const double t=leaf->getClassification().boundaries[reference.boundaryIndex].t;
    long int position=events.size();
    while (position>0) {
        const CsgRayEventReference& before=events[position-1];
        const double beforeT=getT(position-1);
        if (beforeT<t || (beforeT==t && (before.leafIndex<reference.leafIndex ||
            (before.leafIndex==reference.leafIndex && before.boundaryIndex<reference.boundaryIndex)))) break;
        --position;
    }
    if (!events.add(reference)) return false;
    for (long int i=events.size()-1;i>position;--i) events.set(i,events[i-1]);
    events.set(position,reference);
    return true;
}
