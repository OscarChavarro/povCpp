#ifndef VSDK_CSG_POINT_PROOF_H
#define VSDK_CSG_POINT_PROOF_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointEvidence.h"
class CsgOperandView;
// Sufficient leaf facts for a definite result, interpreted with the borrowed
// expression tree. Empty/unavailable means the result has no collected proof.
class CsgPointProof {
private:
    java::ArrayList<CsgPointEvidence> evidence;
    bool available;
public:
    CsgPointProof();
    void clear();
    void begin();
    bool add(const CsgPointEvidence& value);
    bool append(const CsgPointProof& other);
    bool prepend(const CsgOperandView* operand);
    void reverseOrientation();
    bool isAvailable() const;
    long int getCount() const;
    const CsgPointEvidence* getEvidence(long int index) const;
};
#endif
