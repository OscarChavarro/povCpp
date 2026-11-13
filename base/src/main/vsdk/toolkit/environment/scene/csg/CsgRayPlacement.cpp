#include "vsdk/toolkit/environment/scene/csg/CsgRayPlacement.h"
CsgRayPlacement::CsgRayPlacement() : operand(nullptr), childToParent(), parentToChild() {}
CsgRayPlacement::CsgRayPlacement(const CsgOperandView* value, const Matrix4x4d& forward,
    const Matrix4x4d& inverse) : operand(value), childToParent(forward), parentToChild(inverse) {}
