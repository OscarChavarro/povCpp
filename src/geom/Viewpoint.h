#ifndef __VIEWPNT_H__
#define __VIEWPNT_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/Vector3D.h"
#include "geom/GeometryOperations.h"

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
