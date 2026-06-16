#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/TransformableElement.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/GeometryTypes.h"

class Camera : public TransformableElement {
  public:
    GeometryTypes type;
    Vector3Dd location;
    Vector3Dd direction;
    Vector3Dd up;
    Vector3Dd right;
    Vector3Dd sky;

    void initializeDefaults();

    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
};

#endif
