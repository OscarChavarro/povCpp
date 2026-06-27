#ifndef __PARAMETRIC_BI_CUBIC_PATCH__
#define __PARAMETRIC_BI_CUBIC_PATCH__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/TransformedGeometry.h"
#include "environment/geometry/surface/parametric/ParametricControlPoints.h"
#include "environment/geometry/surface/parametric/ParametricPatchChild.h"
#include "environment/geometry/surface/parametric/ParametricPatchNode.h"

class ParametricBiCubicPatch : public TransformedGeometry {
  public:
    static constexpr int MAX_BICUBIC_INTERSECTIONS = 32;

    ParametricBiCubicPatch();
    ParametricBiCubicPatch(int patchType, int uSteps, int vSteps,
        double flatnessValue, const Vector3Dd (&controlPoints)[4][4]);
    ParametricBiCubicPatch(const ParametricBiCubicPatch &other);
    ~ParametricBiCubicPatch() override;

    int getPatchType() const;
    int getUSteps() const;
    int getVSteps() const;
    Vector3Dd (*getControlPoints())[4];
    const Vector3Dd (*getControlPoints() const)[4];
    Vector3Dd &getBoundingSphereCenter();
    double getBoundingSphereRadius() const;
    int getIntersectionCount() const;
    void setIntersectionCount(int count);
    void incrementIntersectionCount();
    Vector3Dd &getNormalVectorAt(int index);
    Vector3Dd &getIntersectionPointAt(int index);
    Vector3Dd **getInterpolatedGrid() const;
    Vector3Dd **getInterpolatedNormals() const;
    Vector3Dd **getSmoothNormals() const;
    double **getInterpolatedD() const;
    ParametricPatchNode *getNodeTree() const;

    static void precomputePatchValues(ParametricBiCubicPatch *shape);

    static void parametricValue(Vector3Dd *result, double u, double v,
        Vector3Dd (*controlPoints)[4][4]);
    static void findAverage(
        int vectorCount, Vector3Dd *vectors, Vector3Dd *center, double *radius);
    static void parametricSubDivider(const RayWithSegments *ray,
        ParametricBiCubicPatch *object, Vector3Dd (*patch)[4][4], double u0,
        double u1, double v0, double v1, int recursionDepth, int *depthCount,
        double *depths, double *uValues, double *vValues);
    static void parametricTreeWalker(const RayWithSegments *ray,
        ParametricBiCubicPatch *shape, ParametricPatchNode *node, int depth,
        int *depthCount, double *depths);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) override;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void normal(Vector3Dd *result, Vector3Dd *localIntersectionPoint) override;
    void normal(
        Vector3Dd *result,
        Vector3Dd *localIntersectionPoint,
        const PovRayRendererConfiguration *config) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    static int maxDepthReached;

    const int patchType;
    const int uSteps;
    const int vSteps;
    Vector3Dd controlPoints[4][4];
    Vector3Dd boundingSphereCenter;
    double boundingSphereRadius;
    const double flatnessValue;
    // intersectionCount/normalVector/intersectionPoint used to live here as
    // plain instance fields: scratch space written by allIntersections() and
    // read back later by normal() (and by ParametricBiCubicSolver, which
    // stores the *hit point itself* through getIntersectionPointAt() - see
    // ParametricBiCubicSolver::allParametricBiCubicPatchIntersections).
    // That is safe only when one thread touches one patch instance at a
    // time; under `-parallel`, two threads hitting the SAME shared patch
    // object concurrently tear each other's writes, which is exactly the
    // speckled/missing-pixel corruption seen on bezier.pov/teapot.pov. The
    // getters/setters below keep the exact same call-site contract but are
    // now backed by thread-local storage keyed on `this` (see the .cpp), so
    // each thread sees its own scratch state per patch instance instead of
    // racing on a shared one.
    Vector3Dd **interpolatedGrid;
    Vector3Dd **interpolatedNormals;
    Vector3Dd **smoothNormals;
    double **interpolatedD;
    ParametricPatchNode *nodeTree;

    static void parametricPartial(
        Vector3Dd *result, double u, double v, ParametricBiCubicPatch *shape);
    static double determineSubPatchFlatness(Vector3Dd (*patch)[4][4]);
    static int flatEnough(
        ParametricBiCubicPatch *shape, Vector3Dd (*patch)[4][4]);
    static void parametricBoundingSphere(
        Vector3Dd (*patch)[4][4], Vector3Dd *center, double *radiusSquared);
    static void parametricSubPatchIntersect(const RayWithSegments *ray,
        ParametricBiCubicPatch *shape, Vector3Dd (*patch)[4][4],
        int *depthCount, double *depths);
    static void parametricSplitLeftRight(Vector3Dd (*patch)[4][4],
        Vector3Dd (*left)[4][4], Vector3Dd (*right)[4][4]);
    static void parametricSplitUpDown(Vector3Dd (*patch)[4][4],
        Vector3Dd (*lower)[4][4], Vector3Dd (*upper)[4][4]);
    static void parametricTreeDeleter(ParametricPatchNode *node);
    static ParametricPatchNode *parametricTreeBuilder(
        ParametricBiCubicPatch *shape, Vector3Dd (*patch)[4][4], int depth);
    static ParametricPatchNode *createNewParametricPatchNode();
    static ParametricControlPoints *createParametricControlPointsBlock();
    static ParametricPatchChild *createParametricPatchChildBlock();
};

inline int
ParametricBiCubicPatch::getPatchType() const
{
    return patchType;
}

inline int
ParametricBiCubicPatch::getUSteps() const
{
    return uSteps;
}

inline int
ParametricBiCubicPatch::getVSteps() const
{
    return vSteps;
}

inline Vector3Dd (*
ParametricBiCubicPatch::getControlPoints())[4]
{
    return controlPoints;
}

inline const Vector3Dd (*
ParametricBiCubicPatch::getControlPoints() const)[4]
{
    return controlPoints;
}

inline Vector3Dd &
ParametricBiCubicPatch::getBoundingSphereCenter()
{
    return boundingSphereCenter;
}

inline double
ParametricBiCubicPatch::getBoundingSphereRadius() const
{
    return boundingSphereRadius;
}

// getIntersectionCount/setIntersectionCount/incrementIntersectionCount/
// getNormalVectorAt/getIntersectionPointAt are defined in the .cpp (thread-
// local backing storage, see the private section's comment above).

inline Vector3Dd **
ParametricBiCubicPatch::getInterpolatedGrid() const
{
    return interpolatedGrid;
}

inline Vector3Dd **
ParametricBiCubicPatch::getInterpolatedNormals() const
{
    return interpolatedNormals;
}

inline Vector3Dd **
ParametricBiCubicPatch::getSmoothNormals() const
{
    return smoothNormals;
}

inline double **
ParametricBiCubicPatch::getInterpolatedD() const
{
    return interpolatedD;
}

inline ParametricPatchNode *
ParametricBiCubicPatch::getNodeTree() const
{
    return nodeTree;
}

#endif
