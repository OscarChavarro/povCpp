#ifndef __LIGHT__
#define __LIGHT__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

class Light {
  private:
    Vector3Dd position;
    ColorRgb emission;
    int id;
    java::String name;

  protected:
    Light(const Vector3Dd &position, const ColorRgb &emission);
    Light(const Light &other);

  public:
    virtual ~Light() {}

    const java::String &getName() const;
    void setName(const java::String &name);
    int getId() const;
    void setId(int id);

    const Vector3Dd &getPosition() const;
    void setPosition(const Vector3Dd &position);
    const ColorRgb &getEmission() const;
    void setEmission(const ColorRgb &emission);

    virtual bool isAmbient() const;

    virtual void getDirectionAndDistance(const Vector3Dd &surfacePoint,
        Vector3Dd *directionOut, double *maxShadowDistanceOut) const = 0;

    virtual double evaluateLightResponseFactor(const Ray *lightSourceRay) const = 0;

    virtual Light *copy() const = 0;
};

#endif
