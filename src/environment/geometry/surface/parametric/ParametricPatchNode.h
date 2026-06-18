#ifndef __PARAMETRIC_PATCH_NODE_H__
#define __PARAMETRIC_PATCH_NODE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class ParametricPatchNode {
  public:
    int getNodeType() const { return nodeType; }
    void setNodeType(int value) { nodeType = value; }
    Vector3Dd &getCenter() { return center; }
    const Vector3Dd &getCenter() const { return center; }
    double &getRadiusSquaredRef() { return radiusSquared; }
    double getRadiusSquared() const { return radiusSquared; }
    void setRadiusSquared(double value) { radiusSquared = value; }
    int getCount() const { return count; }
    void setCount(int value) { count = value; }
    void *getDataPtr() const { return dataPtr; }
    void setDataPtr(void *value) { dataPtr = value; }

  private:
    int nodeType;
    Vector3Dd center;
    double radiusSquared;
    int count;
    void *dataPtr;
};

#endif
