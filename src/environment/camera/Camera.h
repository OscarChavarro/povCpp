#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/TransformableElement.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/GeometryTypes.h"

class Camera : public TransformableElement {
  public:
    GeometryTypes Type;
    Vector3Dd Location;
    Vector3Dd Direction;
    Vector3Dd Up;
    Vector3Dd Right;
    Vector3Dd Sky;

    void initializeDefaults();

    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
};

#endif
