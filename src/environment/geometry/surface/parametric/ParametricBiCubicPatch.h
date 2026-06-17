#ifndef __PARAMETRIC_BICUBIC_PATCH_H__
#define __PARAMETRIC_BICUBIC_PATCH_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/surface/parametric/ParametricControlPoints.h"
#include "environment/geometry/surface/parametric/ParametricPatchChild.h"
#include "environment/geometry/surface/parametric/ParametricPatchNode.h"

class ParametricBiCubicPatch : public Geometry {
  public:
    static constexpr int MAX_BICUBIC_INTERSECTIONS = 32;
    int patchType;
    int uSteps;
    int vSteps;
    Vector3Dd controlPoints[4][4];
    Vector3Dd boundingSphereCenter;
    double boundingSphereRadius;
    double flatnessValue;
    int intersectionCount;
    Vector3Dd normalVector[ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd intersectionPoint[ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd **interpolatedGrid;
    Vector3Dd **interpolatedNormals;
    Vector3Dd **smoothNormals;
    double **interpolatedD;
    ParametricPatchNode *nodeTree;

    int getPatchType() const { return patchType; }
    void setPatchType(int type) { patchType = type; }
    int getUSteps() const { return uSteps; }
    void setUSteps(int steps) { uSteps = steps; }
    int getVSteps() const { return vSteps; }
    void setVSteps(int steps) { vSteps = steps; }
    double getFlatnessValue() const { return flatnessValue; }
    void setFlatnessValue(double value) { flatnessValue = value; }
    Vector3Dd (*getControlPoints())[4] { return controlPoints; }

    static void precomputePatchValues(ParametricBiCubicPatch *shape);

    static void parametricValue(Vector3Dd *result, double u, double v,
        Vector3Dd (*controlPoints)[4][4]);
    static void findAverage(
        int vectorCount, Vector3Dd *vectors, Vector3Dd *center, double *radius);
    static void parametricSubdivider(const RayWithSegments *ray,
        ParametricBiCubicPatch *shape, Vector3Dd (*patch)[4][4], double u0,
        double u1, double v0, double v1, int recursionDepth, int *depthCount,
        double *depths, double *u, double *v);
    static void parametricTreeWalker(const RayWithSegments *ray,
        ParametricBiCubicPatch *shape, ParametricPatchNode *node, int depth,
        int *depthCount, double *depths);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    static void parametricPartial(
        Vector3Dd *result, double u, double v, ParametricBiCubicPatch *shape);
    static int subpatchNormal(Vector3Dd *v1, Vector3Dd *v2, Vector3Dd *v3,
        Vector3Dd *result, double *d);
    static int intersectSubpatch(int patchType, RayWithSegments *ray,
        Vector3Dd *v1, Vector3Dd *v2, Vector3Dd *v3, Vector3Dd *n, double d,
        Vector3Dd *n1, Vector3Dd *n2, Vector3Dd *n3, double *depth,
        Vector3Dd *ip, Vector3Dd *ipNorm);
    static double pointPlaneDistance(
        Vector3Dd *point, Vector3Dd *normal, double *d);
    static double determineSubpatchFlatness(Vector3Dd (*patch)[4][4]);
    static int flatEnough(
        ParametricBiCubicPatch *shape, Vector3Dd (*patch)[4][4]);
    static void parametricBoundingSphere(
        Vector3Dd (*patch)[4][4], Vector3Dd *center, double *radiusSquared);
    static void parametricSubpatchIntersect(const RayWithSegments *ray,
        ParametricBiCubicPatch *shape, Vector3Dd (*patch)[4][4], double u0,
        double u1, double v0, int recursionDepth, int *depthCount,
        double *depths, double *u, double *v);
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

#endif
