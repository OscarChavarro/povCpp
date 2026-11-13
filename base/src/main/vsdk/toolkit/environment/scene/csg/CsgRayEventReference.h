#ifndef VSDK_CSG_RAY_EVENT_REFERENCE_H
#define VSDK_CSG_RAY_EVENT_REFERENCE_H
// Indexes into the input leaf batch and that leaf's boundary array.
class CsgRayEventReference {
public:
    long int leafIndex;
    long int boundaryIndex;
    CsgRayEventReference();
    CsgRayEventReference(long int leaf, long int boundary);
};
#endif
