#ifndef __OPERAND_CULL_BINS__
#define __OPERAND_CULL_BINS__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AabbBinHierarchy.h"

// Bake-time culling index of a wide union bucket: the VITRAL bin hierarchy
// over its cull-safe operands, identified by bucket *position* (never operand
// pointers or global indices, so nothing dangles across later ArrayList
// relocations), and the positions that are always tested.
class OperandCullBins {
  public:
    OperandCullBins(const java::ArrayList<AxisAlignedBoundingBox> &cullSafeBounds,
        const java::ArrayList<int> &cullSafePositions,
        const java::ArrayList<int> &alwaysTestedPositions) :
        alwaysTestedPositions(alwaysTestedPositions)
    {
        hierarchy.build(cullSafeBounds, cullSafePositions);
    }

    const AabbBinHierarchy &getHierarchy() const { return hierarchy; }
    const java::ArrayList<int> &getAlwaysTestedPositions() const { return alwaysTestedPositions; }

  private:
    AabbBinHierarchy hierarchy;
    java::ArrayList<int> alwaysTestedPositions;
};

#endif
