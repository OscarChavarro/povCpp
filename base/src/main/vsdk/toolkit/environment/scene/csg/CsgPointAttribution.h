#ifndef VSDK_CSG_POINT_ATTRIBUTION_H
#define VSDK_CSG_POINT_ATTRIBUTION_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWitness.h"
class CsgOperandView;
// Complete set of certified boundary owners. Paths borrow scene identities;
// discard this snapshot if the scene changes.
class CsgPointAttribution {
private:
    java::ArrayList<CsgPointWitness> owners;
    bool complete;
public:
    CsgPointAttribution();
    CsgPointAttribution(const CsgPointAttribution& other);
    CsgPointAttribution& operator=(const CsgPointAttribution& other);
    ~CsgPointAttribution();
    void clear();
    void begin();
    bool add(const CsgPointWitness& owner);
    bool append(const CsgPointAttribution& other);
    bool prepend(const CsgOperandView* operand);
    void reverseOrientation();
    bool isComplete() const;
    long int getCount() const;
    const CsgPointWitness* getOwner(long int index) const;
};
#endif
