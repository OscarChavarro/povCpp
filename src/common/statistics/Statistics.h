#ifndef __STATISTICS__
#define __STATISTICS__

#include <ctime>

#include "vsdk/toolkit/common/statistics/SolidTextureStatistics.h"

class Statistics {
  private:
    long numberOfPixels;
    long numberOfRays;
    long numberOfPixelsSupersampled;
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
    long rayHtFieldBoxTests;
    long rayHFieldBoxTestsSucceeded;
    long boundingRegionTests;
    long boundingRegionTestsSucceeded;
    long clippingRegionTests;
    long clippingRegionTestsSucceeded;
    SolidTextureStatistics solidTextureStatistics;
    long shadowRayTests;
    long shadowRaysSucceeded;
    long reflectedRaysTraced;
    long refractedRaysTraced;
    long transmittedRaysTraced;
    time_t startTime;
    time_t stopTime;
    double usedTime;

  public:
    static inline Statistics &global() {
        static Statistics instance;
        return instance;
    }
    long getNumberOfPixels() const;
    void setNumberOfPixels(long value);
    void incrementNumberOfPixels();
    long getNumberOfRays() const;
    void setNumberOfRays(long value);
    void incrementNumberOfRays();
    long getNumberOfPixelsSupersampled() const;
    void setNumberOfPixelsSupersampled(long value);
    void incrementNumberOfPixelsSupersampled();
    long getRaySphereTests() const;
    void setRaySphereTests(long value);
    void incrementRaySphereTests();
    long getRaySphereTestsSucceeded() const;
    void setRaySphereTestsSucceeded(long value);
    void incrementRaySphereTestsSucceeded();
    long getRayBoxTests() const;
    void setRayBoxTests(long value);
    void incrementRayBoxTests();
    long getRayBoxTestsSucceeded() const;
    void setRayBoxTestsSucceeded(long value);
    void incrementRayBoxTestsSucceeded();
    long getRayBlobTests() const;
    void setRayBlobTests(long value);
    void incrementRayBlobTests();
    long getRayBlobTestsSucceeded() const;
    void setRayBlobTestsSucceeded(long value);
    void incrementRayBlobTestsSucceeded();
    long getRayPlaneTests() const;
    void setRayPlaneTests(long value);
    void incrementRayPlaneTests();
    long getRayPlaneTestsSucceeded() const;
    void setRayPlaneTestsSucceeded(long value);
    void incrementRayPlaneTestsSucceeded();
    long getRayTriangleTests() const;
    void setRayTriangleTests(long value);
    void incrementRayTriangleTests();
    long getRayTriangleTestsSucceeded() const;
    void setRayTriangleTestsSucceeded(long value);
    void incrementRayTriangleTestsSucceeded();
    long getRayQuadricTests() const;
    void setRayQuadricTests(long value);
    void incrementRayQuadricTests();
    long getRayQuadricTestsSucceeded() const;
    void setRayQuadricTestsSucceeded(long value);
    void incrementRayQuadricTestsSucceeded();
    long getRayPolyTests() const;
    void setRayPolyTests(long value);
    void incrementRayPolyTests();
    long getRayPolyTestsSucceeded() const;
    void setRayPolyTestsSucceeded(long value);
    void incrementRayPolyTestsSucceeded();
    long getRayBicubicTests() const;
    void setRayBicubicTests(long value);
    void incrementRayBicubicTests();
    long getRayBicubicTestsSucceeded() const;
    void setRayBicubicTestsSucceeded(long value);
    void incrementRayBicubicTestsSucceeded();
    long getRayHtFieldTests() const;
    void setRayHtFieldTests(long value);
    void incrementRayHtFieldTests();
    long getRayHtFieldTestsSucceeded() const;
    void setRayHtFieldTestsSucceeded(long value);
    void incrementRayHtFieldTestsSucceeded();
    long getRayHtFieldBoxTests() const;
    void setRayHtFieldBoxTests(long value);
    void incrementRayHtFieldBoxTests();
    long getRayHFieldBoxTestsSucceeded() const;
    void setRayHFieldBoxTestsSucceeded(long value);
    void incrementRayHFieldBoxTestsSucceeded();
    long getBoundingRegionTests() const;
    void setBoundingRegionTests(long value);
    void incrementBoundingRegionTests();
    long getBoundingRegionTestsSucceeded() const;
    void setBoundingRegionTestsSucceeded(long value);
    void incrementBoundingRegionTestsSucceeded();
    long getClippingRegionTests() const;
    void setClippingRegionTests(long value);
    void incrementClippingRegionTests();
    long getClippingRegionTestsSucceeded() const;
    void setClippingRegionTestsSucceeded(long value);
    void incrementClippingRegionTestsSucceeded();
    SolidTextureStatistics *getSolidTextureStatistics();
    const SolidTextureStatistics *getSolidTextureStatistics() const;
    long getShadowRayTests() const;
    void setShadowRayTests(long value);
    void incrementShadowRayTests();
    long getShadowRaysSucceeded() const;
    void setShadowRaysSucceeded(long value);
    void incrementShadowRaysSucceeded();
    long getReflectedRaysTraced() const;
    void setReflectedRaysTraced(long value);
    void incrementReflectedRaysTraced();
    long getRefractedRaysTraced() const;
    void setRefractedRaysTraced(long value);
    void incrementRefractedRaysTraced();
    long getTransmittedRaysTraced() const;
    void setTransmittedRaysTraced(long value);
    void incrementTransmittedRaysTraced();
    time_t getStartTime() const;
    void setStartTime(time_t value);
    time_t getStopTime() const;
    void setStopTime(time_t value);
    double getUsedTime() const;
    void setUsedTime(double value);
    void reset();
    void startTimer();
    void stopTimer();
};

inline long Statistics::getNumberOfPixels() const { return numberOfPixels; }
inline void Statistics::setNumberOfPixels(long value) { numberOfPixels = value; }
inline void Statistics::incrementNumberOfPixels() { ++numberOfPixels; }
inline long Statistics::getNumberOfRays() const { return numberOfRays; }
inline void Statistics::setNumberOfRays(long value) { numberOfRays = value; }
inline void Statistics::incrementNumberOfRays() { ++numberOfRays; }
inline long Statistics::getNumberOfPixelsSupersampled() const { return numberOfPixelsSupersampled; }
inline void Statistics::setNumberOfPixelsSupersampled(long value) { numberOfPixelsSupersampled = value; }
inline void Statistics::incrementNumberOfPixelsSupersampled() { ++numberOfPixelsSupersampled; }
inline long Statistics::getRaySphereTests() const { return raySphereTests; }
inline void Statistics::setRaySphereTests(long value) { raySphereTests = value; }
inline void Statistics::incrementRaySphereTests() { ++raySphereTests; }
inline long Statistics::getRaySphereTestsSucceeded() const { return raySphereTestsSucceeded; }
inline void Statistics::setRaySphereTestsSucceeded(long value) { raySphereTestsSucceeded = value; }
inline void Statistics::incrementRaySphereTestsSucceeded() { ++raySphereTestsSucceeded; }
inline long Statistics::getRayBoxTests() const { return rayBoxTests; }
inline void Statistics::setRayBoxTests(long value) { rayBoxTests = value; }
inline void Statistics::incrementRayBoxTests() { ++rayBoxTests; }
inline long Statistics::getRayBoxTestsSucceeded() const { return rayBoxTestsSucceeded; }
inline void Statistics::setRayBoxTestsSucceeded(long value) { rayBoxTestsSucceeded = value; }
inline void Statistics::incrementRayBoxTestsSucceeded() { ++rayBoxTestsSucceeded; }
inline long Statistics::getRayBlobTests() const { return rayBlobTests; }
inline void Statistics::setRayBlobTests(long value) { rayBlobTests = value; }
inline void Statistics::incrementRayBlobTests() { ++rayBlobTests; }
inline long Statistics::getRayBlobTestsSucceeded() const { return rayBlobTestsSucceeded; }
inline void Statistics::setRayBlobTestsSucceeded(long value) { rayBlobTestsSucceeded = value; }
inline void Statistics::incrementRayBlobTestsSucceeded() { ++rayBlobTestsSucceeded; }
inline long Statistics::getRayPlaneTests() const { return rayPlaneTests; }
inline void Statistics::setRayPlaneTests(long value) { rayPlaneTests = value; }
inline void Statistics::incrementRayPlaneTests() { ++rayPlaneTests; }
inline long Statistics::getRayPlaneTestsSucceeded() const { return rayPlaneTestsSucceeded; }
inline void Statistics::setRayPlaneTestsSucceeded(long value) { rayPlaneTestsSucceeded = value; }
inline void Statistics::incrementRayPlaneTestsSucceeded() { ++rayPlaneTestsSucceeded; }
inline long Statistics::getRayTriangleTests() const { return rayTriangleTests; }
inline void Statistics::setRayTriangleTests(long value) { rayTriangleTests = value; }
inline void Statistics::incrementRayTriangleTests() { ++rayTriangleTests; }
inline long Statistics::getRayTriangleTestsSucceeded() const { return rayTriangleTestsSucceeded; }
inline void Statistics::setRayTriangleTestsSucceeded(long value) { rayTriangleTestsSucceeded = value; }
inline void Statistics::incrementRayTriangleTestsSucceeded() { ++rayTriangleTestsSucceeded; }
inline long Statistics::getRayQuadricTests() const { return rayQuadricTests; }
inline void Statistics::setRayQuadricTests(long value) { rayQuadricTests = value; }
inline void Statistics::incrementRayQuadricTests() { ++rayQuadricTests; }
inline long Statistics::getRayQuadricTestsSucceeded() const { return rayQuadricTestsSucceeded; }
inline void Statistics::setRayQuadricTestsSucceeded(long value) { rayQuadricTestsSucceeded = value; }
inline void Statistics::incrementRayQuadricTestsSucceeded() { ++rayQuadricTestsSucceeded; }
inline long Statistics::getRayPolyTests() const { return rayPolyTests; }
inline void Statistics::setRayPolyTests(long value) { rayPolyTests = value; }
inline void Statistics::incrementRayPolyTests() { ++rayPolyTests; }
inline long Statistics::getRayPolyTestsSucceeded() const { return rayPolyTestsSucceeded; }
inline void Statistics::setRayPolyTestsSucceeded(long value) { rayPolyTestsSucceeded = value; }
inline void Statistics::incrementRayPolyTestsSucceeded() { ++rayPolyTestsSucceeded; }
inline long Statistics::getRayBicubicTests() const { return rayBicubicTests; }
inline void Statistics::setRayBicubicTests(long value) { rayBicubicTests = value; }
inline void Statistics::incrementRayBicubicTests() { ++rayBicubicTests; }
inline long Statistics::getRayBicubicTestsSucceeded() const { return rayBicubicTestsSucceeded; }
inline void Statistics::setRayBicubicTestsSucceeded(long value) { rayBicubicTestsSucceeded = value; }
inline void Statistics::incrementRayBicubicTestsSucceeded() { ++rayBicubicTestsSucceeded; }
inline long Statistics::getRayHtFieldTests() const { return rayHtFieldTests; }
inline void Statistics::setRayHtFieldTests(long value) { rayHtFieldTests = value; }
inline void Statistics::incrementRayHtFieldTests() { ++rayHtFieldTests; }
inline long Statistics::getRayHtFieldTestsSucceeded() const { return rayHtFieldTestsSucceeded; }
inline void Statistics::setRayHtFieldTestsSucceeded(long value) { rayHtFieldTestsSucceeded = value; }
inline void Statistics::incrementRayHtFieldTestsSucceeded() { ++rayHtFieldTestsSucceeded; }
inline long Statistics::getRayHtFieldBoxTests() const { return rayHtFieldBoxTests; }
inline void Statistics::setRayHtFieldBoxTests(long value) { rayHtFieldBoxTests = value; }
inline void Statistics::incrementRayHtFieldBoxTests() { ++rayHtFieldBoxTests; }
inline long Statistics::getRayHFieldBoxTestsSucceeded() const { return rayHFieldBoxTestsSucceeded; }
inline void Statistics::setRayHFieldBoxTestsSucceeded(long value) { rayHFieldBoxTestsSucceeded = value; }
inline void Statistics::incrementRayHFieldBoxTestsSucceeded() { ++rayHFieldBoxTestsSucceeded; }
inline long Statistics::getBoundingRegionTests() const { return boundingRegionTests; }
inline void Statistics::setBoundingRegionTests(long value) { boundingRegionTests = value; }
inline void Statistics::incrementBoundingRegionTests() { ++boundingRegionTests; }
inline long Statistics::getBoundingRegionTestsSucceeded() const { return boundingRegionTestsSucceeded; }
inline void Statistics::setBoundingRegionTestsSucceeded(long value) { boundingRegionTestsSucceeded = value; }
inline void Statistics::incrementBoundingRegionTestsSucceeded() { ++boundingRegionTestsSucceeded; }
inline long Statistics::getClippingRegionTests() const { return clippingRegionTests; }
inline void Statistics::setClippingRegionTests(long value) { clippingRegionTests = value; }
inline void Statistics::incrementClippingRegionTests() { ++clippingRegionTests; }
inline long Statistics::getClippingRegionTestsSucceeded() const { return clippingRegionTestsSucceeded; }
inline void Statistics::setClippingRegionTestsSucceeded(long value) { clippingRegionTestsSucceeded = value; }
inline void Statistics::incrementClippingRegionTestsSucceeded() { ++clippingRegionTestsSucceeded; }
inline const SolidTextureStatistics *Statistics::getSolidTextureStatistics() const { return &solidTextureStatistics; }
inline long Statistics::getShadowRayTests() const { return shadowRayTests; }
inline void Statistics::setShadowRayTests(long value) { shadowRayTests = value; }
inline void Statistics::incrementShadowRayTests() { ++shadowRayTests; }
inline long Statistics::getShadowRaysSucceeded() const { return shadowRaysSucceeded; }
inline void Statistics::setShadowRaysSucceeded(long value) { shadowRaysSucceeded = value; }
inline void Statistics::incrementShadowRaysSucceeded() { ++shadowRaysSucceeded; }
inline long Statistics::getReflectedRaysTraced() const { return reflectedRaysTraced; }
inline void Statistics::setReflectedRaysTraced(long value) { reflectedRaysTraced = value; }
inline void Statistics::incrementReflectedRaysTraced() { ++reflectedRaysTraced; }
inline long Statistics::getRefractedRaysTraced() const { return refractedRaysTraced; }
inline void Statistics::setRefractedRaysTraced(long value) { refractedRaysTraced = value; }
inline void Statistics::incrementRefractedRaysTraced() { ++refractedRaysTraced; }
inline long Statistics::getTransmittedRaysTraced() const { return transmittedRaysTraced; }
inline void Statistics::setTransmittedRaysTraced(long value) { transmittedRaysTraced = value; }
inline void Statistics::incrementTransmittedRaysTraced() { ++transmittedRaysTraced; }
inline time_t Statistics::getStartTime() const { return startTime; }
inline void Statistics::setStartTime(time_t value) { startTime = value; }
inline time_t Statistics::getStopTime() const { return stopTime; }
inline void Statistics::setStopTime(time_t value) { stopTime = value; }
inline double Statistics::getUsedTime() const { return usedTime; }
inline void Statistics::setUsedTime(double value) { usedTime = value; }

#endif
