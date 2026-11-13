#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafBatch.h"
CsgRayLeafBatch::CsgRayLeafBatch() : leaves(4) {}
CsgRayLeafBatch::CsgRayLeafBatch(const CsgRayLeafBatch& other) : leaves(other.leaves) {}
CsgRayLeafBatch& CsgRayLeafBatch::operator=(const CsgRayLeafBatch& other)
{ if (this!=&other) leaves=other.leaves; return *this; }
CsgRayLeafBatch::~CsgRayLeafBatch() {}
void CsgRayLeafBatch::clear() { leaves.clear(); }
bool CsgRayLeafBatch::add(const CsgRayLeaf& leaf) { return leaves.add(leaf); }
long int CsgRayLeafBatch::size() const { return leaves.size(); }
CsgRayLeaf* CsgRayLeafBatch::get(long int index) { return index>=0 && index<leaves.size() ? &leaves[index] : nullptr; }
const CsgRayLeaf* CsgRayLeafBatch::get(long int index) const { return index>=0 && index<leaves.size() ? &leaves[index] : nullptr; }
