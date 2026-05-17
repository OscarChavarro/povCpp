#ifndef __QUADRICS_H__
#define __QUADRICS_H__

#include "common/Frame.h"
#include "app/PovApp.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Quadric : public Geometry {
  public:
    Vector3D Object_2_Terms;
    Vector3D Object_Mixed_Terms;
    Vector3D Object_Terms;
    double Object_Constant;
    double Object_VP_Constant;
    int Constant_Cached;
    int Non_Zero_Square_Term;

    static int allQuadricIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int intersectQuadric(
        Ray *ray, Quadric *shape, double *depth1, double *depth2);
    static int insideQuadric(Vector3D *point, SimpleBody *object);
    static void quadricNormal(
        Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint);
    static void *copyQuadric(SimpleBody *object);
    static void translateQuadric(SimpleBody *object, Vector3D *vector);
    static void rotateQuadric(SimpleBody *object, Vector3D *vector);
    static void scaleQuadric(SimpleBody *object, Vector3D *vector);
    static void invertQuadric(SimpleBody *object);

  private:
    static void quadricToMatrix(Quadric *quadric, MATRIX *matrix);
    static void matrixToQuadric(MATRIX *matrix, Quadric *quadric);
    static void transformQuadric(Quadric *shape, Transformation *transformation);
};

extern Methods Quadric_Methods;
extern Quadric *getQuadricShape(void);
#endif
