#ifndef __RAY_SHARED_CACHE__
#define __RAY_SHARED_CACHE__

// Plan 7: per-render-task cache for ray-shared quadric/plane viewpoint
// constants that used to live as mutable fields on shared baked records
// (Quadric::objectVpConstant/constantCached read/written from
// BakedCsgTrace's copies, CsgOperandRecord::planeVp*). Those mutable fields
// were safe only because the old render loop was single-threaded per scene;
// under -parallel, multiple tile threads intersecting the same shared
// BakedScene concurrently would race on them (same class of bug as the B6
// noise-stats race in the parallel-raytracer plan). This class moves that
// state into a flat array owned by the render task (one instance per
// RenderWorker), indexed by a slot assigned once at BakedSceneBuilder build
// time - no per-ray allocation, no hashing.
//
// The constants cached here depend only on each baked shape's own
// coefficients and the fixed primary-ray eye position, so unlike a
// per-ray cache they are valid for the entire render (never invalidated
// mid-frame) - this mirrors the lifetime of the mutable fields it replaces.
// Per-ray aggregate reuse (position2/direction2/... for world-space
// operands) does not need any storage here: RayWithSegments already caches
// those itself (quadricConstantsCached, reset on every new ray generation
// by every ray-construction call site) - BakedCsgTrace's intersectBaked*
// helpers now read that existing cache instead of duplicating it.
class RaySharedCache {
  public:
    RaySharedCache() :
        quadricSlotCount(0), planeSlotCount(0),
        quadricComputed(nullptr), quadricValue(nullptr),
        planeComputed(nullptr), planeValue(nullptr)
    {
    }

    ~RaySharedCache()
    {
        release();
    }

    RaySharedCache(const RaySharedCache &) = delete;
    RaySharedCache &operator=(const RaySharedCache &) = delete;

    // No-op after the first call for a given (quadricSlots, planeSlots) pair -
    // the scene is built once per render, so every subsequent call from the
    // per-ray entry points is a cheap size check.
    void ensureCapacity(int quadricSlots, int planeSlots)
    {
        if (quadricSlots != quadricSlotCount) {
            delete[] quadricComputed;
            delete[] quadricValue;
            quadricSlotCount = quadricSlots;
            quadricComputed = quadricSlotCount > 0 ? new bool[quadricSlotCount] : nullptr;
            quadricValue = quadricSlotCount > 0 ? new double[quadricSlotCount] : nullptr;
            for (int i = 0; i < quadricSlotCount; i++) {
                quadricComputed[i] = false;
            }
        }
        if (planeSlots != planeSlotCount) {
            delete[] planeComputed;
            delete[] planeValue;
            planeSlotCount = planeSlots;
            planeComputed = planeSlotCount > 0 ? new bool[planeSlotCount] : nullptr;
            planeValue = planeSlotCount > 0 ? new double[planeSlotCount] : nullptr;
            for (int i = 0; i < planeSlotCount; i++) {
                planeComputed[i] = false;
            }
        }
    }

    bool getQuadricViewpointConstant(int slot, double &value) const
    {
        if (slot < 0 || slot >= quadricSlotCount || !quadricComputed[slot]) {
            return false;
        }
        value = quadricValue[slot];
        return true;
    }

    void setQuadricViewpointConstant(int slot, double value)
    {
        if (slot < 0 || slot >= quadricSlotCount) {
            return;
        }
        quadricValue[slot] = value;
        quadricComputed[slot] = true;
    }

    bool getPlaneViewpointConstant(int slot, double &value) const
    {
        if (slot < 0 || slot >= planeSlotCount || !planeComputed[slot]) {
            return false;
        }
        value = planeValue[slot];
        return true;
    }

    void setPlaneViewpointConstant(int slot, double value)
    {
        if (slot < 0 || slot >= planeSlotCount) {
            return;
        }
        planeValue[slot] = value;
        planeComputed[slot] = true;
    }

  private:
    void release()
    {
        delete[] quadricComputed;
        delete[] quadricValue;
        delete[] planeComputed;
        delete[] planeValue;
        quadricComputed = nullptr;
        quadricValue = nullptr;
        planeComputed = nullptr;
        planeValue = nullptr;
        quadricSlotCount = 0;
        planeSlotCount = 0;
    }

    int quadricSlotCount;
    int planeSlotCount;
    bool *quadricComputed;
    double *quadricValue;
    bool *planeComputed;
    double *planeValue;
};

#endif
