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
};

extern Methods Viewpoint_Methods;
extern Viewpoint *getViewpoint(void);
extern void *copyViewpoint(SimpleBody *Object);
extern void translateViewpoint(SimpleBody *Object, Vector3D *Vector);
extern void rotateViewpoint(SimpleBody *Object, Vector3D *Vector);
extern void scaleViewpoint(SimpleBody *Object, Vector3D *Vector);

#endif
