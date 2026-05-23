#include "common/Statistics.h"

#include "environment/scene/SceneFrame.h"
#include "environment/material/RendererConfiguration.h"

#include <cstdio>

Statistics globalStatistics;

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
    callsToNoise = 0L;
    callsToDNoise = 0L;
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

void
Statistics::print(
    const RenderFrame &frame, const RenderingConfiguration &configuration)
{
    FILE *statOut = stdout;
    if (configuration.options & VERBOSE_FILE) {
        statOut = fopen(configuration.statFileName, "w+t");
    }

    const long pixelsInImage =
        (long)frame.Screen_Width * (long)frame.Screen_Height;

    fprintf(statOut, "\n%s statistics\n", configuration.inputFileName);
    if (pixelsInImage > numberOfPixels) {
        fprintf(statOut, "  Partial Image Rendered");
    }

    fprintf(statOut, "--------------------------------------\n");
    fprintf(statOut, "Resolution %d x %d\n", frame.Screen_Width,
        frame.Screen_Height);
    fprintf(statOut,
        "# Rays:  %10ld     # Pixels:  %10ld  # Pixels supersampled: %10ld\n",
        numberOfRays, numberOfPixels, numberOfPixelsSupersampled);

    fprintf(statOut, "  Ray->Shape Intersection Tests:\n");
    fprintf(statOut,
        "    Type                 Tests     Succeeded    Percentage\n");
    fprintf(statOut,
        "  -----------------------------------------------------------\n");
#define PRINT_INTERSECTION_ROW(label, tests, succeeded)                         \
    if (tests) {                                                                 \
        fprintf(statOut, label " %10ld  %10ld  %10.2f\n", tests, succeeded,    \
            (((double)succeeded / (double)tests) * 100.0));                     \
    }
    PRINT_INTERSECTION_ROW("  Sphere        ", raySphereTests, raySphereTestsSucceeded);
    PRINT_INTERSECTION_ROW("  Plane         ", rayPlaneTests, rayPlaneTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Triangle     ", rayTriangleTests, rayTriangleTestsSucceeded);
    PRINT_INTERSECTION_ROW("  Quadric       ", rayQuadricTests, rayQuadricTestsSucceeded);
    PRINT_INTERSECTION_ROW("  Blob          ", rayBlobTests, rayBlobTestsSucceeded);
    PRINT_INTERSECTION_ROW("  Box           ", rayBoxTests, rayBoxTestsSucceeded);
    PRINT_INTERSECTION_ROW("  Quartic\\Poly ", rayPolyTests, rayPolyTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Bezier Patch ", rayBicubicTests, rayBicubicTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Height Fld   ", rayHtFieldTests, rayHtFieldTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Hght Fld Box ", rayHtFieldBoxTests, rayHFieldBoxTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Bounds        ", boundingRegionTests, boundingRegionTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Clips         ", clippingRegionTests, clippingRegionTestsSucceeded);
#undef PRINT_INTERSECTION_ROW

    if (callsToNoise) {
        fprintf(statOut, "  Calls to Noise:    %10ld\n", callsToNoise);
    }
    if (callsToDNoise) {
        fprintf(statOut, "  Calls to DNoise:  %10ld\n", callsToDNoise);
    }
    if (shadowRayTests) {
        fprintf(statOut,
            "  Shadow Ray Tests: %10ld      Blocking Objects Found:  %10ld\n",
            shadowRayTests, shadowRaysSucceeded);
    }
    if (reflectedRaysTraced) {
        fprintf(statOut, "  Reflected Rays:    %10ld\n", reflectedRaysTraced);
    }
    if (refractedRaysTraced) {
        fprintf(statOut, "  Refracted Rays:    %10ld\n", refractedRaysTraced);
    }
    if (transmittedRaysTraced) {
        fprintf(statOut, "  Transmitted Rays: %10ld\n", transmittedRaysTraced);
    }

    if (usedTime == 0.0) {
        stopTimer();
    }
    if (usedTime != 0.0) {
        const int hours = (int)usedTime / 3600;
        const int minutes = (int)(usedTime - hours * 3600) / 60;
        const double seconds = usedTime - (double)(hours * 3600 + minutes * 60);
        fprintf(statOut,
            "  Time For Trace:    %2d hours %2d minutes %4.2f seconds\n", hours,
            minutes, seconds);
    }
    if (configuration.options & VERBOSE_FILE) {
        fclose(statOut);
    }
}
