#ifndef __BOX_H__
#define __BOX_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Box : public Geometry {
  public:
    Transformation *Transform;
    Vector3Dd bounds[2];
    short Inverted;

    static int allBoxIntersections(
        SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue);
    static int intersectBoxx(RayWithSegments *ray, Box *box, double *depth1, double *depth2);
    static int insideBox(Vector3Dd *point, SimpleBody *object);
    static void boxNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copyBox(SimpleBody *object);
    static void translateBox(SimpleBody *object, Vector3Dd *vector);
    static void rotateBox(SimpleBody *object, Vector3Dd *vector);
    static void scaleBox(SimpleBody *object, Vector3Dd *vector);
    static void invertBox(SimpleBody *object);

  private:
    static int closeTo(double x, double y);
};

extern Methods Box_Methods;
extern Box *getBoxShape();

#endif
