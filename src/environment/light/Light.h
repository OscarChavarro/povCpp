#ifndef __POINT_H__
#define __POINT_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Light : public Geometry {
  public:
    static Methods methodTable;
    Vector3Dd Center;
    Vector3Dd pointsAt;
    Light *Next_Light_Source;
    bool Inverted;
    double Coeff;
    double Radius;
    double Falloff;

    static int allPointIntersections(SimpleBody *object, RayWithSegments *ray,
        PriorityQueueNode *depthQueue);
    static int insidePoint(Vector3Dd *testPoint, SimpleBody *object);
    static void *copyPoint(SimpleBody *object);
    static void translatePoint(SimpleBody *object, Vector3Dd *vector);
    static void rotatePoint(SimpleBody *object, Vector3Dd *vector);
    static void scalePoint(SimpleBody *object, Vector3Dd *vector);
    static void invertPoint(SimpleBody *object);
    static double attenuateLight(
        const Light *lightSource, const RayWithSegments *lightSourceRay);

  private:
    static double cubicSpline(double low, double high, double pos);
};

#endif
