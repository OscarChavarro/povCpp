#ifndef __POINT_H__
#define __POINT_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Light : public Geometry {
  public:
    Vector3D Center, Points_At;
    Light *Next_Light_Source;
    short Inverted;
    double Coeff, Radius, Falloff;

    static int allPointIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int insidePoint(Vector3D *testPoint, SimpleBody *object);
    static void *copyPoint(SimpleBody *object);
    static void translatePoint(SimpleBody *object, Vector3D *vector);
    static void rotatePoint(SimpleBody *object, Vector3D *vector);
    static void scalePoint(SimpleBody *object, Vector3D *vector);
    static void invertPoint(SimpleBody *object);
    static double attenuateLight(Light *lightSource, Ray *lightSourceRay);

  private:
    static double cubicSpline(double low, double high, double pos);
};

extern Methods Point_Methods;
extern Light *getLightSourceShape(void);

#endif
