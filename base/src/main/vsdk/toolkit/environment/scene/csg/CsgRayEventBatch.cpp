#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventBatch.h"
CsgRayEventBatch::CsgRayEventBatch() : groups(4) {}
CsgRayEventBatch::CsgRayEventBatch(const CsgRayEventBatch& other) : groups(other.groups) {}
CsgRayEventBatch& CsgRayEventBatch::operator=(const CsgRayEventBatch& other)
{ if (this!=&other) groups=other.groups; return *this; }
CsgRayEventBatch::~CsgRayEventBatch() {}
void CsgRayEventBatch::clear() { groups.clear(); }
bool CsgRayEventBatch::add(const CsgRayEventGroup& group) { return groups.add(group); }
long int CsgRayEventBatch::size() const { return groups.size(); }
const CsgRayEventGroup* CsgRayEventBatch::get(long int index) const
{ return index>=0 && index<groups.size() ? &groups[index] : nullptr; }
