#ifndef VSDK_CSG_RAY_LEAF_BATCH_H
#define VSDK_CSG_RAY_LEAF_BATCH_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeaf.h"
// Borrowed leaf identities; owns snapshots of the classifications and paths.
class CsgRayLeafBatch {
private:
    java::ArrayList<CsgRayLeaf> leaves;
public:
    CsgRayLeafBatch();
    CsgRayLeafBatch(const CsgRayLeafBatch& other);
    CsgRayLeafBatch& operator=(const CsgRayLeafBatch& other);
    ~CsgRayLeafBatch();
    void clear();
    bool add(const CsgRayLeaf& leaf);
    long int size() const;
    CsgRayLeaf* get(long int index);
    const CsgRayLeaf* get(long int index) const;
};
#endif
