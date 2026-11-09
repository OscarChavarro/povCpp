#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/_Polygon2DContour.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/MonotoneDecompositionTriangulator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_ContourAwarePolygonTriangulator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_RandomSegmentOrder.h"
void MonotoneDecompositionTriangulator::stage1PrepareAndOrder(
    const Polygon2D &input, int &numVertices) {
    if (input.loops.size() <= 0) {
        Logger::reportMessage("MonotoneDecompositionTriangulator", Logger::ERROR, "stage1PrepareAndOrder",
            "Polygon input must contain at least one contour");
        throw VSDKFatalException("Polygon input must contain at least one contour");
    }
    java::ArrayList<int> contourSizes;
    java::ArrayList<double> vertices;
    Polygon2D &mutableInput = const_cast<Polygon2D &>(input);
    for (int contourIndex = 0; contourIndex < mutableInput.loops.size();
         contourIndex++) {
        _Polygon2DContour *contour = mutableInput.loops[contourIndex];
        const int pointCount = static_cast<int>(contour->vertices.size());
        if (pointCount <= 0) {
            continue;
        }
        contourSizes.add(pointCount);
        for (int pointIndex = 0; pointIndex < pointCount; pointIndex++) {
            vertices.add(contour->vertices[pointIndex].x);
            vertices.add(contour->vertices[pointIndex].y);
        }
    }

    if (contourSizes.size() <= 0 || vertices.size() <= 0) {
        Logger::reportMessage("MonotoneDecompositionTriangulator", Logger::ERROR, "stage1PrepareAndOrder",
            "Polygon input must contain at least one vertex");
        throw VSDKFatalException("Polygon input must contain at least one vertex");
    }

    numVertices = _SegmentTableBuilder::prepareSegments(
        vertices.data(), static_cast<int>(vertices.size() / 2),
        contourSizes.data(), static_cast<int>(contourSizes.size()));
}

void MonotoneDecompositionTriangulator::stage2BootStrap(int numVertices) {
    // [SEID1991].3 Initialize randomized incremental insertion status.
    // Stage-2 in Seidel (1991): initialize randomized insertion status.
    // Reset insertion flags and generate the randomized segment ordering used
    // by Stage-3 insertion.
    for (int i = 1; i <= numVertices; ++i) {
        _Construct::segmentAt(i).hasBeenInserted = false;
    }
    _RandomSegmentOrder::generateRandomOrdering(numVertices);
}

void MonotoneDecompositionTriangulator::stage3IncrementalBatchedInsertion(
    int numVertices) {
    // [SEID1991].3 Execute batched randomized segment insertion into T(S).
    buildTrapezoids(numVertices);
}

void MonotoneDecompositionTriangulator::buildTrapezoids(int numVertices) {
    _Construct::constructTrapezoids(numVertices);
}

int MonotoneDecompositionTriangulator::stage4FinalizeAndExtractTriangles(
    int numVertices, java::ArrayList<Triangle> &out) {
    // [SEID1991].2/.3 Convert trapezoid decomposition into monotone triangles.
    int op[SEGMENT_SIZE][3];
    const int nmonpoly = _Monotone::monotonateTrapezoids(numVertices);
    const int ntriangles =
        _Monotone::triangulateMonotonePolygons(numVertices, nmonpoly, op);

    out.clear();
    for (int i = 0; i < ntriangles; ++i) {
        Triangle triangle;
        triangle.add(op[i][0] - 1);
        triangle.add(op[i][1] - 1);
        triangle.add(op[i][2] - 1);
        out.add(triangle);
    }

    return ntriangles;
}

void MonotoneDecompositionTriangulator::triangulate(
    const Polygon2D &input, java::ArrayList<Triangle> &triangles,
    int &triangleCount) {
    const int contourAwareTriangles =
        _ContourAwarePolygonTriangulator::triangulate(input, triangles);
    if (contourAwareTriangles >= 0) {
        triangleCount = contourAwareTriangles;
        return;
    }

    int numVertices = 0;

    stage1PrepareAndOrder(input, numVertices);
    stage2BootStrap(numVertices);
    stage3IncrementalBatchedInsertion(numVertices);
    triangleCount = stage4FinalizeAndExtractTriangles(numVertices, triangles);
}
