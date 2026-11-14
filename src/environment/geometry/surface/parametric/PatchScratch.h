#ifndef __PATCH_SCRATCH__
#define __PATCH_SCRATCH__

#include "java/util/HashMap.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatch.h"

// Per-thread hit cache of one bicubic patch: crossing emission fills it and
// the detail kernel of the winning hit reads the normal back by matching the
// intersection point. PovRay shares MAX_BICUBIC_INTERSECTIONS hits per patch
// and thread between the rays traced since the last primary ray.
class PatchScratch {
  public:
    static constexpr int CAPACITY = 32;

    static PatchScratch &forPatch(const SubdividedBezierPatch *patch);

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

inline PatchScratch &
PatchScratch::forPatch(const SubdividedBezierPatch *patch)
{
    thread_local java::HashMap<const SubdividedBezierPatch *, PatchScratch> scratchByPatch;
    PatchScratch *scratch = scratchByPatch.get(patch);
    if (scratch == nullptr) {
        scratchByPatch.put(patch, PatchScratch());
        scratch = scratchByPatch.get(patch);
    }
    return *scratch;
}

#endif
