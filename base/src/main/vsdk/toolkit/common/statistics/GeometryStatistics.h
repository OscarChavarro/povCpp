#ifndef __GEOMETRYSTATISTICS__
#define __GEOMETRYSTATISTICS__

#include "java/util/ArrayList.h"

class GeometryStatistics {
  private:
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

  public:
    GeometryStatistics() { reset(); }
    explicit GeometryStatistics(java::ArrayList<GeometryStatistics*> *partsPerThread);
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
    void reset();
};

inline long GeometryStatistics::getRaySphereTests() const { return raySphereTests; }
inline void GeometryStatistics::incrementRaySphereTests() { ++raySphereTests; }
inline long GeometryStatistics::getRaySphereTestsSucceeded() const { return raySphereTestsSucceeded; }
inline void GeometryStatistics::incrementRaySphereTestsSucceeded() { ++raySphereTestsSucceeded; }
inline long GeometryStatistics::getRayBoxTests() const { return rayBoxTests; }
inline void GeometryStatistics::incrementRayBoxTests() { ++rayBoxTests; }
inline long GeometryStatistics::getRayBoxTestsSucceeded() const { return rayBoxTestsSucceeded; }
inline void GeometryStatistics::incrementRayBoxTestsSucceeded() { ++rayBoxTestsSucceeded; }
inline long GeometryStatistics::getRayBlobTests() const { return rayBlobTests; }
inline void GeometryStatistics::incrementRayBlobTests() { ++rayBlobTests; }
inline long GeometryStatistics::getRayBlobTestsSucceeded() const { return rayBlobTestsSucceeded; }
inline void GeometryStatistics::incrementRayBlobTestsSucceeded() { ++rayBlobTestsSucceeded; }
inline long GeometryStatistics::getRayPlaneTests() const { return rayPlaneTests; }
inline void GeometryStatistics::incrementRayPlaneTests() { ++rayPlaneTests; }
inline long GeometryStatistics::getRayPlaneTestsSucceeded() const { return rayPlaneTestsSucceeded; }
inline void GeometryStatistics::incrementRayPlaneTestsSucceeded() { ++rayPlaneTestsSucceeded; }
inline long GeometryStatistics::getRayTriangleTests() const { return rayTriangleTests; }
inline void GeometryStatistics::incrementRayTriangleTests() { ++rayTriangleTests; }
inline long GeometryStatistics::getRayTriangleTestsSucceeded() const { return rayTriangleTestsSucceeded; }
inline void GeometryStatistics::incrementRayTriangleTestsSucceeded() { ++rayTriangleTestsSucceeded; }
inline long GeometryStatistics::getRayQuadricTests() const { return rayQuadricTests; }
inline void GeometryStatistics::incrementRayQuadricTests() { ++rayQuadricTests; }
inline long GeometryStatistics::getRayQuadricTestsSucceeded() const { return rayQuadricTestsSucceeded; }
inline void GeometryStatistics::incrementRayQuadricTestsSucceeded() { ++rayQuadricTestsSucceeded; }
inline long GeometryStatistics::getRayPolyTests() const { return rayPolyTests; }
inline void GeometryStatistics::incrementRayPolyTests() { ++rayPolyTests; }
inline long GeometryStatistics::getRayPolyTestsSucceeded() const { return rayPolyTestsSucceeded; }
inline void GeometryStatistics::incrementRayPolyTestsSucceeded() { ++rayPolyTestsSucceeded; }
inline long GeometryStatistics::getRayBicubicTests() const { return rayBicubicTests; }
inline void GeometryStatistics::incrementRayBicubicTests() { ++rayBicubicTests; }
inline long GeometryStatistics::getRayBicubicTestsSucceeded() const { return rayBicubicTestsSucceeded; }
inline void GeometryStatistics::incrementRayBicubicTestsSucceeded() { ++rayBicubicTestsSucceeded; }
inline long GeometryStatistics::getRayHtFieldTests() const { return rayHtFieldTests; }
inline void GeometryStatistics::incrementRayHtFieldTests() { ++rayHtFieldTests; }
inline long GeometryStatistics::getRayHtFieldTestsSucceeded() const { return rayHtFieldTestsSucceeded; }
inline void GeometryStatistics::incrementRayHtFieldTestsSucceeded() { ++rayHtFieldTestsSucceeded; }
inline long GeometryStatistics::getBoundingRegionTests() const { return boundingRegionTests; }
inline void GeometryStatistics::incrementBoundingRegionTests() { ++boundingRegionTests; }
inline long GeometryStatistics::getBoundingRegionTestsSucceeded() const { return boundingRegionTestsSucceeded; }
inline void GeometryStatistics::incrementBoundingRegionTestsSucceeded() { ++boundingRegionTestsSucceeded; }
inline long GeometryStatistics::getClippingRegionTests() const { return clippingRegionTests; }
inline void GeometryStatistics::incrementClippingRegionTests() { ++clippingRegionTests; }
inline long GeometryStatistics::getClippingRegionTestsSucceeded() const { return clippingRegionTestsSucceeded; }
inline void GeometryStatistics::incrementClippingRegionTestsSucceeded() { ++clippingRegionTestsSucceeded; }

#endif
