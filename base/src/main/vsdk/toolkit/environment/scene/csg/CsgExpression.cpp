#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
CsgExpression::~CsgExpression() {}

bool CsgExpression::isStructuralTraversalSafe() const { return false; }

bool CsgExpression::queryStructureVersion(long long&) const { return false; }
