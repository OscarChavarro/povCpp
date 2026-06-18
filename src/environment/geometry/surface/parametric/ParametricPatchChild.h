#ifndef __PARAMETRIC_PATCH_CHILD__
#define __PARAMETRIC_PATCH_CHILD__

#include "environment/geometry/surface/parametric/ParametricPatchNode.h"

class ParametricPatchChild {
  public:
    ParametricPatchNode *getChild(int index) const;
    void setChild(int index, ParametricPatchNode *value);

  private:
    ParametricPatchNode *children[4];
};

inline ParametricPatchNode *ParametricPatchChild::getChild(int index) const
{
    return children[index];
}

inline void ParametricPatchChild::setChild(int index, ParametricPatchNode *value)
{
    children[index] = value;
}

#endif
