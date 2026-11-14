#ifndef VSDK_HEIGHT_FIELD_SOLID_H
#define VSDK_HEIGHT_FIELD_SOLID_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldBlock.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldTraceResult.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldTraversal.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"

// Height field: heights h(x, z) on the integer grid 0 <= x < columns,
// 0 <= z < rows, stored in single precision and interpolated linearly over the
// two triangles of each cell, split along the diagonal from (x+1, z) to
// (x, z+1). The solid is the part of the bounding box below the surface,
// { min <= p <= max, y < h(x, z) }; the box must lie in the grid domain.
// Surface index 0 is the height surface and 1..6 the box faces -x, +x, -y,
// +y, -z, +z. Ray queries split the ray at cell edges and diagonals, where the
// height difference is linear, so crossings are exact up to rounding.
// traceFirstHit is the PovRay block traversal, which reports only the first
// surface hit and never box faces. Edits invalidate the traversal blocks until
// updateBlocks is called.
class HeightFieldSolid : public Solid {
public:
    // Tolerances of the PovRay first-hit traversal.
    static constexpr double INTERSECTION_EPSILON = 1.0e-5;
    static constexpr double SMALL_TOLERANCE = 0.001;
    static constexpr double MAX_DISTANCE = 1.0e7;
private:
    long columns;
    long rows;
    java::ArrayList<float> heights;
    Vector3Dd minimum;
    Vector3Dd maximum;
    double blockSize;
    double inverseBlockSize;
    long blockColumns;
    long blockRows;
    java::ArrayList<HeightFieldBlock> blocks;
    bool blocksValid;

    static double minValue(double x, double y) { return (x > y) ? y : x; }
    static double maxValue(double x, double y) { return (x < y) ? y : x; }
    static int signOf(double x) { return (x > 0.0) ? 1 : ((x == 0.0) ? 0 : -1); }
    double heightAt(int x, int z) const { return (double)heights[(long)z*columns + x]; }
    const HeightFieldBlock* blockAt(int x, int z) const;
    int intersectPixel(int x, int z, const Vector3Dd& origin, const Vector3Dd& direction,
        HeightFieldTraversal& state, double height1, double height2) const;
    int intersectSubBlock(const HeightFieldBlock* block, const Vector3Dd& origin,
        const Vector3Dd& direction, HeightFieldTraversal& state,
        const Vector3Dd* start, const Vector3Dd* end) const;
    int intersectHfNode(const Vector3Dd& origin, const Vector3Dd& direction,
        HeightFieldTraversal& state, const Vector3Dd* start, const Vector3Dd* end) const;
    bool isInsideGrid(double x, double z) const;
    double heightDifference(const Vector3Dd& origin, const Vector3Dd& direction, double t,
        long cellX, long cellZ, bool firstTriangle) const;
    void heightRange(double lowX, double highX, double lowZ, double highZ,
        double& lowest, double& highest) const;
    long nearestSurface(const Vector3Dd& point) const;
    Vector3Dd surfaceNormal(const Vector3Dd& point, long surfaceIndex) const;
    GeometryContainment classifyRegion(const Vector3Dd& center, const Vector3Dd& halfWidths,
        double tolerance) const;
public:
    // Heights start at zero. A grid needs at least 2 x 2 points.
    HeightFieldSolid(long columns, long rows, const Vector3Dd& minimum, const Vector3Dd& maximum);
    HeightFieldSolid(const HeightFieldSolid& other);
    HeightFieldSolid& operator=(const HeightFieldSolid& other);
    ~HeightFieldSolid() override;
    long getColumnCount() const { return columns; }
    long getRowCount() const { return rows; }
    double getHeight(long x, long z) const;
    void setHeight(long x, long z, double height);
    const Vector3Dd& getMinimum() const { return minimum; }
    const Vector3Dd& getMaximum() const { return maximum; }
    void setBounds(const Vector3Dd& minimum, const Vector3Dd& maximum);
    // Grid, finite heights and an ordered box inside the grid domain.
    bool isValid() const;
    // Rebuilds the traversal blocks; heights below the box are counted at its
    // floor.
    void updateBlocks();
    bool hasCurrentBlocks() const { return blocksValid; }
    // Interpolated height, NaN outside the grid.
    double evaluateHeight(double x, double z) const;
    // Edge vectors of the triangle under the point, in the PovRay order; their
    // cross product second x first is the upward surface normal.
    void getSurfaceTangents(const Vector3Dd& point, Vector3Dd& first, Vector3Dd& second) const;
    // PovRay first surface hit along origin + depth*direction, depth beyond
    // SMALL_TOLERANCE; requires current blocks. Like PovRay height fields, the
    // box must stay at least one unit inside the grid on x and z
    // (1 <= x <= columns - 2, likewise z); otherwise hits may be missed. It
    // can also report hits on the cells just outside the box. The typed ray
    // queries have neither limitation.
    HeightFieldTraceResult traceFirstHit(const Vector3Dd& origin, const Vector3Dd& direction,
        double& depth) const;

    Geometry* copyGeometry() const override;
    void getBounds(GeometryBounds& result) const override;
    GeometryContainment classifyPoint(const Vector3Dd& point, double tolerance) const override;
    GeometryContainment classifyPointCertified(const Vector3Dd& point, double tolerance) const override;
    GeometryContainment classifyBoxCertified(const Vector3Dd& center, const Vector3Dd& halfWidths) const override;
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = nullptr) const override;
    RayFirstHitStatus querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
        RayHit* hit) const override;
    bool doIntersectionFirstHit(const Ray& ray, RayHit* hit) override;
    void doExtraInformation(const Ray& ray, double t, RayHit* hit) override;
    int doContainmentTest(const Vector3Dd& point, double tolerance) override;
    double* getMinMax() override;
};
#endif
