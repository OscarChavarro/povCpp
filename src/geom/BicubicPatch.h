#ifndef __BICUBIC_PATCH_H__
#define __BICUBIC_PATCH_H__

#include "common/FrameConfig.h"
#include "common/Vector3D.h"
#include "geom/BezierNode.h"
#include "geom/GeometryOps.h"

static constexpr int MAX_BICUBIC_INTERSECTIONS = 32;
class BezierVertices;
class BezierChild;
class BezierPatches;
class BezierIntersection;

class BicubicPatch : public Geometry {
  friend class BezierPatches;
  friend class BezierIntersection;
  public:
    int Patch_Type, U_Steps, V_Steps;
    Vector3D Control_Points[4][4];
    Vector3D Bounding_Sphere_Center;
    double Bounding_Sphere_Radius;
    double Flatness_Value;
    int Intersection_Count;
    Vector3D Normal_Vector[MAX_BICUBIC_INTERSECTIONS];
    Vector3D Intersection_Point[MAX_BICUBIC_INTERSECTIONS];
    Vector3D **Interpolated_Grid, **Interpolated_Normals, **Smooth_Normals;
    double **Interpolated_D;
    BezierNode *Node_Tree;

    static BicubicPatch *getBicubicPatchShape();
    static void precomputePatchValues(BicubicPatch *shape);
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
        Vector3D *result, double u, double v, Vector3D (*controlPoints)[4][4]);
    static void bezierPartial(Vector3D *result, double u, double v, BicubicPatch *shape);
    static int subpatchNormal(
        Vector3D *v1, Vector3D *v2, Vector3D *v3, Vector3D *result, double *d);
    static int intersectSubpatch(int patchType, Ray *ray, Vector3D *v1,
        Vector3D *v2, Vector3D *v3, Vector3D *n, double d, Vector3D *n1, Vector3D *n2,
        Vector3D *n3, double *depth, Vector3D *ip, Vector3D *ipNorm);
    static void findAverage(
        int vectorCount, Vector3D *vectors, Vector3D *center, double *radius);
    static double pointPlaneDistance(Vector3D *point, Vector3D *normal, double *d);
    static double determineSubpatchFlatness(Vector3D (*patch)[4][4]);
    static int flatEnough(BicubicPatch *shape, Vector3D (*patch)[4][4]);
    static void bezierBoundingSphere(
        Vector3D (*patch)[4][4], Vector3D *center, double *radiusSquared);
    static void bezierSubpatchIntersect(Ray *ray, BicubicPatch *shape,
        Vector3D (*patch)[4][4], double u0, double u1, double v0, int recursionDepth,
        int *depthCount, double *depths, double *u, double *v);
    static void bezierSplitLeftRight(
        Vector3D (*patch)[4][4], Vector3D (*left)[4][4], Vector3D (*right)[4][4]);
    static void bezierSplitUpDown(
        Vector3D (*patch)[4][4], Vector3D (*lower)[4][4], Vector3D (*upper)[4][4]);
    static void bezierSubdivider(Ray *ray, BicubicPatch *shape, Vector3D (*patch)[4][4],
        double u0, double u1, double v0, double v1, int recursionDepth, int *depthCount,
        double *depths, double *u, double *v);
    static void bezierTreeDeleter(BezierNode *node);
    static BezierNode *bezierTreeBuilder(
        BicubicPatch *shape, Vector3D (*patch)[4][4], int depth);
    static void bezierTreeWalker(
        Ray *ray, BicubicPatch *shape, BezierNode *node, int depth, int *depthCount, double *depths);
    static BezierNode *createNewBezierNode();
    static BezierVertices *createBezierVertexBlock();
    static BezierChild *createBezierChildBlock();
};

#endif
