#ifndef VSDK_SOLID_BOUNDARY_TESSELLATOR_H
#define VSDK_SOLID_BOUNDARY_TESSELLATOR_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/processing/SolidBoundaryTessellationReport.h"
class SceneSolid;
class TriangleMesh;

// Drawable triangle mesh of the boundary of a body's solid inside a world
// region: naive surface nets (Gibson 1998) over a grid of cubic cells whose
// side is the region's longest side divided by the resolution. It works for any
// body that classifies rays or points, such as quadrics, implicit solids,
// height fields and CSG bodies, and asks the body for everything:
// - node containment comes from one ray classification per grid line along x
//   (one point query per node when the line cannot be classified);
// - each grid edge whose ends differ crosses the boundary where the body's
//   first hit from the outside end lies, with the body's surface normal;
// - each cell holding crossings gets one vertex, the mean of its crossings,
//   moved onto the boundary by a ray along the mean normal when that hit stays
//   in the cell;
// - each crossing edge gives the quad of the four cells around it, facing out.
// The mesh is closed where the solid leaves the region. It is an approximation
// of the declared resolution, not another definition of the solid: details
// thinner than a cell may disappear or join, and a cell with several sheets
// gets one vertex. The report says how many crossings and vertices lie on the
// boundary found by the body's own ray queries.
class SolidBoundaryTessellator {
private:
    static constexpr signed char OUTSIDE_NODE = 0;
    static constexpr signed char INSIDE_NODE = 1;
    const SceneSolid& body;
    Vector3Dd origin;
    double cellSize;
    long int nodeCounts[3];
    java::ArrayList<signed char> inside;
    java::ArrayList<int> edgeCrossings[3];
    java::ArrayList<Vector3Dd> crossingPoints;
    java::ArrayList<Vector3Dd> crossingNormals;
    java::ArrayList<int> cellVertices;
    java::ArrayList<Vector3Dd> vertexPoints;
    java::ArrayList<Vector3Dd> vertexNormals;
    java::ArrayList<int> triangleIndices;
    SolidBoundaryTessellationReport& report;

    SolidBoundaryTessellator(const SceneSolid& body, SolidBoundaryTessellationReport& report);
    long int nodeIndex(long int i, long int j, long int k) const;
    long int cellIndex(long int i, long int j, long int k) const;
    Vector3Dd nodePoint(long int i, long int j, long int k) const;
    bool isBorderNode(long int i, long int j, long int k) const;
    bool classifyNodes();
    void classifyNodesByPoints(long int j, long int k);
    int crossing(int axis, long int i, long int j, long int k);
    bool firstHit(const Vector3Dd& from, const Vector3Dd& direction, double reach,
        Vector3Dd& point, Vector3Dd& normal) const;
    int cellVertex(long int i, long int j, long int k);
    void emitQuads(int axis);
    TriangleMesh* buildMesh() const;
    SolidBoundaryTessellator(const SolidBoundaryTessellator&) = delete;
    SolidBoundaryTessellator& operator=(const SolidBoundaryTessellator&) = delete;
public:
    // Largest accepted resolution.
    static const int MAXIMUM_RESOLUTION = 512;
    // Returns a new mesh, owned by the caller, in world coordinates, or null
    // when the region is not finite and nonempty or the resolution is outside
    // 1..MAXIMUM_RESOLUTION. An empty mesh means no boundary was found.
    static TriangleMesh* tessellate(const SceneSolid& body, const GeometryBounds& region,
        int resolution, SolidBoundaryTessellationReport& report);
};

#endif
