#ifndef VSDK_CSG_RAY_CLASSIFICATION_H
#define VSDK_CSG_RAY_CLASSIFICATION_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventReference.h"
#include "vsdk/toolkit/environment/scene/csg/CsgVersionStamp.h"
// Combined membership of a whole expression along one ray. Boundary i of the
// solid is copied from its first owner. The owners of boundary i are the leaf
// events whose transition alone produces that membership change or, when none
// does, every changed leaf oriented like that change; there are none only when
// neither exists. References index the leaf snapshots kept here. The version
// stamp records the borrowed operands as they were when the result was built.
// Storage is reused across queries.
class CsgRayClassification {
private:
    CsgRayLeafBatch leaves;
    RaySolidClassification solid;
    java::ArrayList<CsgRayEventReference> owners;
    java::ArrayList<long long> ownerStarts;
    CsgVersionStamp versions;
public:
    CsgRayClassification();
    CsgRayClassification(const CsgRayClassification& other);
    CsgRayClassification& operator=(const CsgRayClassification& other);
    ~CsgRayClassification();
    void clear();
    CsgRayLeafBatch& getLeaves();
    const CsgRayLeafBatch& getLeaves() const;
    RaySolidClassification& getSolid();
    const RaySolidClassification& getSolid() const;
    CsgVersionStamp& getVersions();
    const CsgVersionStamp& getVersions() const;
    // Starts the owner list of the next boundary.
    bool beginOwners();
    bool addOwner(const CsgRayEventReference& reference);
    long int getOwnerGroupCount() const;
    long int getOwnerCount(long int boundaryIndex) const;
    const CsgRayEventReference* getOwner(long int boundaryIndex, long int ownerIndex) const;
};
#endif
