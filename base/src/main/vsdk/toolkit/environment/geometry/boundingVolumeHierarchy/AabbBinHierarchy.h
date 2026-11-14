#ifndef __AABB_BIN_HIERARCHY__
#define __AABB_BIN_HIERARCHY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AxisAlignedBoundingBox.h"

/**
Two-level hierarchy of axis aligned boxes over a set of primitives, built once
before tracing. The primitives are sorted by the centroid coordinate along the
axis of widest centroid spread and cut into about the square root of their
count bins (at most 64) of consecutive primitives; each bin keeps the box that
encloses its members.

The structure is a set of flat, index-linked arrays without virtual dispatch,
in the form of a bottom-level acceleration structure over procedural boxes
(Vulkan `VK_GEOMETRY_TYPE_AABBS_KHR`): a member is identified by the caller's
primitive id, and its box is copied into the structure in bin order.

A query lists the ids of the members whose own box passes the forward slab
test, in bin order and member order inside a bin; it never omits a primitive
whose box the ray crosses.
*/
class AabbBinHierarchy {
  private:
    static constexpr int MAXIMUM_BIN_COUNT = 64;

    java::ArrayList<AxisAlignedBoundingBox> binBounds;
    java::ArrayList<int> binMemberStart;
    java::ArrayList<int> binMemberCount;
    java::ArrayList<int> memberIds;
    java::ArrayList<AxisAlignedBoundingBox> memberBounds;

  public:
    AabbBinHierarchy();

    /** Builds the hierarchy over `boxes`, whose primitive ids are `ids`
    (same size). */
    void build(const java::ArrayList<AxisAlignedBoundingBox> &boxes,
        const java::ArrayList<int> &ids);

    long int getBinCount() const;
    long int getMemberCount() const;
    const AxisAlignedBoundingBox &getBinBounds(long int bin) const;

    /** Writes into `outIds` the ids of the members whose box passes the
    forward slab test and returns their count, or -1 when more than
    `capacity` members pass. */
    int gatherCandidates(const Ray &ray, const RaySlabReciprocals &reciprocals,
        int *outIds, int capacity) const;
};

inline int AabbBinHierarchy::gatherCandidates(const Ray &ray,
    const RaySlabReciprocals &reciprocals, int *outIds, int capacity) const
{
    int count = 0;
    for (long int b = 0; b < binBounds.size(); b++) {
        if (!binBounds[b].intersectsRayForward(ray, reciprocals)) {
            continue;
        }
        const int start = binMemberStart[b];
        const int end = start + binMemberCount[b];
        for (int m = start; m < end; m++) {
            if (!memberBounds[m].intersectsRayForward(ray, reciprocals)) {
                continue;
            }
            if (count >= capacity) {
                return -1;
            }
            outIds[count++] = memberIds[m];
        }
    }
    return count;
}

#endif
