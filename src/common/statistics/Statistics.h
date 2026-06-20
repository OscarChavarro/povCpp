#ifndef __STATISTICS__
#define __STATISTICS__

#include "vsdk/toolkit/common/statistics/SolidTextureStatistics.h"

class Statistics {
  private:
    long numberOfPixels;
    long numberOfRays;
    long numberOfPixelsSuperSampled;
    long raySphereTests;
    long raySphereTestsSucceeded;
    long rayBoxTests;
    long rayBoxTestsSucceeded;
    long rayBlobTests;
    long rayBlobTestsSucceeded;
    long rayPlaneTests;
    long rayPlaneTestsSucceeded;
    long rayTriangleTests;
    long rayTriangleTestsSucceeded;
    long rayQuadricTests;
    long rayQuadricTestsSucceeded;
    long rayPolyTests;
    long rayPolyTestsSucceeded;
    long rayBicubicTests;
    long rayBicubicTestsSucceeded;
    long rayHtFieldTests;
    long rayHtFieldTestsSucceeded;
    long boundingRegionTests;
    long boundingRegionTestsSucceeded;
    long clippingRegionTests;
    long clippingRegionTestsSucceeded;
    SolidTextureStatistics solidTextureStatistics;
    long shadowRayTests;
    long reflectedRaysTraced;
    long refractedRaysTraced;
    long transmittedRaysTraced;
    double usedTime;

  public:
    Statistics() { reset(); }
    long getNumberOfPixels() const;
    void incrementNumberOfPixels();
    long getNumberOfRays() const;
    void incrementNumberOfRays();
    long getNumberOfPixelsSuperSampled() const;
    void incrementNumberOfPixelsSuperSampled();
    long getRaySphereTests() const;
    void incrementRaySphereTests();
    long getRaySphereTestsSucceeded() const;
    void incrementRaySphereTestsSucceeded();
    long getRayBoxTests() const;
    void incrementRayBoxTests();
    long getRayBoxTestsSucceeded() const;
    void incrementRayBoxTestsSucceeded();
    long getRayBlobTests() const;
    void incrementRayBlobTests();
    long getRayBlobTestsSucceeded() const;
    void incrementRayBlobTestsSucceeded();
    long getRayPlaneTests() const;
    void incrementRayPlaneTests();
    long getRayPlaneTestsSucceeded() const;
    void incrementRayPlaneTestsSucceeded();
    long getRayTriangleTests() const;
    void incrementRayTriangleTests();
    long getRayTriangleTestsSucceeded() const;
    void incrementRayTriangleTestsSucceeded();
    long getRayQuadricTests() const;
    void incrementRayQuadricTests();
    long getRayQuadricTestsSucceeded() const;
    void incrementRayQuadricTestsSucceeded();
    long getRayPolyTests() const;
    void incrementRayPolyTests();
    long getRayPolyTestsSucceeded() const;
    void incrementRayPolyTestsSucceeded();
    long getRayBicubicTests() const;
    void incrementRayBicubicTests();
    long getRayBicubicTestsSucceeded() const;
    void incrementRayBicubicTestsSucceeded();
    long getRayHtFieldTests() const;
    void incrementRayHtFieldTests();
    long getRayHtFieldTestsSucceeded() const;
    void incrementRayHtFieldTestsSucceeded();
    long getBoundingRegionTests() const;
    void incrementBoundingRegionTests();
    long getBoundingRegionTestsSucceeded() const;
    void incrementBoundingRegionTestsSucceeded();
    long getClippingRegionTests() const;
    void incrementClippingRegionTests();
    long getClippingRegionTestsSucceeded() const;
    void incrementClippingRegionTestsSucceeded();
    SolidTextureStatistics *getSolidTextureStatistics();
    const SolidTextureStatistics *getSolidTextureStatistics() const;
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

inline long Statistics::getNumberOfPixels() const { return numberOfPixels; }
inline void Statistics::incrementNumberOfPixels() { ++numberOfPixels; }
inline long Statistics::getNumberOfRays() const { return numberOfRays; }
inline void Statistics::incrementNumberOfRays() { ++numberOfRays; }
inline long Statistics::getNumberOfPixelsSuperSampled() const { return numberOfPixelsSuperSampled; }
inline void Statistics::incrementNumberOfPixelsSuperSampled() { ++numberOfPixelsSuperSampled; }
inline long Statistics::getRaySphereTests() const { return raySphereTests; }
inline void Statistics::incrementRaySphereTests() { ++raySphereTests; }
inline long Statistics::getRaySphereTestsSucceeded() const { return raySphereTestsSucceeded; }
inline void Statistics::incrementRaySphereTestsSucceeded() { ++raySphereTestsSucceeded; }
inline long Statistics::getRayBoxTests() const { return rayBoxTests; }
inline void Statistics::incrementRayBoxTests() { ++rayBoxTests; }
inline long Statistics::getRayBoxTestsSucceeded() const { return rayBoxTestsSucceeded; }
inline void Statistics::incrementRayBoxTestsSucceeded() { ++rayBoxTestsSucceeded; }
inline long Statistics::getRayBlobTests() const { return rayBlobTests; }
inline void Statistics::incrementRayBlobTests() { ++rayBlobTests; }
inline long Statistics::getRayBlobTestsSucceeded() const { return rayBlobTestsSucceeded; }
inline void Statistics::incrementRayBlobTestsSucceeded() { ++rayBlobTestsSucceeded; }
inline long Statistics::getRayPlaneTests() const { return rayPlaneTests; }
inline void Statistics::incrementRayPlaneTests() { ++rayPlaneTests; }
inline long Statistics::getRayPlaneTestsSucceeded() const { return rayPlaneTestsSucceeded; }
inline void Statistics::incrementRayPlaneTestsSucceeded() { ++rayPlaneTestsSucceeded; }
inline long Statistics::getRayTriangleTests() const { return rayTriangleTests; }
inline void Statistics::incrementRayTriangleTests() { ++rayTriangleTests; }
inline long Statistics::getRayTriangleTestsSucceeded() const { return rayTriangleTestsSucceeded; }
inline void Statistics::incrementRayTriangleTestsSucceeded() { ++rayTriangleTestsSucceeded; }
inline long Statistics::getRayQuadricTests() const { return rayQuadricTests; }
inline void Statistics::incrementRayQuadricTests() { ++rayQuadricTests; }
inline long Statistics::getRayQuadricTestsSucceeded() const { return rayQuadricTestsSucceeded; }
inline void Statistics::incrementRayQuadricTestsSucceeded() { ++rayQuadricTestsSucceeded; }
inline long Statistics::getRayPolyTests() const { return rayPolyTests; }
inline void Statistics::incrementRayPolyTests() { ++rayPolyTests; }
inline long Statistics::getRayPolyTestsSucceeded() const { return rayPolyTestsSucceeded; }
inline void Statistics::incrementRayPolyTestsSucceeded() { ++rayPolyTestsSucceeded; }
inline long Statistics::getRayBicubicTests() const { return rayBicubicTests; }
inline void Statistics::incrementRayBicubicTests() { ++rayBicubicTests; }
inline long Statistics::getRayBicubicTestsSucceeded() const { return rayBicubicTestsSucceeded; }
inline void Statistics::incrementRayBicubicTestsSucceeded() { ++rayBicubicTestsSucceeded; }
inline long Statistics::getRayHtFieldTests() const { return rayHtFieldTests; }
inline void Statistics::incrementRayHtFieldTests() { ++rayHtFieldTests; }
inline long Statistics::getRayHtFieldTestsSucceeded() const { return rayHtFieldTestsSucceeded; }
inline void Statistics::incrementRayHtFieldTestsSucceeded() { ++rayHtFieldTestsSucceeded; }
inline long Statistics::getBoundingRegionTests() const { return boundingRegionTests; }
inline void Statistics::incrementBoundingRegionTests() { ++boundingRegionTests; }
inline long Statistics::getBoundingRegionTestsSucceeded() const { return boundingRegionTestsSucceeded; }
inline void Statistics::incrementBoundingRegionTestsSucceeded() { ++boundingRegionTestsSucceeded; }
inline long Statistics::getClippingRegionTests() const { return clippingRegionTests; }
inline void Statistics::incrementClippingRegionTests() { ++clippingRegionTests; }
inline long Statistics::getClippingRegionTestsSucceeded() const { return clippingRegionTestsSucceeded; }
inline void Statistics::incrementClippingRegionTestsSucceeded() { ++clippingRegionTestsSucceeded; }
inline const SolidTextureStatistics *Statistics::getSolidTextureStatistics() const { return &solidTextureStatistics; }
inline long Statistics::getShadowRayTests() const { return shadowRayTests; }
inline void Statistics::incrementShadowRayTests() { ++shadowRayTests; }
inline long Statistics::getReflectedRaysTraced() const { return reflectedRaysTraced; }
inline void Statistics::incrementReflectedRaysTraced() { ++reflectedRaysTraced; }
inline long Statistics::getRefractedRaysTraced() const { return refractedRaysTraced; }
inline void Statistics::incrementRefractedRaysTraced() { ++refractedRaysTraced; }
inline long Statistics::getTransmittedRaysTraced() const { return transmittedRaysTraced; }
inline void Statistics::incrementTransmittedRaysTraced() { ++transmittedRaysTraced; }
inline double Statistics::getUsedTime() const { return usedTime; }

#endif
