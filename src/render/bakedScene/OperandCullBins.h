#ifndef __OPERAND_CULL_BINS__
#define __OPERAND_CULL_BINS__

#include "environment/geometry/element/AxisAlignedBoundingBox.h"
#include "java/util/ArrayList.h"

// Bake-time only. Stores bucket *positions*, never operand pointers or
// global indices, so nothing dangles across later ArrayList relocations.
class OperandCullBins {
  public:
    OperandCullBins(
        bool built,
        const java::ArrayList<AxisAlignedBoundingBox> &binBounds,
        const java::ArrayList<int> &binMemberStart,
        const java::ArrayList<int> &binMemberCount,
        const java::ArrayList<int> &binMembers,
        const java::ArrayList<int> &alwaysTestedPositions) :
        built(built),
        binBounds(binBounds),
        binMemberStart(binMemberStart),
        binMemberCount(binMemberCount),
        binMembers(binMembers),
        alwaysTestedPositions(alwaysTestedPositions)
    {}

    bool getBuilt() const { return built; }
    const java::ArrayList<AxisAlignedBoundingBox> &getBinBounds() const { return binBounds; }
    const java::ArrayList<int> &getBinMemberStart() const { return binMemberStart; }
    const java::ArrayList<int> &getBinMemberCount() const { return binMemberCount; }
    const java::ArrayList<int> &getBinMembers() const { return binMembers; }
    const java::ArrayList<int> &getAlwaysTestedPositions() const { return alwaysTestedPositions; }

  private:
    bool built;
    java::ArrayList<AxisAlignedBoundingBox> binBounds;
    java::ArrayList<int> binMemberStart;
    java::ArrayList<int> binMemberCount;
    java::ArrayList<int> binMembers;
    java::ArrayList<int> alwaysTestedPositions;
};

#endif
