#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/io/geometry/ReaderPlyResult.h"

ReaderPlyResult::ReaderPlyResult()
    : triangleMesh(0), pointCloud()
{
}

ReaderPlyResult::~ReaderPlyResult()
{
    if ( triangleMesh != 0 ) {
        delete triangleMesh;
        triangleMesh = 0;
    }
}

TriangleMesh* ReaderPlyResult::detachTriangleMesh()
{
    TriangleMesh* out = triangleMesh;
    triangleMesh = 0;
    return out;
}
