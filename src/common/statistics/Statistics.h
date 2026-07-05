#ifndef __STATISTICS__
#define __STATISTICS__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/statistics/SolidTextureStatistics.h"
#include "common/statistics/GeometryStatistics.h"

class Statistics {
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
    Statistics() { reset(); }
    Statistics(java::ArrayList<Statistics*> *partsPerThread);

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

inline const SolidTextureStatistics *Statistics::getSolidTextureStatistics() const { return &solidTextureStatistics; }
inline double Statistics::getUsedTime() const { return usedTime; }
inline GeometryStatistics *Statistics::getGeometryStatistics() { return &geometryStatistics; }
inline const GeometryStatistics *Statistics::getGeometryStatistics() const { return &geometryStatistics; }

inline long Statistics::getNumberOfPixels() const { return numberOfPixels; }
inline void Statistics::incrementNumberOfPixels() { ++numberOfPixels; }
inline long Statistics::getNumberOfRays() const { return numberOfRays; }
inline void Statistics::incrementNumberOfRays() { ++numberOfRays; }
inline long Statistics::getNumberOfPixelsSuperSampled() const { return numberOfPixelsSuperSampled; }
inline void Statistics::incrementNumberOfPixelsSuperSampled() { ++numberOfPixelsSuperSampled; }
inline long Statistics::getShadowRayTests() const { return shadowRayTests; }
inline void Statistics::incrementShadowRayTests() { ++shadowRayTests; }
inline long Statistics::getReflectedRaysTraced() const { return reflectedRaysTraced; }
inline void Statistics::incrementReflectedRaysTraced() { ++reflectedRaysTraced; }
inline long Statistics::getRefractedRaysTraced() const { return refractedRaysTraced; }
inline void Statistics::incrementRefractedRaysTraced() { ++refractedRaysTraced; }
inline long Statistics::getTransmittedRaysTraced() const { return transmittedRaysTraced; }
inline void Statistics::incrementTransmittedRaysTraced() { ++transmittedRaysTraced; }

#endif
