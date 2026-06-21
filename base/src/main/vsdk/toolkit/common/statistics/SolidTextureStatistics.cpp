#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/SolidTextureStatistics.h"

SolidTextureStatistics::SolidTextureStatistics(
    java::ArrayList<SolidTextureStatistics *> *partsPerThread)
{
    reset();

    if (partsPerThread == nullptr) {
        return;
    }

    for (long i = 0; i < partsPerThread->size(); ++i) {
        const SolidTextureStatistics *part = partsPerThread->get(i);
        if (part == nullptr) {
            continue;
        }
        callsToNoise += part->callsToNoise;
        callsToDNoise += part->callsToDNoise;
    }
}
