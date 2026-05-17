#ifndef __POINT_H__
#define __POINT_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Light : public Geometry {
  public:
    Vector3Dd Center, Points_At;
    Light *Next_Light_Source;
    short Inverted;
    double Coeff, Radius, Falloff;

    static int allPointIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int insidePoint(Vector3Dd *testPoint, SimpleBody *object);
    static void *copyPoint(SimpleBody *object);
    static void translatePoint(SimpleBody *object, Vector3Dd *vector);
    static void rotatePoint(SimpleBody *object, Vector3Dd *vector);
    static void scalePoint(SimpleBody *object, Vector3Dd *vector);
    static void invertPoint(SimpleBody *object);
    static double attenuateLight(Light *lightSource, Ray *lightSourceRay);

  private:
    static double cubicSpline(double low, double high, double pos);
};

extern Methods Point_Methods;
extern Light *getLightSourceShape(void);

#endif
