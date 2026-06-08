#ifndef __BOX_H__
#define __BOX_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/GeometryOperations.h"

class Box : public Geometry {
  public:
    static Methods methodTable;
    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    Vector3Dd bounds[2];
    bool Inverted;

    static int allBoxIntersections(SimpleBody *object, RayWithSegments *ray,
        PriorityQueueNode *depthQueue);
    static int intersectBoxx(
        RayWithSegments *ray, Box *box, double *depth1, double *depth2);
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

#endif
