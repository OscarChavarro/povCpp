#ifndef __VIEWPNT_H__
#define __VIEWPNT_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/Vector3Dd.h"
#include "geom/GeometryOperations.h"

class Viewpoint {
  public:
    Methods *methods;
    int Type;
    Vector3Dd Location;
    Vector3Dd Direction;
    Vector3Dd Up;
    Vector3Dd Right;
    Vector3Dd Sky;

    void initializeDefaults();
    static void *copyViewpoint(SimpleBody *object);
    static void translateViewpoint(SimpleBody *object, Vector3Dd *vector);
    static void rotateViewpoint(SimpleBody *object, Vector3Dd *vector);
    static void scaleViewpoint(SimpleBody *object, Vector3Dd *vector);
};

extern Methods Viewpoint_Methods;
extern Viewpoint *getViewpoint(void);

#endif
