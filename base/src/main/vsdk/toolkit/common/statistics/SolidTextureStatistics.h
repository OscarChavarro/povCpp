#ifndef __SOLIDTEXTURESTATISTICS__
#define __SOLIDTEXTURESTATISTICS__

#include "java/util/ArrayList.h"

class SolidTextureStatistics {
  public:
    long callsToNoise = 0;
    long callsToDNoise = 0;

    SolidTextureStatistics() = default;

    // Supports multi-thread rendering: each RenderTask binds its own
    // ProceduralNoise to its own SolidTextureStatistics instance (so noise()/
    // differentialNoise() call counters never race across threads), then
    // this reduces every task's instance into one total once all tasks have
    // joined. Mirrors Statistics(ArrayList<Statistics*>*).
    explicit SolidTextureStatistics(
        java::ArrayList<SolidTextureStatistics *> *partsPerThread);

    void reset() { callsToNoise = 0L; callsToDNoise = 0L; }
};

#endif
