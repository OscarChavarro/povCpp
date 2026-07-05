#ifndef __MATERIAL__
#define __MATERIAL__

class Material {
  public:
    virtual ~Material() {}

    virtual Material *translate(Vector3Dd *vector) = 0;
    virtual Material *rotate(Vector3Dd *vector) = 0;
    virtual Material *scale(Vector3Dd *vector) = 0;
    virtual Material *copy() const = 0;
};

#endif
