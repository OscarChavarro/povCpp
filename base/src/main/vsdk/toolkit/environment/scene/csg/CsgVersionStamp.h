#ifndef VSDK_CSG_VERSION_STAMP_H
#define VSDK_CSG_VERSION_STAMP_H
#include "java/util/ArrayList.h"
class CsgExpression;
class CsgOperandView;
// Modification versions of every expression node and operand reachable from
// a root, recorded when a result that borrows them is built. A result is known
// to be current only when the stamp is complete (every node and operand
// reports a version) and every version is unchanged. Versions cover edits made
// through the scene and CSG APIs; geometry parameters changed behind a body's
// back are not observed. Storage is reused across captures.
class CsgVersionStamp {
private:
    java::ArrayList<const CsgExpression*> expressions;
    java::ArrayList<long long> expressionVersions;
    java::ArrayList<const CsgOperandView*> operands;
    java::ArrayList<long long> operandVersions;
    java::ArrayList<const CsgExpression*> path;
    bool complete;
    bool captured;
    bool record(const CsgExpression& expression);
public:
    CsgVersionStamp();
    CsgVersionStamp(const CsgVersionStamp& other);
    CsgVersionStamp& operator=(const CsgVersionStamp& other);
    ~CsgVersionStamp();
    void clear();
    // False only for a cyclic expression or exhausted storage; the stamp is
    // then cleared.
    bool capture(const CsgExpression& root);
    bool isCaptured() const;
    bool isComplete() const;
    bool isCurrent() const;
};
#endif
