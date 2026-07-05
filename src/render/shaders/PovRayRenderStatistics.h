#ifndef __POV_RAY_RENDER_STATISTICS__
#define __POV_RAY_RENDER_STATISTICS__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/statistics/SolidTextureStatistics.h"
#include "vsdk/toolkit/common/statistics/GeometryStatistics.h"

class PovRayRenderStatistics {
  private:
    SolidTextureStatistics solidTextureStatistics;
    GeometryStatistics geometryStatistics;

    long numberOfPixels;
    long numberOfRays;
    long numberOfPixelsSuperSampled;
    long shadowRayTests;
    long reflectedRaysTraced;
    long refractedRaysTraced;
    long transmittedRaysTraced;
    double usedTime;

  public:
    PovRayRenderStatistics() { reset(); }
    PovRayRenderStatistics(java::ArrayList<PovRayRenderStatistics*> *partsPerThread);

    SolidTextureStatistics *getSolidTextureStatistics();
    const SolidTextureStatistics *getSolidTextureStatistics() const;
    GeometryStatistics *getGeometryStatistics();
    const GeometryStatistics *getGeometryStatistics() const;

    long getNumberOfPixels() const;
    void incrementNumberOfPixels();
    long getNumberOfRays() const;
    void incrementNumberOfRays();
    long getNumberOfPixelsSuperSampled() const;
    void incrementNumberOfPixelsSuperSampled();
    long getShadowRayTests() const;
    void incrementShadowRayTests();
    long getReflectedRaysTraced() const;
    void incrementReflectedRaysTraced();
    long getRefractedRaysTraced() const;
    void incrementRefractedRaysTraced();
    long getTransmittedRaysTraced() const;
    void incrementTransmittedRaysTraced();
    double getUsedTime() const;
    void reset();
};

inline const SolidTextureStatistics *PovRayRenderStatistics::getSolidTextureStatistics() const { return &solidTextureStatistics; }
inline double PovRayRenderStatistics::getUsedTime() const { return usedTime; }
inline GeometryStatistics *PovRayRenderStatistics::getGeometryStatistics() { return &geometryStatistics; }
inline const GeometryStatistics *PovRayRenderStatistics::getGeometryStatistics() const { return &geometryStatistics; }

inline long PovRayRenderStatistics::getNumberOfPixels() const { return numberOfPixels; }
inline void PovRayRenderStatistics::incrementNumberOfPixels() { ++numberOfPixels; }
inline long PovRayRenderStatistics::getNumberOfRays() const { return numberOfRays; }
inline void PovRayRenderStatistics::incrementNumberOfRays() { ++numberOfRays; }
inline long PovRayRenderStatistics::getNumberOfPixelsSuperSampled() const { return numberOfPixelsSuperSampled; }
inline void PovRayRenderStatistics::incrementNumberOfPixelsSuperSampled() { ++numberOfPixelsSuperSampled; }
inline long PovRayRenderStatistics::getShadowRayTests() const { return shadowRayTests; }
inline void PovRayRenderStatistics::incrementShadowRayTests() { ++shadowRayTests; }
inline long PovRayRenderStatistics::getReflectedRaysTraced() const { return reflectedRaysTraced; }
inline void PovRayRenderStatistics::incrementReflectedRaysTraced() { ++reflectedRaysTraced; }
inline long PovRayRenderStatistics::getRefractedRaysTraced() const { return refractedRaysTraced; }
inline void PovRayRenderStatistics::incrementRefractedRaysTraced() { ++refractedRaysTraced; }
inline long PovRayRenderStatistics::getTransmittedRaysTraced() const { return transmittedRaysTraced; }
inline void PovRayRenderStatistics::incrementTransmittedRaysTraced() { ++transmittedRaysTraced; }

#endif
