#include "java/lang/Double.h"
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Vertex2D.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/_Polygon2DContour.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonTriangulation/monotoneDecomposition/_ContourAwarePolygonTriangulator.h"
const double EPSILON = 1.0e-9;

template <typename T>
static const T& minValue(const T& a, const T& b)
{
    return (b < a) ? b : a;
}

template <typename T>
static const T& maxValue(const T& a, const T& b)
{
    return (a < b) ? b : a;
}

template <typename T>
static void reverseArray(T* values, size_t count)
{
    if (count < 2) {
        return;
    }
    size_t left = 0;
    size_t right = count - 1;
    while (left < right) {
        T temp = values[left];
        values[left] = values[right];
        values[right] = temp;
        ++left;
        --right;
    }
}

struct IndexedVertex {
    double x;
    double y;
    int originalIndex;
};

struct ContourData {
    java::ArrayList<IndexedVertex> vertices;
    java::ArrayList<int> childContours;
    double signedArea = 0.0;
    int parentContour = -1;
    int depth = -1;
};

double signedArea(const java::ArrayList<IndexedVertex> &vertices)
{
    double area = 0.0;
    for (size_t i = 0; i < vertices.size(); i++) {
        const IndexedVertex &current = vertices[i];
        const IndexedVertex &next = vertices[(i + 1) % vertices.size()];
        area += current.x * next.y - next.x * current.y;
    }
    return area * 0.5;
}

bool samePoint(const IndexedVertex &a, const IndexedVertex &b)
{
    return std::abs(a.x - b.x) <= EPSILON && std::abs(a.y - b.y) <= EPSILON;
}

double cross(const IndexedVertex &a, const IndexedVertex &b,
    const IndexedVertex &c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

java::ArrayList<IndexedVertex> removeConsecutiveDuplicates(
    const java::ArrayList<IndexedVertex> &polygon)
{
    java::ArrayList<IndexedVertex> result;
    for (size_t i = 0; i < polygon.size(); i++) {
        if (result.size() == 0 || !samePoint(result[result.size() - 1], polygon[i])) {
            result.add(polygon[i]);
        }
    }
    if (result.size() > 1 && samePoint(result[0], result[result.size() - 1])) {
        result.remove(result.size() - 1);
    }
    return result;
}

void normalizeOrientation(java::ArrayList<IndexedVertex> &vertices,
    bool wantCounterClockwise)
{
    const double area = signedArea(vertices);
    if ((wantCounterClockwise && area < 0.0) ||
        (!wantCounterClockwise && area > 0.0)) {
        reverseArray(vertices.data(), vertices.size());
    }
}

bool pointOnSegment(double x, double y, const IndexedVertex &a,
    const IndexedVertex &b)
{
    const double areaTwice = (b.x - a.x) * (y - a.y) -
        (b.y - a.y) * (x - a.x);
    if (std::abs(areaTwice) > EPSILON) {
        return false;
    }

    return x >= minValue(a.x, b.x) - EPSILON &&
        x <= maxValue(a.x, b.x) + EPSILON &&
        y >= minValue(a.y, b.y) - EPSILON &&
        y <= maxValue(a.y, b.y) + EPSILON;
}

bool containsPoint(const java::ArrayList<IndexedVertex> &contour, double x, double y)
{
    bool inside = false;
    for (size_t i = 0, j = contour.size() - 1; i < contour.size(); j = i++) {
        const IndexedVertex &a = contour[i];
        const IndexedVertex &b = contour[j];

        if (pointOnSegment(x, y, a, b)) {
            return true;
        }

        const bool intersects = ((a.y > y) != (b.y > y)) &&
            (x < (b.x - a.x) * (y - a.y) / (b.y - a.y) + a.x);
        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

double squaredDistance(const IndexedVertex &a, const IndexedVertex &b)
{
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
}

bool rangesOverlap(double a0, double a1, double b0, double b1)
{
    const double minA = minValue(a0, a1);
    const double maxA = maxValue(a0, a1);
    const double minB = minValue(b0, b1);
    const double maxB = maxValue(b0, b1);
    return maxValue(minA, minB) <= minValue(maxA, maxB) + EPSILON;
}

bool segmentsIntersectProperly(const IndexedVertex &a, const IndexedVertex &b,
    const IndexedVertex &c, const IndexedVertex &d)
{
    if (samePoint(a, c) || samePoint(a, d) || samePoint(b, c) ||
        samePoint(b, d)) {
        return false;
    }

    const double abC = cross(a, b, c);
    const double abD = cross(a, b, d);
    const double cdA = cross(c, d, a);
    const double cdB = cross(c, d, b);

    if (std::abs(abC) <= EPSILON && std::abs(abD) <= EPSILON &&
        std::abs(cdA) <= EPSILON && std::abs(cdB) <= EPSILON) {
        return rangesOverlap(a.x, b.x, c.x, d.x) &&
            rangesOverlap(a.y, b.y, c.y, d.y);
    }

    return ((abC > EPSILON && abD < -EPSILON) ||
               (abC < -EPSILON && abD > EPSILON)) &&
        ((cdA > EPSILON && cdB < -EPSILON) ||
            (cdA < -EPSILON && cdB > EPSILON));
}

bool segmentCrossesAnyEdge(const IndexedVertex &a, const IndexedVertex &b,
    const java::ArrayList<IndexedVertex> &boundary, int allowedVertexIndex)
{
    for (size_t i = 0; i < boundary.size(); i++) {
        const IndexedVertex &edgeStart = boundary[i];
        const IndexedVertex &edgeEnd = boundary[(i + 1) % boundary.size()];

        if (allowedVertexIndex >= 0) {
            const IndexedVertex &allowed = boundary[allowedVertexIndex];
            if (edgeStart.originalIndex == allowed.originalIndex ||
                edgeEnd.originalIndex == allowed.originalIndex) {
                continue;
            }
        }

        if (segmentsIntersectProperly(a, b, edgeStart, edgeEnd)) {
            return true;
        }
    }
    return false;
}

bool segmentCrossesBoundary(const IndexedVertex &a, const IndexedVertex &b,
    const java::ArrayList<IndexedVertex> &polygon, int polygonVertexIndex,
    const java::ArrayList<IndexedVertex> &hole, int holeVertexIndex)
{
    if (segmentCrossesAnyEdge(a, b, polygon, polygonVertexIndex)) {
        return true;
    }
    return segmentCrossesAnyEdge(a, b, hole, holeVertexIndex);
}

int findRightmostVertexIndex(const java::ArrayList<IndexedVertex> &contour)
{
    int bestIndex = 0;
    IndexedVertex bestVertex = contour[0];
    for (size_t i = 1; i < contour.size(); i++) {
        const IndexedVertex &candidate = contour[i];
        if (candidate.x > bestVertex.x + EPSILON ||
            (std::abs(candidate.x - bestVertex.x) <= EPSILON &&
                candidate.y < bestVertex.y)) {
            bestVertex = candidate;
            bestIndex = static_cast<int>(i);
        }
    }
    return bestIndex;
}

int computeDepth(java::ArrayList<ContourData> &contours, int contourIndex)
{
    ContourData &contour = contours[contourIndex];
    if (contour.depth >= 0) {
        return contour.depth;
    }
    if (contour.parentContour < 0) {
        contour.depth = 1;
    }
    else {
        contour.depth = computeDepth(contours, contour.parentContour) + 1;
    }
    return contour.depth;
}

java::ArrayList<ContourData> flattenContours(const Polygon2D &input)
{
    java::ArrayList<ContourData> contours;
    int flattenedIndex = 0;
    Polygon2D &mutableInput = const_cast<Polygon2D &>(input);

    for (long contourIndex = 0; contourIndex < mutableInput.loops.size();
         contourIndex++) {
        _Polygon2DContour *loop = mutableInput.loops.get(contourIndex);
        if (loop->vertices.size() <= 0) {
            continue;
        }

        ContourData contour;
        for (long vertexIndex = 0; vertexIndex < loop->vertices.size();
             vertexIndex++) {
            const Vertex2D &vertex = loop->vertices[vertexIndex];
            IndexedVertex indexedVertex;
            indexedVertex.x = vertex.x;
            indexedVertex.y = vertex.y;
            indexedVertex.originalIndex = flattenedIndex++;
            contour.vertices.add(indexedVertex);
        }
        contour.signedArea = signedArea(contour.vertices);
        if (std::abs(contour.signedArea) > EPSILON) {
            contours.add(contour);
        }
    }

    return contours;
}

void classifyContourHierarchy(java::ArrayList<ContourData> &contours)
{
    for (size_t i = 0; i < contours.size(); i++) {
        ContourData &contour = contours[i];
        const IndexedVertex &probe = contour.vertices[0];
        double bestContainerArea = java::Double::POSITIVE_INFINITY;

        for (size_t j = 0; j < contours.size(); j++) {
            if (i == j) {
                continue;
            }

            ContourData &candidate = contours[j];
            const double candidateAbsArea = std::abs(candidate.signedArea);
            if (candidateAbsArea <= std::abs(contour.signedArea) + EPSILON) {
                continue;
            }

            if (containsPoint(candidate.vertices, probe.x, probe.y) &&
                candidateAbsArea < bestContainerArea) {
                bestContainerArea = candidateAbsArea;
                contour.parentContour = static_cast<int>(j);
            }
        }
    }

    for (size_t i = 0; i < contours.size(); i++) {
        computeDepth(contours, static_cast<int>(i));
    }

    for (size_t i = 0; i < contours.size(); i++) {
        const int parentIndex = contours[i].parentContour;
        if (parentIndex >= 0) {
            contours[parentIndex].childContours.add(static_cast<int>(i));
        }
    }
}

bool hasRepeatedVertex(const IndexedVertex &a, const IndexedVertex &b,
    const IndexedVertex &c)
{
    return a.originalIndex == b.originalIndex ||
        b.originalIndex == c.originalIndex ||
        a.originalIndex == c.originalIndex;
}

bool pointInTriangle(const IndexedVertex &a, const IndexedVertex &b,
    const IndexedVertex &c, const IndexedVertex &p)
{
    if (samePoint(a, p) || samePoint(b, p) || samePoint(c, p)) {
        return false;
    }

    const double ab = cross(a, b, p);
    const double bc = cross(b, c, p);
    const double ca = cross(c, a, p);
    const bool hasNegative = ab < -EPSILON || bc < -EPSILON || ca < -EPSILON;
    const bool hasPositive = ab > EPSILON || bc > EPSILON || ca > EPSILON;
    return !(hasNegative && hasPositive);
}

void simplifyCollinearVertices(java::ArrayList<IndexedVertex> &polygon)
{
    bool removed;
    do {
        removed = false;
        if (polygon.size() <= 3) {
            return;
        }
        for (size_t i = 0; i < polygon.size(); i++) {
            const IndexedVertex &previous =
                polygon[(i + polygon.size() - 1) % polygon.size()];
            const IndexedVertex &current = polygon[i];
            const IndexedVertex &next = polygon[(i + 1) % polygon.size()];
            if (samePoint(previous, current) || samePoint(current, next) ||
                std::abs(cross(previous, current, next)) <= EPSILON) {
                polygon.remove(static_cast<long>(i));
                removed = true;
                break;
            }
        }
    }
    while (removed);
}

bool earClipSimplePolygon(const java::ArrayList<IndexedVertex> &polygon,
    java::ArrayList<MonotoneDecompositionTriangulator::Triangle> &output)
{
    java::ArrayList<IndexedVertex> work = removeConsecutiveDuplicates(polygon);
    simplifyCollinearVertices(work);
    if (work.size() < 3) {
        return true;
    }

    if (signedArea(work) < 0.0) {
        reverseArray(work.data(), work.size());
    }

    int guard = static_cast<int>(work.size() * work.size());
    while (work.size() > 3 && guard-- > 0) {
        bool clippedEar = false;

        for (size_t i = 0; i < work.size(); i++) {
            const IndexedVertex &previous =
                work[(i + work.size() - 1) % work.size()];
            const IndexedVertex &current = work[i];
            const IndexedVertex &next = work[(i + 1) % work.size()];

            if (cross(previous, current, next) <= EPSILON) {
                continue;
            }
            if (hasRepeatedVertex(previous, current, next)) {
                continue;
            }

            bool containsOtherVertex = false;
            for (size_t j = 0; j < work.size(); j++) {
                if (j == i || j == (i + 1) % work.size() ||
                    j == (i + work.size() - 1) % work.size()) {
                    continue;
                }
                if (pointInTriangle(previous, current, next, work[j])) {
                    containsOtherVertex = true;
                    break;
                }
            }
            if (containsOtherVertex) {
                continue;
            }

            MonotoneDecompositionTriangulator::Triangle triangle;
            triangle.add(previous.originalIndex);
            triangle.add(current.originalIndex);
            triangle.add(next.originalIndex);
            output.add(triangle);

            work.remove(static_cast<long>(i));
            simplifyCollinearVertices(work);
            clippedEar = true;
            break;
        }

        if (!clippedEar) {
            return false;
        }
    }

    if (work.size() == 3 &&
        !hasRepeatedVertex(work[0], work[1], work[2]) &&
        std::abs(cross(work[0], work[1], work[2])) > EPSILON) {
        MonotoneDecompositionTriangulator::Triangle triangle;
        triangle.add(work[0].originalIndex);
        triangle.add(work[1].originalIndex);
        triangle.add(work[2].originalIndex);
        output.add(triangle);
        return true;
    }

    return work.size() == 0 || work.size() < 3;
}

int findRightmostHoleIndex(
    const java::ArrayList< java::ArrayList<IndexedVertex> > &holes)
{
    int bestHoleIndex = 0;
    IndexedVertex bestVertex = holes[0][findRightmostVertexIndex(holes[0])];

    for (size_t holeIndex = 1; holeIndex < holes.size(); holeIndex++) {
        const IndexedVertex &candidate =
            holes[holeIndex][findRightmostVertexIndex(holes[holeIndex])];
        if (candidate.x > bestVertex.x + EPSILON ||
            (std::abs(candidate.x - bestVertex.x) <= EPSILON &&
                candidate.y < bestVertex.y)) {
            bestVertex = candidate;
            bestHoleIndex = static_cast<int>(holeIndex);
        }
    }
    return bestHoleIndex;
}

int findVisibleBridgeVertex(const java::ArrayList<IndexedVertex> &polygon,
    const java::ArrayList<IndexedVertex> &hole, int holeVertexIndex,
    const java::ArrayList< java::ArrayList<IndexedVertex> > &remainingHoles)
{
    const IndexedVertex &holeVertex = hole[holeVertexIndex];
    int bestPolygonVertexIndex = -1;
    double bestDistanceSquared = java::Double::POSITIVE_INFINITY;

    for (size_t polygonVertexIndex = 0;
         polygonVertexIndex < polygon.size(); polygonVertexIndex++) {
        const IndexedVertex &polygonVertex = polygon[polygonVertexIndex];
        if (segmentCrossesBoundary(holeVertex, polygonVertex, polygon,
                static_cast<int>(polygonVertexIndex), hole, holeVertexIndex)) {
            continue;
        }

        bool crossesRemainingHole = false;
        for (size_t holeIndex = 0; holeIndex < remainingHoles.size();
             holeIndex++) {
            if (segmentCrossesAnyEdge(holeVertex, polygonVertex,
                    remainingHoles[holeIndex], -1)) {
                crossesRemainingHole = true;
                break;
            }
        }
        if (crossesRemainingHole) {
            continue;
        }

        const double midX = (holeVertex.x + polygonVertex.x) * 0.5;
        const double midY = (holeVertex.y + polygonVertex.y) * 0.5;
        if (!containsPoint(polygon, midX, midY)) {
            continue;
        }
        if (containsPoint(hole, midX, midY)) {
            continue;
        }

        const double distanceSquared =
            squaredDistance(holeVertex, polygonVertex);
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestPolygonVertexIndex = static_cast<int>(polygonVertexIndex);
        }
    }

    return bestPolygonVertexIndex;
}

java::ArrayList<IndexedVertex> bridgeHoleIntoPolygon(
    const java::ArrayList<IndexedVertex> &polygonInput,
    const java::ArrayList<IndexedVertex> &holeInput,
    const java::ArrayList< java::ArrayList<IndexedVertex> > &remainingHoles,
    bool &ok)
{
    const java::ArrayList<IndexedVertex> polygon =
        removeConsecutiveDuplicates(polygonInput);
    const java::ArrayList<IndexedVertex> hole =
        removeConsecutiveDuplicates(holeInput);
    if (hole.size() < 3) {
        ok = true;
        return polygon;
    }

    const int holeVertexIndex = findRightmostVertexIndex(hole);
    const IndexedVertex &holeVertex = hole[holeVertexIndex];
    const int bridgePolygonVertexIndex = findVisibleBridgeVertex(
        polygon, hole, holeVertexIndex, remainingHoles);
    if (bridgePolygonVertexIndex < 0) {
        ok = false;
        return java::ArrayList<IndexedVertex>();
    }

    java::ArrayList<IndexedVertex> merged;
    for (int i = 0; i <= bridgePolygonVertexIndex; i++) {
        merged.add(polygon[i]);
    }

    merged.add(holeVertex);
    for (size_t offset = 1; offset < hole.size(); offset++) {
        const int index =
            (holeVertexIndex + static_cast<int>(offset)) % static_cast<int>(hole.size());
        merged.add(hole[index]);
    }
    merged.add(holeVertex);

    for (size_t i = static_cast<size_t>(bridgePolygonVertexIndex);
         i < polygon.size(); i++) {
        merged.add(polygon[i]);
    }

    ok = true;
    return removeConsecutiveDuplicates(merged);
}

bool triangulateFilledRegion(const java::ArrayList<IndexedVertex> &outerInput,
    const java::ArrayList< java::ArrayList<IndexedVertex> > &holesInput,
    java::ArrayList<MonotoneDecompositionTriangulator::Triangle> &output)
{
    java::ArrayList<IndexedVertex> outer = outerInput;
    normalizeOrientation(outer, true);

    java::ArrayList< java::ArrayList<IndexedVertex> > holes = holesInput;
    for (size_t i = 0; i < holes.size(); i++) {
        normalizeOrientation(holes[i], false);
    }

    java::ArrayList<IndexedVertex> polygon = removeConsecutiveDuplicates(outer);
    if (polygon.size() < 3) {
        return true;
    }

    java::ArrayList< java::ArrayList<IndexedVertex> > remainingHoles = holes;
    while (remainingHoles.size() > 0) {
        const int nextHoleIndex = findRightmostHoleIndex(remainingHoles);
        const java::ArrayList<IndexedVertex> hole = remainingHoles[nextHoleIndex];
        remainingHoles.remove(nextHoleIndex);

        bool ok = false;
        polygon = bridgeHoleIntoPolygon(polygon, hole, remainingHoles, ok);
        if (!ok) {
            return false;
        }
    }

    return earClipSimplePolygon(polygon, output);
}

int _ContourAwarePolygonTriangulator::triangulate(
    const Polygon2D &input,
    java::ArrayList<MonotoneDecompositionTriangulator::Triangle> &output)
{
    java::ArrayList<ContourData> contours = flattenContours(input);
    if (contours.size() == 0) {
        output.clear();
        return 0;
    }

    classifyContourHierarchy(contours);

    java::ArrayList<MonotoneDecompositionTriangulator::Triangle> triangles;
    for (size_t contourIndex = 0; contourIndex < contours.size();
         contourIndex++) {
        const ContourData &contour = contours[contourIndex];
        if ((contour.depth % 2) == 0) {
            continue;
        }

        java::ArrayList< java::ArrayList<IndexedVertex> > holes;
        for (size_t childIndex = 0; childIndex < contour.childContours.size();
             childIndex++) {
            holes.add(
                contours[contour.childContours[childIndex]].vertices);
        }

        if (!triangulateFilledRegion(contour.vertices, holes, triangles)) {
            return -1;
        }
    }

    output.clear();
    for (long i = 0; i < triangles.size(); i++) {
        output.add(triangles.get(i));
    }
    return static_cast<int>(output.size());
}
