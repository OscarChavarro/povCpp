#ifndef __PATCH_SCRATCH__
#define __PATCH_SCRATCH__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

// Per-ray scratch a ParametricBiCubicPatch needs between
// doIntersectionForAllRayCrossings() (which fills it in) and the later
// computeSurfaceNormal() call for the winning hit (which reads it back by
// matching the intersection point). See ParametricBiCubicPatch.h's private
// section for the thread-local-keyed-by-patch-identity rationale.
class PatchScratch {
  public:
    static constexpr int CAPACITY = 32;

    PatchScratch() : intersectionCount(0) {}

    int getIntersectionCount() const { return intersectionCount; }
    void setIntersectionCount(int count) { intersectionCount = count; }
    void incrementIntersectionCount() { intersectionCount++; }

    Vector3Dd &getIntersectionPointAt(int index) { return intersectionPoint[index]; }
    Vector3Dd &getNormalVectorAt(int index) { return normalVector[index]; }

  private:
    int intersectionCount;
    Vector3Dd intersectionPoint[CAPACITY];
    Vector3Dd normalVector[CAPACITY];
};

#endif
