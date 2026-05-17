#ifndef __BICUBIC_PATCH_H__
#define __BICUBIC_PATCH_H__

#include "common/FrameConfig.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "geom/BezierNode.h"
#include "geom/GeometryOperations.h"

static constexpr int MAX_BICUBIC_INTERSECTIONS = 32;
class BezierVertices;
class BezierChild;
class BezierPatch;
class BezierIntersection;

class BicubicPatch : public Geometry {
  friend class BezierPatch;
  friend class BezierIntersection;
  public:
    int Patch_Type, U_Steps, V_Steps;
    Vector3Dd Control_Points[4][4];
    Vector3Dd Bounding_Sphere_Center;
    double Bounding_Sphere_Radius;
    double Flatness_Value;
    int Intersection_Count;
    Vector3Dd Normal_Vector[MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd Intersection_Point[MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd **Interpolated_Grid, **Interpolated_Normals, **Smooth_Normals;
    double **Interpolated_D;
    BezierNode *Node_Tree;

    static BicubicPatch *getBicubicPatchShape();
    static void precomputePatchValues(BicubicPatch *shape);
    static int insideBicubicPatch(Vector3Dd *point, SimpleBody *object);
    static void bicubicPatchNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copyBicubicPatch(SimpleBody *object);
    static void translateBicubicPatch(SimpleBody *object, Vector3Dd *vector);
    static void rotateBicubicPatch(SimpleBody *object, Vector3Dd *vector);
    static void scaleBicubicPatch(SimpleBody *object, Vector3Dd *vector);
    static void invertBicubicPatch(SimpleBody *object);
  private:
    static void bezierValue(
        Vector3Dd *result, double u, double v, Vector3Dd (*controlPoints)[4][4]);
    static void bezierPartial(Vector3Dd *result, double u, double v, BicubicPatch *shape);
    static int subpatchNormal(
        Vector3Dd *v1, Vector3Dd *v2, Vector3Dd *v3, Vector3Dd *result, double *d);
    static int intersectSubpatch(int patchType, Ray *ray, Vector3Dd *v1,
        Vector3Dd *v2, Vector3Dd *v3, Vector3Dd *n, double d, Vector3Dd *n1, Vector3Dd *n2,
        Vector3Dd *n3, double *depth, Vector3Dd *ip, Vector3Dd *ipNorm);
    static void findAverage(
        int vectorCount, Vector3Dd *vectors, Vector3Dd *center, double *radius);
    static double pointPlaneDistance(Vector3Dd *point, Vector3Dd *normal, double *d);
    static double determineSubpatchFlatness(Vector3Dd (*patch)[4][4]);
    static int flatEnough(BicubicPatch *shape, Vector3Dd (*patch)[4][4]);
    static void bezierBoundingSphere(
        Vector3Dd (*patch)[4][4], Vector3Dd *center, double *radiusSquared);
    static void bezierSubpatchIntersect(Ray *ray, BicubicPatch *shape,
        Vector3Dd (*patch)[4][4], double u0, double u1, double v0, int recursionDepth,
        int *depthCount, double *depths, double *u, double *v);
    static void bezierSplitLeftRight(
        Vector3Dd (*patch)[4][4], Vector3Dd (*left)[4][4], Vector3Dd (*right)[4][4]);
    static void bezierSplitUpDown(
        Vector3Dd (*patch)[4][4], Vector3Dd (*lower)[4][4], Vector3Dd (*upper)[4][4]);
    static void bezierSubdivider(Ray *ray, BicubicPatch *shape, Vector3Dd (*patch)[4][4],
        double u0, double u1, double v0, double v1, int recursionDepth, int *depthCount,
        double *depths, double *u, double *v);
    static void bezierTreeDeleter(BezierNode *node);
    static BezierNode *bezierTreeBuilder(
        BicubicPatch *shape, Vector3Dd (*patch)[4][4], int depth);
    static void bezierTreeWalker(
        Ray *ray, BicubicPatch *shape, BezierNode *node, int depth, int *depthCount, double *depths);
    static BezierNode *createNewBezierNode();
    static BezierVertices *createBezierVertexBlock();
    static BezierChild *createBezierChildBlock();
};

#endif
