#include "java/util/ArrayList.txx"
#include "common/statistics/GeometryStatistics.h"

// Mirrors Statistics(ArrayList<Statistics*>*): supports multi-thread ray
// tracing, where GeometryStatistics parts are totaled after all worker
// threads have been joined.
GeometryStatistics::GeometryStatistics(java::ArrayList<GeometryStatistics*> *partsPerThread)
{
    reset();

    if (partsPerThread == nullptr) {
        return;
    }

    for (long i = 0; i < partsPerThread->size(); ++i) {
        const GeometryStatistics *part = partsPerThread->get(i);
        if (part == nullptr) {
            continue;
        }

        raySphereTests += part->raySphereTests;
        raySphereTestsSucceeded += part->raySphereTestsSucceeded;
        rayBoxTests += part->rayBoxTests;
        rayBoxTestsSucceeded += part->rayBoxTestsSucceeded;
        rayBlobTests += part->rayBlobTests;
        rayBlobTestsSucceeded += part->rayBlobTestsSucceeded;
        rayPlaneTests += part->rayPlaneTests;
        rayPlaneTestsSucceeded += part->rayPlaneTestsSucceeded;
        rayTriangleTests += part->rayTriangleTests;
        rayTriangleTestsSucceeded += part->rayTriangleTestsSucceeded;
        rayQuadricTests += part->rayQuadricTests;
        rayQuadricTestsSucceeded += part->rayQuadricTestsSucceeded;
        rayPolyTests += part->rayPolyTests;
        rayPolyTestsSucceeded += part->rayPolyTestsSucceeded;
        rayBicubicTests += part->rayBicubicTests;
        rayBicubicTestsSucceeded += part->rayBicubicTestsSucceeded;
        rayHtFieldTests += part->rayHtFieldTests;
        rayHtFieldTestsSucceeded += part->rayHtFieldTestsSucceeded;
        boundingRegionTests += part->boundingRegionTests;
        boundingRegionTestsSucceeded += part->boundingRegionTestsSucceeded;
        clippingRegionTests += part->clippingRegionTests;
        clippingRegionTestsSucceeded += part->clippingRegionTestsSucceeded;
    }
}

void
GeometryStatistics::reset()
{
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
    boundingRegionTests = 0L;
    boundingRegionTestsSucceeded = 0L;
    clippingRegionTests = 0L;
    clippingRegionTestsSucceeded = 0L;
}
