#include "environment/light/PointLight.h"

PointLight::PointLight()
{
    this->center = Vector3Dd(0.0, 0.0, 0.0);
    this->pointsAt = Vector3Dd(0.0, 0.0, 1.0);
    this->inverted = false;
    this->coefficient = 10.0;
    this->radius = 0.35;
    this->falloff = 0.35;
    this->nextLightSource = nullptr;
}

double
PointLight::attenuate(const RayWithSegments *lightSourceRay) const
{
    return 1.0;
}

PointLight *
PointLight::copy()
{
    PointLight * const newLight = new PointLight();
    if (newLight == nullptr) {
        return nullptr;
    }
    copyStateInto(newLight);
    return newLight;
}
