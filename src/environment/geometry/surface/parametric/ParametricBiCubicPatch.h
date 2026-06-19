#ifndef __PARAMETRIC_BI_CUBIC_PATCH__
#define __PARAMETRIC_BI_CUBIC_PATCH__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/surface/parametric/ParametricControlPoints.h"
#include "environment/geometry/surface/parametric/ParametricPatchChild.h"
#include "environment/geometry/surface/parametric/ParametricPatchNode.h"

class ParametricBiCubicPatch : public Geometry {
  public:
    static constexpr int MAX_BICUBIC_INTERSECTIONS = 32;

    ParametricBiCubicPatch();
    ParametricBiCubicPatch(int patchType, int uSteps, int vSteps,
        double flatnessValue, const Vector3Dd (&controlPoints)[4][4]);

    int getPatchType() const;
    int getUSteps() const;
    int getVSteps() const;
    Vector3Dd (*getControlPoints())[4];
    const Vector3Dd (*getControlPoints() const)[4];
    Vector3Dd &getBoundingSphereCenter();
    const Vector3Dd &getBoundingSphereCenter() const;
    double getBoundingSphereRadius() const;
    void setBoundingSphereRadius(double radius);
    double getFlatnessValue() const;
    int getIntersectionCount() const;
    void setIntersectionCount(int count);
    void incrementIntersectionCount();
    Vector3Dd &getNormalVectorAt(int index);
    const Vector3Dd &getNormalVectorAt(int index) const;
    Vector3Dd &getIntersectionPointAt(int index);
    const Vector3Dd &getIntersectionPointAt(int index) const;
    Vector3Dd **getInterpolatedGrid() const;
    void setInterpolatedGrid(Vector3Dd **grid);
    Vector3Dd **getInterpolatedNormals() const;
    void setInterpolatedNormals(Vector3Dd **normals);
    Vector3Dd **getSmoothNormals() const;
    void setSmoothNormals(Vector3Dd **normals);
    double **getInterpolatedD() const;
    void setInterpolatedD(double **values);
    ParametricPatchNode *getNodeTree() const;
    void setNodeTree(ParametricPatchNode *tree);

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
    void normal(
        Vector3Dd *result,
        Vector3Dd *intersectionPoint,
        const RenderingConfiguration *config) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    const int patchType;
    const int uSteps;
    const int vSteps;
    Vector3Dd controlPoints[4][4];
    Vector3Dd boundingSphereCenter;
    double boundingSphereRadius;
    const double flatnessValue;
    int intersectionCount;
    Vector3Dd normalVector[ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd intersectionPoint[ParametricBiCubicPatch::MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd **interpolatedGrid;
    Vector3Dd **interpolatedNormals;
    Vector3Dd **smoothNormals;
    double **interpolatedD;
    ParametricPatchNode *nodeTree;

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

inline const Vector3Dd &
ParametricBiCubicPatch::getBoundingSphereCenter() const
{
    return boundingSphereCenter;
}

inline double
ParametricBiCubicPatch::getBoundingSphereRadius() const
{
    return boundingSphereRadius;
}

inline void
ParametricBiCubicPatch::setBoundingSphereRadius(double radius)
{
    boundingSphereRadius = radius;
}

inline double
ParametricBiCubicPatch::getFlatnessValue() const
{
    return flatnessValue;
}

inline int
ParametricBiCubicPatch::getIntersectionCount() const
{
    return intersectionCount;
}

inline void
ParametricBiCubicPatch::setIntersectionCount(int count)
{
    intersectionCount = count;
}

inline void
ParametricBiCubicPatch::incrementIntersectionCount()
{
    intersectionCount++;
}

inline Vector3Dd &
ParametricBiCubicPatch::getNormalVectorAt(int index)
{
    return normalVector[index];
}

inline const Vector3Dd &
ParametricBiCubicPatch::getNormalVectorAt(int index) const
{
    return normalVector[index];
}

inline Vector3Dd &
ParametricBiCubicPatch::getIntersectionPointAt(int index)
{
    return intersectionPoint[index];
}

inline const Vector3Dd &
ParametricBiCubicPatch::getIntersectionPointAt(int index) const
{
    return intersectionPoint[index];
}

inline Vector3Dd **
ParametricBiCubicPatch::getInterpolatedGrid() const
{
    return interpolatedGrid;
}

inline void
ParametricBiCubicPatch::setInterpolatedGrid(Vector3Dd **grid)
{
    interpolatedGrid = grid;
}

inline Vector3Dd **
ParametricBiCubicPatch::getInterpolatedNormals() const
{
    return interpolatedNormals;
}

inline void
ParametricBiCubicPatch::setInterpolatedNormals(Vector3Dd **normals)
{
    interpolatedNormals = normals;
}

inline Vector3Dd **
ParametricBiCubicPatch::getSmoothNormals() const
{
    return smoothNormals;
}

inline void
ParametricBiCubicPatch::setSmoothNormals(Vector3Dd **normals)
{
    smoothNormals = normals;
}

inline double **
ParametricBiCubicPatch::getInterpolatedD() const
{
    return interpolatedD;
}

inline void
ParametricBiCubicPatch::setInterpolatedD(double **values)
{
    interpolatedD = values;
}

inline ParametricPatchNode *
ParametricBiCubicPatch::getNodeTree() const
{
    return nodeTree;
}

inline void
ParametricBiCubicPatch::setNodeTree(ParametricPatchNode *tree)
{
    nodeTree = tree;
}

#endif
