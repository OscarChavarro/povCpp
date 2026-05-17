#ifndef __BOX_H__
#define __BOX_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Box : public Geometry {
  public:
    Transformation *Transform;
    Vector3D bounds[2];
    short Inverted;

    static int allBoxIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int intersectBoxx(Ray *ray, Box *box, double *depth1, double *depth2);
    static int insideBox(Vector3D *point, SimpleBody *object);
    static void boxNormal(
        Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint);
    static void *copyBox(SimpleBody *object);
    static void translateBox(SimpleBody *object, Vector3D *vector);
    static void rotateBox(SimpleBody *object, Vector3D *vector);
    static void scaleBox(SimpleBody *object, Vector3D *vector);
    static void invertBox(SimpleBody *object);

  private:
    static int closeTo(double x, double y);
};

extern Methods Box_Methods;
extern Box *getBoxShape();

#endif
