#ifndef VSDK_CSG_POINT_WITNESS_H
#define VSDK_CSG_POINT_WITNESS_H
#include "java/util/ArrayList.h"
class CsgOperandView;
// Borrowed root-to-leaf path for a single operand proving a point result.
// Reversal records difference orientation, not a certified boundary hit.
class CsgPointWitness {
private:
    java::ArrayList<const CsgOperandView*> path;
    bool reversed;
public:
    CsgPointWitness();
    CsgPointWitness(const CsgPointWitness& other);
    CsgPointWitness& operator=(const CsgPointWitness& other);
    ~CsgPointWitness();
    void clear();
    bool prepend(const CsgOperandView* operand);
    void reverseOrientation();
    bool isAvailable() const;
    bool isReversed() const;
    long int getLength() const;
    const CsgOperandView* getOperand(long int index) const;
};
#endif
