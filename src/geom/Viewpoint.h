#ifndef __VIEWPNT_H__
#define __VIEWPNT_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Viewpoint {
  public:
    Methods *methods;
    int Type;
    Vector3D Location;
    Vector3D Direction;
    Vector3D Up;
    Vector3D Right;
    Vector3D Sky;

    void initializeDefaults();
    static void *copyViewpoint(SimpleBody *object);
    static void translateViewpoint(SimpleBody *object, Vector3D *vector);
    static void rotateViewpoint(SimpleBody *object, Vector3D *vector);
    static void scaleViewpoint(SimpleBody *object, Vector3D *vector);
};

extern Methods Viewpoint_Methods;
extern Viewpoint *getViewpoint(void);

#endif
