#ifndef __RAY_SHARED_CACHE__
#define __RAY_SHARED_CACHE__

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
