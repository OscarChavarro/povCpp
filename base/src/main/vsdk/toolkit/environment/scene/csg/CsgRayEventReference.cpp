#include "vsdk/toolkit/environment/scene/csg/CsgRayEventReference.h"
CsgRayEventReference::CsgRayEventReference() : leafIndex(-1), boundaryIndex(-1) {}
CsgRayEventReference::CsgRayEventReference(long int leaf, long int boundary) : leafIndex(leaf), boundaryIndex(boundary) {}
