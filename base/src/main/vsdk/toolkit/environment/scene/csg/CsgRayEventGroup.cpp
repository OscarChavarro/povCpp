#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventGroup.h"
CsgRayEventGroup::CsgRayEventGroup() : t(0), references(4) {}
CsgRayEventGroup::CsgRayEventGroup(double value) : t(value), references(4) {}
CsgRayEventGroup::CsgRayEventGroup(const CsgRayEventGroup& other) : t(other.t), references(other.references) {}
CsgRayEventGroup& CsgRayEventGroup::operator=(const CsgRayEventGroup& other)
{ if (this!=&other) { t=other.t; references=other.references; } return *this; }
CsgRayEventGroup::~CsgRayEventGroup() {}
double CsgRayEventGroup::getT() const { return t; }
bool CsgRayEventGroup::add(const CsgRayEventReference& reference) { return references.add(reference); }
long int CsgRayEventGroup::size() const { return references.size(); }
const CsgRayEventReference* CsgRayEventGroup::get(long int index) const
{ return index>=0 && index<references.size() ? &references[index] : nullptr; }
