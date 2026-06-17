#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Camera {
  public:
    Vector3Dd location;
    Vector3Dd direction;
    Vector3Dd up;
    Vector3Dd right;
    Vector3Dd sky;

    void initializeDefaults();
    void applyLinearTransformation(const Matrix4x4d &transformation);

    void *copy();
    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
};

#endif
