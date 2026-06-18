#ifndef __MATERIAL__
#define __MATERIAL__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Material {
  public:
    virtual ~Material() = default;

    virtual Material *copy() = 0;
    virtual void translate(Vector3Dd *vector) = 0;
    virtual void rotate(Vector3Dd *vector) = 0;
    virtual void scale(Vector3Dd *vector) = 0;
    virtual double getObjectIndexOfRefraction() const = 0;
};

#endif
