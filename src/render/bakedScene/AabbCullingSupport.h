#ifndef __AABB_CULLING_SUPPORT__
#define __AABB_CULLING_SUPPORT__

#include "render/bakedScene/BakedScene.h"

class AabbCullingSupport {
public:
    static constexpr int OPERAND_CULL_SCRATCH_CAPACITY = 4096;

    static inline bool rayIntersectsAabbForward(
        const RayWithTracingState &ray,
        const AxisAlignedBoundingBox &box)
    {
        const Vector3Dd origin = ray.getOrigin();
        double invDirX, invDirY, invDirZ;
        bool degenerateX, degenerateY, degenerateZ;
        ray.getAabbSlabReciprocals(
            &invDirX, &invDirY, &invDirZ,
            &degenerateX, &degenerateY, &degenerateZ);
        double tMin = 0.0;
        double tMax = 1e30;

        auto updateAxis = [&](double originCoord, double invDir, bool degenerate,
                              double minCoord, double maxCoord) -> bool {
            if (degenerate) {
                return originCoord >= minCoord && originCoord <= maxCoord;
            }
            double nearT = (minCoord - originCoord) * invDir;
            double farT = (maxCoord - originCoord) * invDir;
            if (nearT > farT) {
                const double tmp = nearT;
                nearT = farT;
                farT = tmp;
            }
            tMin = nearT > tMin ? nearT : tMin;
            tMax = farT < tMax ? farT : tMax;
            return tMin <= tMax;
        };

        return
            updateAxis(origin.x(), invDirX, degenerateX, box.getMin().x(), box.getMax().x()) &&
            updateAxis(origin.y(), invDirY, degenerateY, box.getMin().y(), box.getMax().y()) &&
            updateAxis(origin.z(), invDirZ, degenerateZ, box.getMin().z(), box.getMax().z()) &&
            tMax >= 0.0;
    }

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
        const java::ArrayList<CsgOperandRecord *> &operands,
        RayWithTracingState &ray,
        int *outPositions,
        int capacity)
    {
        int count = 0;
        for (long int b = 0; b < bins.getBinBounds().size(); b++) {
            if (!rayIntersectsAabbForward(ray, bins.getBinBounds()[b])) {
                continue;
            }
            const int start = bins.getBinMemberStart()[b];
            const int memberCount = bins.getBinMemberCount()[b];
            for (int m = start; m < start + memberCount; m++) {
                const int pos = bins.getBinMembers()[m];
                if (!rayIntersectsAabbForward(ray, operands[bucket[pos]]->getBakedBounds())) {
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

    static bool pointInsideAabb(
        const Vector3Dd &point,
        const AxisAlignedBoundingBox &box,
        double tolerance);
};

#endif
