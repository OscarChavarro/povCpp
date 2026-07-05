#include "java/util/ArrayList.txx"
#include "render/shaders/PovRayRenderStatistics.h"

// Supports multi-thread ray tracing. PovRayRenderStatistics must be totaled after all
// worker threads have been joined.
PovRayRenderStatistics::PovRayRenderStatistics(java::ArrayList<PovRayRenderStatistics*> *partsPerThread)
{
    reset();

    if (partsPerThread == nullptr) {
        return;
    }

    // Collected alongside the scalar counters below and reduced through
    // SolidTextureStatistics's / GeometryStatistics's own parts-summing
    // constructors, rather than summing their fields inline here, so each
    // reduction lives in one place and stays in sync however many fields it
    // grows to.
    java::ArrayList<SolidTextureStatistics*> solidTextureStatisticsParts;
    java::ArrayList<GeometryStatistics*> geometryStatisticsParts;

    for (long i = 0; i < partsPerThread->size(); ++i) {
        const PovRayRenderStatistics *part = partsPerThread->get(i);
        if (part == nullptr) {
            continue;
        }

        solidTextureStatisticsParts.add(
            const_cast<SolidTextureStatistics*>(&part->solidTextureStatistics));
        geometryStatisticsParts.add(
            const_cast<GeometryStatistics*>(&part->geometryStatistics));

        numberOfPixels += part->numberOfPixels;
        numberOfRays += part->numberOfRays;
        numberOfPixelsSuperSampled += part->numberOfPixelsSuperSampled;
        shadowRayTests += part->shadowRayTests;
        reflectedRaysTraced += part->reflectedRaysTraced;
        refractedRaysTraced += part->refractedRaysTraced;
        transmittedRaysTraced += part->transmittedRaysTraced;
        usedTime += part->usedTime;
    }

    solidTextureStatistics = SolidTextureStatistics(&solidTextureStatisticsParts);
    geometryStatistics = GeometryStatistics(&geometryStatisticsParts);
}

SolidTextureStatistics*
PovRayRenderStatistics::getSolidTextureStatistics()
{
    return &solidTextureStatistics;
}

void
PovRayRenderStatistics::reset()
{
    numberOfPixels = 0L;
    numberOfRays = 0L;
    numberOfPixelsSuperSampled = 0L;
    geometryStatistics.reset();
    solidTextureStatistics.reset();
    shadowRayTests = 0L;
    reflectedRaysTraced = 0L;
    refractedRaysTraced = 0L;
    transmittedRaysTraced = 0L;
    usedTime = 0.0;
}
