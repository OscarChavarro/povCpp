#ifndef __BICUBIC_PATCH_H__
#define __BICUBIC_PATCH_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "geom/BezierNode.h"
#include "geom/Geometry.h"

static constexpr int MAX_BICUBIC_INTERSECTIONS = 32;
class BezierVertices;
class BezierChild;

class BicubicPatch : public Geometry {
  public:
    int Patch_Type, U_Steps, V_Steps;
    Vector3D Control_Points[4][4];
    Vector3D Bounding_Sphere_Center;
    DBL Bounding_Sphere_Radius;
    DBL Flatness_Value;
    int Intersection_Count;
    Vector3D Normal_Vector[MAX_BICUBIC_INTERSECTIONS];
    Vector3D Intersection_Point[MAX_BICUBIC_INTERSECTIONS];
    Vector3D **Interpolated_Grid, **Interpolated_Normals, **Smooth_Normals;
    DBL **Interpolated_D;
    BezierNode *Node_Tree;

    static BicubicPatch *getBicubicPatchShape();
    static void precomputePatchValues(BicubicPatch *shape);
    static int allBicubicPatchIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int insideBicubicPatch(Vector3D *point, SimpleBody *object);
    static void bicubicPatchNormal(
        Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint);
    static void *copyBicubicPatch(SimpleBody *object);
    static void translateBicubicPatch(SimpleBody *object, Vector3D *vector);
    static void rotateBicubicPatch(SimpleBody *object, Vector3D *vector);
    static void scaleBicubicPatch(SimpleBody *object, Vector3D *vector);
    static void invertBicubicPatch(SimpleBody *object);
  private:
    static void bezierValue(
        Vector3D *result, DBL u, DBL v, Vector3D (*controlPoints)[4][4]);
    static void bezierPartial(Vector3D *result, DBL u, DBL v, BicubicPatch *shape);
    static int subpatchNormal(
        Vector3D *v1, Vector3D *v2, Vector3D *v3, Vector3D *result, DBL *d);
    static int intersectSubpatch(int patchType, Ray *ray, Vector3D *v1,
        Vector3D *v2, Vector3D *v3, Vector3D *n, DBL d, Vector3D *n1, Vector3D *n2,
        Vector3D *n3, DBL *depth, Vector3D *ip, Vector3D *ipNorm);
    static void findAverage(
        int vectorCount, Vector3D *vectors, Vector3D *center, DBL *radius);
    static int sphericalBoundsCheck(Ray *ray, Vector3D *center, DBL radius);
    static int intersectBicubicPatch0(Ray *ray, BicubicPatch *shape, DBL *depths);
    static int intersectBicubicPatch1(Ray *ray, BicubicPatch *shape, DBL *depths);
    static int intersectBicubicPatch2(Ray *ray, BicubicPatch *shape, DBL *depths);
    static int intersectBicubicPatch3(Ray *ray, BicubicPatch *shape, DBL *depths);
    static int intersectBicubicPatch4(Ray *ray, BicubicPatch *shape, DBL *depths);
    static DBL pointPlaneDistance(Vector3D *point, Vector3D *normal, DBL *d);
    static DBL determineSubpatchFlatness(Vector3D (*patch)[4][4]);
    static int flatEnough(BicubicPatch *shape, Vector3D (*patch)[4][4]);
    static void bezierBoundingSphere(
        Vector3D (*patch)[4][4], Vector3D *center, DBL *radiusSquared);
    static void bezierSubpatchIntersect(Ray *ray, BicubicPatch *shape,
        Vector3D (*patch)[4][4], DBL u0, DBL u1, DBL v0, int recursionDepth,
        int *depthCount, DBL *depths, DBL *u, DBL *v);
    static void bezierSplitLeftRight(
        Vector3D (*patch)[4][4], Vector3D (*left)[4][4], Vector3D (*right)[4][4]);
    static void bezierSplitUpDown(
        Vector3D (*patch)[4][4], Vector3D (*lower)[4][4], Vector3D (*upper)[4][4]);
    static void bezierSubdivider(Ray *ray, BicubicPatch *shape, Vector3D (*patch)[4][4],
        DBL u0, DBL u1, DBL v0, DBL v1, int recursionDepth, int *depthCount,
        DBL *depths, DBL *u, DBL *v);
    static void bezierTreeDeleter(BezierNode *node);
    static BezierNode *bezierTreeBuilder(
        BicubicPatch *shape, Vector3D (*patch)[4][4], int depth);
    static void bezierTreeWalker(
        Ray *ray, BicubicPatch *shape, BezierNode *node, int depth, int *depthCount, DBL *depths);
    static BezierNode *createNewBezierNode();
    static BezierVertices *createBezierVertexBlock();
    static BezierChild *createBezierChildBlock();
};

#endif
