#ifndef __STATISTICS_H__
#define __STATISTICS_H__

class Statistics {
  public:
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
    long callsToNoise;
    long callsToDNoise;
    long shadowRayTests;
    long shadowRaysSucceeded;
    long reflectedRaysTraced;
    long refractedRaysTraced;
    long transmittedRaysTraced;
    time_t startTime;
    time_t stopTime;
    double usedTime;

    static Statistics &global();
    void reset();
    void startTimer();
    void stopTimer();
};

#endif
