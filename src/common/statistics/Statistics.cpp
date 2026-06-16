#include <ctime>

#include "common/statistics/Statistics.h"

SolidTextureStatistics*
Statistics::getSolidTextureStatistics()
{
    return &solidTextureStatistics;
}

void
Statistics::reset()
{
    numberOfPixels = 0L;
    numberOfRays = 0L;
    numberOfPixelsSupersampled = 0L;
    raySphereTests = 0L;
    raySphereTestsSucceeded = 0L;
    rayBoxTests = 0L;
    rayBoxTestsSucceeded = 0L;
    rayBlobTests = 0L;
    rayBlobTestsSucceeded = 0L;
    rayPlaneTests = 0L;
    rayPlaneTestsSucceeded = 0L;
    rayTriangleTests = 0L;
    rayTriangleTestsSucceeded = 0L;
    rayQuadricTests = 0L;
    rayQuadricTestsSucceeded = 0L;
    rayPolyTests = 0L;
    rayPolyTestsSucceeded = 0L;
    rayBicubicTests = 0L;
    rayBicubicTestsSucceeded = 0L;
    rayHtFieldTests = 0L;
    rayHtFieldTestsSucceeded = 0L;
    rayHtFieldBoxTests = 0L;
    rayHFieldBoxTestsSucceeded = 0L;
    boundingRegionTests = 0L;
    boundingRegionTestsSucceeded = 0L;
    clippingRegionTests = 0L;
    clippingRegionTestsSucceeded = 0L;
    solidTextureStatistics.reset();
    shadowRayTests = 0L;
    shadowRaysSucceeded = 0L;
    reflectedRaysTraced = 0L;
    refractedRaysTraced = 0L;
    transmittedRaysTraced = 0L;
    startTime = 0;
    stopTime = 0;
    usedTime = 0.0;
}

void
Statistics::startTimer()
{
    std::time(&startTime);
}

void
Statistics::stopTimer()
{
    std::time(&stopTime);
    usedTime = std::difftime(stopTime, startTime);
}
