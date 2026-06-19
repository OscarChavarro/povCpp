#ifndef __CAMERA__
#define __CAMERA__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Camera {
  private:
    Vector3Dd location;
    Vector3Dd direction;
    Vector3Dd up;
    Vector3Dd right;
    Vector3Dd sky;

  public:
    Camera();
    Camera(const Vector3Dd &location, const Vector3Dd &direction,
        const Vector3Dd &up, const Vector3Dd &right, const Vector3Dd &sky);

    Vector3Dd& getLocation() { return location; }
    const Vector3Dd& getLocation() const { return location; }
    Vector3Dd& getDirection() { return direction; }
    const Vector3Dd& getDirection() const { return direction; }
    Vector3Dd& getUp() { return up; }
    const Vector3Dd& getUp() const { return up; }
    Vector3Dd& getRight() { return right; }
    const Vector3Dd& getRight() const { return right; }
    Vector3Dd& getSky() { return sky; }
    const Vector3Dd& getSky() const { return sky; }

    void applyLinearTransformation(const Matrix4x4d &transformation);

    void *copy();
    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
};

#endif
