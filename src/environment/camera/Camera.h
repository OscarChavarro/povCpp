#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/GeometryTypes.h"

class Camera {
  public:
    static Methods methodTable;
    Methods *methods;
    GeometryTypes Type;
    Vector3Dd Location;
    Vector3Dd Direction;
    Vector3Dd Up;
    Vector3Dd Right;
    Vector3Dd Sky;

    void initializeDefaults();
    static void *copyCamera(SimpleBody *object);
    static void translateCamera(SimpleBody *object, Vector3Dd *vector);
    static void rotateCamera(SimpleBody *object, Vector3Dd *vector);
    static void scaleCamera(SimpleBody *object, Vector3Dd *vector);
};

#endif
