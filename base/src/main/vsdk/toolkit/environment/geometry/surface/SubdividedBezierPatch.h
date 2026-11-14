#ifndef VSDK_SUBDIVIDED_BEZIER_PATCH_H
#define VSDK_SUBDIVIDED_BEZIER_PATCH_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/surface/BezierPatchSubdivision.h"
#include "vsdk/toolkit/environment/geometry/surface/SubdividedBezierPatchNode.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"

// Open bicubic Bezier patch rendered as the flat triangles of a subdivision of
// its 4 x 4 control net: the PovRay bicubic_patch. Control point (i, j) has
// parameters u = i/3 and v = j/3. The geometry is the triangulation chosen by
// the subdivision, uSteps, vSteps and flatness, not the exact Bezier surface;
// for that see ParametricBiCubicPatch. Being open, the patch has no inside:
// ray classification and containment are Unsupported. Ray crossings are
// Unclassified and their surface index is the hit's position in intersect
// order; typed normals face the u x v side of their triangle. Bounds are the
// control net's box, which holds the patch.
class SubdividedBezierPatch : public Surface {
public:
    // Hits a single ray query can report.
    static constexpr int MAXIMUM_HITS = 32;
    // Degenerate triangle, derivative and direction threshold.
    static constexpr double DEGENERACY_EPSILON = 1.0e-10;
    // Depth below which PovRay ignores a hit.
    static constexpr double POVRAY_MINIMUM_DEPTH = 0.001;
private:
    BezierPatchSubdivision subdivision;
    int uSteps;
    int vSteps;
    double flatness;
    Vector3Dd controlPoints[4][4];
    Vector3Dd boundingCenter;
    double boundingRadiusSquared;
    // Precomputed grids, (uSteps + 1) rows: grid points with vSteps + 1
    // columns; triangle planes and offsets with 2*(vSteps + 1) columns.
    java::ArrayList<Vector3Dd> gridPoints;
    java::ArrayList<Vector3Dd> gridPlaneNormals;
    java::ArrayList<double> gridPlaneOffsets;
    java::ArrayList<Vector3Dd> gridSmoothNormals;
    java::ArrayList<SubdividedBezierPatchNode> nodes;

    static void sphereAround(int count, const Vector3Dd* points, Vector3Dd& center, double& radiusSquared);
    static void netSphere(const Vector3Dd (&net)[4][4], Vector3Dd& center, double& radiusSquared);
    static bool trianglePlane(const Vector3Dd& v1, const Vector3Dd& v2, const Vector3Dd& v3,
        Vector3Dd& normal, double& offset);
    static double planeDistance(const Vector3Dd& p, const Vector3Dd& n, double d);
    static bool rayMeetsSphere(const Vector3Dd& origin, const Vector3Dd& direction,
        const Vector3Dd& center, double radiusSquared);
    static void splitLeftRight(const Vector3Dd (&net)[4][4], Vector3Dd (&left)[4][4], Vector3Dd (&right)[4][4]);
    static void splitUpDown(const Vector3Dd (&net)[4][4], Vector3Dd (&top)[4][4], Vector3Dd (&bottom)[4][4]);
    static double netFlatness(const Vector3Dd (&net)[4][4]);
    bool isFlatEnough(const Vector3Dd (&net)[4][4]) const;
    bool intersectTriangle(const Vector3Dd& origin, const Vector3Dd& direction, double minimumDepth,
        bool orientNormal,
        const Vector3Dd& v1, const Vector3Dd& v2, const Vector3Dd& v3, const Vector3Dd& n, double d,
        const Vector3Dd* n1, const Vector3Dd* n2, const Vector3Dd* n3,
        double& depth, Vector3Dd& point, Vector3Dd& normal) const;
    long buildTree(const Vector3Dd (&net)[4][4], int depth);
    void precompute();
    long gridIndex(int i, int j) const { return (long)i*(vSteps + 1) + j; }
    long planeIndex(int i, int j) const { return (long)i*2*(vSteps + 1) + j; }

    // The hit sink of every strategy; returns false once the hit budget is spent.
    class HitList {
    public:
        int used;
        int maximum;
        int count;
        double* depths;
        Vector3Dd* points;
        Vector3Dd* normals;
        bool orientNormals;
        bool add(double depth, const Vector3Dd& point, const Vector3Dd& normal);
        bool isFull() const { return used + count >= maximum; }
    };
    int intersectWith(const Vector3Dd& origin, const Vector3Dd& direction, double minimumDepth,
        bool orient, int alreadyUsed, int maximumHits, double* depths, Vector3Dd* points,
        Vector3Dd* normals) const;
    int intersectUniform(const Vector3Dd& origin, const Vector3Dd& direction, double minimumDepth, HitList& hits) const;
    int intersectGrid(const Vector3Dd& origin, const Vector3Dd& direction, double minimumDepth, HitList& hits) const;
    int intersectSmooth(const Vector3Dd& origin, const Vector3Dd& direction, double minimumDepth, HitList& hits) const;
    void subdivide(const Vector3Dd& origin, const Vector3Dd& direction, double minimumDepth,
        const Vector3Dd (&net)[4][4], int depth, HitList& hits) const;
    void intersectPiece(const Vector3Dd& origin, const Vector3Dd& direction, double minimumDepth,
        const Vector3Dd& c0, const Vector3Dd& c1, const Vector3Dd& c2, const Vector3Dd& c3, HitList& hits) const;
    void walkTree(const Vector3Dd& origin, const Vector3Dd& direction, double minimumDepth,
        long node, HitList& hits) const;
    RayFirstHitStatus collectHits(const Ray& ray, const RayCrossingQuery& query, int& count,
        double* depths, Vector3Dd* normals) const;
public:
    // Throws java::IllegalArgumentException for steps below 1 (or, for the
    // adaptive subdivisions, below 0) or a non-finite control point.
    SubdividedBezierPatch(BezierPatchSubdivision subdivision, int uSteps, int vSteps,
        double flatness, const Vector3Dd (&controlPoints)[4][4]);
    SubdividedBezierPatch(const SubdividedBezierPatch& other);
    SubdividedBezierPatch& operator=(const SubdividedBezierPatch& other);
    ~SubdividedBezierPatch() override;
    BezierPatchSubdivision getSubdivision() const { return subdivision; }
    int getUSteps() const { return uSteps; }
    int getVSteps() const { return vSteps; }
    double getFlatness() const { return flatness; }
    const Vector3Dd& getControlPoint(int i, int j) const { return controlPoints[i][j]; }
    // Point of the Bezier surface at (u, v) in [0, 1]^2.
    Vector3Dd evaluate(double u, double v) const;
    // Cross product of the unit u and v derivatives, or (1, 0, 0) where
    // either derivative vanishes.
    Vector3Dd evaluateNormal(double u, double v) const;
    // PovRay hits along origin + depth*direction with depth >= minimumDepth,
    // in discovery order, each with its point and unit triangle (or, for
    // SmoothGrid, interpolated) normal. alreadyUsed hits of a shared budget of
    // maximumHits are already spent; returns how many were added, at most
    // maximumHits - alreadyUsed, written from index 0 of the arrays.
    int intersect(const Vector3Dd& origin, const Vector3Dd& direction, double minimumDepth,
        int alreadyUsed, int maximumHits, double* depths, Vector3Dd* points, Vector3Dd* normals) const;
    // As intersect, with every normal turned to the u x v side of its
    // triangle; the PovRay SmoothGrid interpolation can leave it reversed or,
    // dividing by zero, not finite, and then the triangle normal is used.
    int intersectOriented(const Vector3Dd& origin, const Vector3Dd& direction, double minimumDepth,
        int alreadyUsed, int maximumHits, double* depths, Vector3Dd* points, Vector3Dd* normals) const;

    Geometry* copyGeometry() const override;
    void getBounds(GeometryBounds& result) const override;
    RayFirstHitStatus queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
        RayHit* hit, RayBoundary* boundary = nullptr) const override;
    RayFirstHitStatus querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
        RayHit* hit) const override;
    RayCrossingSupport emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
        RayCrossingSink& sink) const override;
    // Like PovRay, ignores hits nearer than POVRAY_MINIMUM_DEPTH: the
    // overlapping pieces of an adaptive subdivision would shadow themselves.
    bool doIntersectionFirstHit(const Ray& ray, RayHit* hit) override;
    void doExtraInformation(const Ray& ray, double t, RayHit* hit) override;
    double* getMinMax() override;
};
#endif
