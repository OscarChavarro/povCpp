#ifndef __QUADRIC_H__
#define __QUADRIC_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "geom/GeometryOperations.h"

class Quadric : public Geometry {
  public:
    Vector3Dd Object_2_Terms;
    Vector3Dd Object_Mixed_Terms;
    Vector3Dd Object_Terms;
    double Object_Constant;
    double Object_VP_Constant;
    int Constant_Cached;
    int Non_Zero_Square_Term;

    static int allQuadricIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int intersectQuadric(
        Ray *ray, Quadric *shape, double *depth1, double *depth2);
    static int insideQuadric(Vector3Dd *point, SimpleBody *object);
    static void quadricNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copyQuadric(SimpleBody *object);
    static void translateQuadric(SimpleBody *object, Vector3Dd *vector);
    static void rotateQuadric(SimpleBody *object, Vector3Dd *vector);
    static void scaleQuadric(SimpleBody *object, Vector3Dd *vector);
    static void invertQuadric(SimpleBody *object);

  private:
    static void quadricToMatrix(Quadric *quadric, MATRIX *matrix);
    static void matrixToQuadric(MATRIX *matrix, Quadric *quadric);
    static void transformQuadric(Quadric *shape, Transformation *transformation);
};

extern Methods Quadric_Methods;
extern Quadric *getQuadricShape(void);
#endif
