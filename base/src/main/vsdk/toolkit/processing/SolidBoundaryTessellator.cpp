#include <cmath>
#include "java/lang/Double.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidQueryStatus.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/scene/SceneSolid.h"
#include "vsdk/toolkit/processing/SolidBoundaryTessellator.h"

SolidBoundaryTessellator::SolidBoundaryTessellator(const SceneSolid& body,
    SolidBoundaryTessellationReport& report)
    : body(body), cellSize(0), report(report)
{
    nodeCounts[0] = nodeCounts[1] = nodeCounts[2] = 0;
}

long int SolidBoundaryTessellator::nodeIndex(long int i, long int j, long int k) const
{
    return (k*nodeCounts[1] + j)*nodeCounts[0] + i;
}

long int SolidBoundaryTessellator::cellIndex(long int i, long int j, long int k) const
{
    return (k*(nodeCounts[1] - 1) + j)*(nodeCounts[0] - 1) + i;
}

Vector3Dd SolidBoundaryTessellator::nodePoint(long int i, long int j, long int k) const
{
    return Vector3Dd(origin.x() + i*cellSize, origin.y() + j*cellSize, origin.z() + k*cellSize);
}

// The outer layer of nodes lies outside the region and counts as outside, so
// the mesh closes where the solid leaves the region.
bool SolidBoundaryTessellator::isBorderNode(long int i, long int j, long int k) const
{
    return i == 0 || j == 0 || k == 0 || i == nodeCounts[0] - 1 ||
        j == nodeCounts[1] - 1 || k == nodeCounts[2] - 1;
}

void SolidBoundaryTessellator::classifyNodesByPoints(long int j, long int k)
{
    report.pointClassifiedLines++;
    for (long int i = 1; i < nodeCounts[0] - 1; i++) {
        const Vector3Dd p = nodePoint(i, j, k);
        GeometryContainment c = body.classifyPoint(p, 0);
        if (c == GeometryContainment::Unsupported) c = body.classifyPointCertified(p, 0);
        if (c == GeometryContainment::Inside) inside[nodeIndex(i, j, k)] = INSIDE_NODE;
        else if (c != GeometryContainment::Outside && c != GeometryContainment::Boundary) report.unknownNodes++;
    }
}

bool SolidBoundaryTessellator::classifyNodes()
{
    const long int total = nodeCounts[0]*nodeCounts[1]*nodeCounts[2];
    inside.clear();
    inside.reserve(total);
    for (long int n = 0; n < total; n++) inside.add(OUTSIDE_NODE);
    const double length = (nodeCounts[0] - 1)*cellSize;
    RaySolidClassification classification;
    for (long int k = 1; k < nodeCounts[2] - 1; k++) {
        for (long int j = 1; j < nodeCounts[1] - 1; j++) {
            const Ray line = Ray::fromAffine(nodePoint(0, j, k), Vector3Dd(1, 0, 0));
            const RaySolidQueryStatus status =
                body.classifyRay(line, RayCrossingQuery(0, length), classification);
            if (status != RaySolidQueryStatus::Supported ||
                classification.insideSegments.size() != classification.boundaries.size() + 1) {
                classifyNodesByPoints(j, k);
                continue;
            }
            long int segment = 0;
            for (long int i = 1; i < nodeCounts[0] - 1; i++) {
                const double t = i*cellSize;
                while (segment < classification.boundaries.size() &&
                    classification.boundaries[segment].t < t) segment++;
                if (classification.insideSegments[segment]) inside[nodeIndex(i, j, k)] = INSIDE_NODE;
            }
        }
    }
    return true;
}

bool SolidBoundaryTessellator::firstHit(const Vector3Dd& from, const Vector3Dd& direction,
    double reach, Vector3Dd& point, Vector3Dd& normal) const
{
    const Ray ray(from, direction);
    RayHit candidate;
    candidate.setStoreRay(false);
    candidate.resetForDistanceOnly();
    if (!body.doIntersectionFirstHit(ray, &candidate)) return false;
    const double distance = candidate.getHitDistance();
    if (!(distance >= 0) || distance > reach) return false;
    const Ray hitRay = ray.withT(distance);
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    hit.setRay(hitRay);
    body.doExtraInformation(hitRay, distance, &hit);
    point = from.add(ray.getDirection().multiply(distance));
    normal = hit.normal;
    const double size = normal.length();
    if (!(size > 0) || !java::Double::isFinite(size)) return false;
    normal = normal.multiply(1.0/size);
    return true;
}

// Crossing on the edge from node (i, j, k) to its neighbour along the axis.
int SolidBoundaryTessellator::crossing(int axis, long int i, long int j, long int k)
{
    const long int edge = nodeIndex(i, j, k);
    if (edgeCrossings[axis][edge] >= 0) return edgeCrossings[axis][edge];
    long int ni = i, nj = j, nk = k;
    if (axis == 0) ni++; else if (axis == 1) nj++; else nk++;
    const bool firstInside = inside[edge] == INSIDE_NODE;
    const Vector3Dd a = nodePoint(i, j, k), b = nodePoint(ni, nj, nk);
    const Vector3Dd outer = firstInside ? b : a, inner = firstInside ? a : b;
    const bool outerIsBorder = firstInside ? isBorderNode(ni, nj, nk) : isBorderNode(i, j, k);
    const Vector3Dd inward = inner.subtract(outer).multiply(1.0/cellSize);
    Vector3Dd point, normal;
    if (firstHit(outer, inward, cellSize*(1 + 1e-9), point, normal)) {
        if (normal.dotProduct(inward) > 0) normal = normal.multiply(-1);
        report.exactCrossings++;
    }
    else if (outerIsBorder) {
        // The solid continues beyond the region: close it at the inner node.
        point = inner;
        normal = inward.multiply(-1);
        report.regionCrossings++;
    }
    else {
        point = a.add(b).multiply(0.5);
        normal = inward.multiply(-1);
        report.midpointCrossings++;
    }
    const int index = (int)crossingPoints.size();
    crossingPoints.add(point);
    crossingNormals.add(normal);
    edgeCrossings[axis][edge] = index;
    return index;
}

int SolidBoundaryTessellator::cellVertex(long int i, long int j, long int k)
{
    const long int cell = cellIndex(i, j, k);
    if (cellVertices[cell] >= 0) return cellVertices[cell];
    Vector3Dd sum(0, 0, 0), normalSum(0, 0, 0);
    int count = 0;
    for (int axis = 0; axis < 3; axis++) {
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                long int ei = i, ej = j, ek = k;
                if (axis == 0) { ej += a; ek += b; }
                else if (axis == 1) { ek += a; ei += b; }
                else { ei += a; ej += b; }
                long int fi = ei, fj = ej, fk = ek;
                if (axis == 0) fi++; else if (axis == 1) fj++; else fk++;
                if (inside[nodeIndex(ei, ej, ek)] == inside[nodeIndex(fi, fj, fk)]) continue;
                const int c = crossing(axis, ei, ej, ek);
                sum = sum.add(crossingPoints[c]);
                normalSum = normalSum.add(crossingNormals[c]);
                count++;
            }
        }
    }
    Vector3Dd point = sum.multiply(1.0/count);
    Vector3Dd normal = normalSum;
    const double size = normal.length();
    bool projected = false;
    if (size > 1e-12) {
        normal = normal.multiply(1.0/size);
        const double reach = cellSize*std::sqrt(3.0);
        const Vector3Dd from = point.add(normal.multiply(reach/2));
        Vector3Dd hitPoint, hitNormal;
        if (firstHit(from, normal.multiply(-1), reach, hitPoint, hitNormal)) {
            const Vector3Dd low = nodePoint(i, j, k);
            const double margin = 1e-9*cellSize;
            if (hitPoint.x() >= low.x() - margin && hitPoint.x() <= low.x() + cellSize + margin &&
                hitPoint.y() >= low.y() - margin && hitPoint.y() <= low.y() + cellSize + margin &&
                hitPoint.z() >= low.z() - margin && hitPoint.z() <= low.z() + cellSize + margin) {
                if (hitNormal.dotProduct(normal) < 0) hitNormal = hitNormal.multiply(-1);
                point = hitPoint;
                normal = hitNormal;
                projected = true;
            }
        }
    }
    else normal = Vector3Dd(0, 0, 1);
    if (projected) report.boundaryVertices++;
    else report.cellVertices++;
    const int index = (int)vertexPoints.size();
    vertexPoints.add(point);
    vertexNormals.add(normal);
    cellVertices[cell] = index;
    return index;
}

// Every edge along the axis whose ends differ gives the quad of the four cells
// around it, wound to face from the inside end to the outside end.
void SolidBoundaryTessellator::emitQuads(int axis)
{
    const int b = (axis + 1) % 3, c = (axis + 2) % 3;
    long int index[3];
    for (index[2] = 0; index[2] < nodeCounts[2]; index[2]++) {
        for (index[1] = 0; index[1] < nodeCounts[1]; index[1]++) {
            for (index[0] = 0; index[0] < nodeCounts[0]; index[0]++) {
                if (index[axis] + 1 >= nodeCounts[axis]) continue;
                long int next[3] = {index[0], index[1], index[2]};
                next[axis]++;
                const bool here = inside[nodeIndex(index[0], index[1], index[2])] == INSIDE_NODE;
                const bool there = inside[nodeIndex(next[0], next[1], next[2])] == INSIDE_NODE;
                if (here == there) continue;
                // Border nodes are outside, so both ends are interior across b and c.
                int quad[4];
                const int offsets[4][2] = {{-1, -1}, {0, -1}, {0, 0}, {-1, 0}};
                for (int q = 0; q < 4; q++) {
                    long int cell[3] = {index[0], index[1], index[2]};
                    cell[b] += offsets[q][0];
                    cell[c] += offsets[q][1];
                    quad[q] = cellVertex(cell[0], cell[1], cell[2]);
                }
                // The (b, c) cycle above turns around +axis.
                if (here) {
                    triangleIndices.add(quad[0]); triangleIndices.add(quad[1]); triangleIndices.add(quad[2]);
                    triangleIndices.add(quad[0]); triangleIndices.add(quad[2]); triangleIndices.add(quad[3]);
                }
                else {
                    triangleIndices.add(quad[0]); triangleIndices.add(quad[2]); triangleIndices.add(quad[1]);
                    triangleIndices.add(quad[0]); triangleIndices.add(quad[3]); triangleIndices.add(quad[2]);
                }
            }
        }
    }
}

TriangleMesh* SolidBoundaryTessellator::buildMesh() const
{
    TriangleMesh* mesh = new TriangleMesh();
    const int vertexCount = (int)vertexPoints.size();
    const int triangleCount = (int)(triangleIndices.size()/3);
    mesh->initVertexPositionsArray(vertexCount);
    mesh->initVertexNormalsArray();
    java::ArrayList<double>& positions = mesh->getVertexPositions();
    java::ArrayList<double>& normals = mesh->getVertexNormals();
    for (int v = 0; v < vertexCount; v++) {
        positions[v*3] = vertexPoints[v].x();
        positions[v*3 + 1] = vertexPoints[v].y();
        positions[v*3 + 2] = vertexPoints[v].z();
        normals[v*3] = vertexNormals[v].x();
        normals[v*3 + 1] = vertexNormals[v].y();
        normals[v*3 + 2] = vertexNormals[v].z();
    }
    mesh->initTriangleArrays(triangleCount);
    java::ArrayList<int>& indices = mesh->getTriangleIndexes();
    java::ArrayList<double>& faceNormals = mesh->getTriangleNormals();
    for (int t = 0; t < triangleCount; t++) {
        const int v0 = triangleIndices[t*3], v1 = triangleIndices[t*3 + 1], v2 = triangleIndices[t*3 + 2];
        indices[t*3] = v0;
        indices[t*3 + 1] = v1;
        indices[t*3 + 2] = v2;
        Vector3Dd n = vertexPoints[v1].subtract(vertexPoints[v0]).crossProduct(
            vertexPoints[v2].subtract(vertexPoints[v0]));
        const double size = n.length();
        n = size > 0 ? n.multiply(1.0/size) : vertexNormals[v0];
        faceNormals[t*3] = n.x();
        faceNormals[t*3 + 1] = n.y();
        faceNormals[t*3 + 2] = n.z();
    }
    return mesh;
}

TriangleMesh* SolidBoundaryTessellator::tessellate(const SceneSolid& body,
    const GeometryBounds& region, int resolution, SolidBoundaryTessellationReport& report)
{
    report.clear();
    if (region.getKind() != GeometryBounds::Kind::Finite) return nullptr;
    if (resolution < 1 || resolution > MAXIMUM_RESOLUTION) return nullptr;
    const Vector3Dd low = region.getMinimum(), high = region.getMaximum();
    const double extent[3] = {high.x() - low.x(), high.y() - low.y(), high.z() - low.z()};
    double longest = 0;
    for (int a = 0; a < 3; a++) {
        if (!java::Double::isFinite(extent[a]) || extent[a] < 0) return nullptr;
        if (extent[a] > longest) longest = extent[a];
    }
    if (!(longest > 0)) return nullptr;

    SolidBoundaryTessellator tessellator(body, report);
    tessellator.cellSize = longest/resolution;
    // Cells covering the region, plus one layer of outside nodes on each side.
    for (int a = 0; a < 3; a++) {
        long int cells = (long int)std::ceil(extent[a]/tessellator.cellSize - 1e-9);
        if (cells < 1) cells = 1;
        tessellator.nodeCounts[a] = cells + 3;
        report.nodeCounts[a] = tessellator.nodeCounts[a];
    }
    report.cellSize = tessellator.cellSize;
    tessellator.origin = Vector3Dd(low.x() - tessellator.cellSize, low.y() - tessellator.cellSize,
        low.z() - tessellator.cellSize);
    tessellator.classifyNodes();

    const long int nodes = tessellator.nodeCounts[0]*tessellator.nodeCounts[1]*tessellator.nodeCounts[2];
    for (int a = 0; a < 3; a++) {
        tessellator.edgeCrossings[a].reserve(nodes);
        for (long int n = 0; n < nodes; n++) tessellator.edgeCrossings[a].add(-1);
    }
    const long int cells = (tessellator.nodeCounts[0] - 1)*(tessellator.nodeCounts[1] - 1)*
        (tessellator.nodeCounts[2] - 1);
    tessellator.cellVertices.reserve(cells);
    for (long int c = 0; c < cells; c++) tessellator.cellVertices.add(-1);
    for (int a = 0; a < 3; a++) tessellator.emitQuads(a);
    report.triangles = tessellator.triangleIndices.size()/3;
    return tessellator.buildMesh();
}
