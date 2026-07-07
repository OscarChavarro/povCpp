#ifndef __AABB_CULLING_SUPPORT__
#define __AABB_CULLING_SUPPORT__

#include "render/bakedScene/BakedScene.h"

class AabbCullingSupport {
public:
    static constexpr int OPERAND_CULL_SCRATCH_CAPACITY = 4096;

    // Traverses a bake-time OperandCullBins index, testing each surviving
    // bin's aggregate box then each member's own bakedBounds (the aggregate
    // box can pass while an individual member's tighter box does not), and
    // appends the never-cull-safe positions untested exactly as the plain
    // linear scan does today. Returns -1 on scratch overflow (caller must
    // fall back), otherwise the number of positions written to outPositions
    // (unsorted - caller must sort descending to match the plain scan's
    // iteration order before dispatching).
    static inline int gatherCullSurvivors(
        const OperandCullBins &bins,
        const java::ArrayList<int> &bucket,
        const java::ArrayList<CsgOperandRecord> &operands,
        RayWithTracingState &ray,
        int *outPositions,
        int capacity)
    {
        int count = 0;
        for (long int b = 0; b < bins.getBinBounds().size(); b++) {
            if (!bins.getBinBounds()[b].intersectsRayForward(ray)) {
                continue;
            }
            const int start = bins.getBinMemberStart()[b];
            const int memberCount = bins.getBinMemberCount()[b];
            for (int m = start; m < start + memberCount; m++) {
                const int pos = bins.getBinMembers()[m];
                if (!operands[bucket[pos]].getBakedBounds().intersectsRayForward(ray)) {
                    continue;
                }
                if (count >= capacity) {
                    return -1;
                }
                outPositions[count++] = pos;
            }
        }
        for (long int i = 0; i < bins.getAlwaysTestedPositions().size(); i++) {
            if (count >= capacity) {
                return -1;
            }
            outPositions[count++] = bins.getAlwaysTestedPositions()[i];
        }
        return count;
    }

    // Small-array insertion sort, descending - matches the plain scan's
    // `for (p = size-1; p >= 0; p--)` traversal order exactly. Survivor
    // counts here are a handful to a few dozen even for wide unions, so
    // O(k^2) is negligible and allocation-free.
    static inline void sortPositionsDescending(int *positions, int count)
    {
        for (int i = 1; i < count; i++) {
            const int key = positions[i];
            int j = i - 1;
            while (j >= 0 && positions[j] < key) {
                positions[j + 1] = positions[j];
                j--;
            }
            positions[j + 1] = key;
        }
    }

};

#endif
