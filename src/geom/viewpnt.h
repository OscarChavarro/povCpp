#ifndef __VIEWPNT_H__
#define __VIEWPNT_H__

#include "common/frame.h"
#include "common/povproto.h"
#include "common/vector.h"
#include "geom/geometry.h"

class Viewpoint {
  public:
    Methods *methods;
    int Type;
    Vector3D Location;
    Vector3D Direction;
    Vector3D Up;
    Vector3D Right;
    Vector3D Sky;
};

extern Methods Viewpoint_Methods;
extern Viewpoint *Get_Viewpoint(void);
extern void *Copy_Viewpoint(SimpleBody *Object);
extern void Translate_Viewpoint(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Viewpoint(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Viewpoint(SimpleBody *Object, Vector3D *Vector);

#endif
