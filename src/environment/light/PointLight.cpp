#include "environment/light/PointLight.h"

PointLight::PointLight()
{
    this->getCenter() = Vector3Dd(0.0, 0.0, 0.0);
    this->getPointsAt() = Vector3Dd(0.0, 0.0, 1.0);
    this->setInverted(false);
    this->setCoefficient(10.0);
    this->setRadius(0.35);
    this->setFalloff(0.35);
    this->setNextLightSource(nullptr);
}

double
PointLight::evaluateLightResponseFactor(const Ray *lightSourceRay) const
{
    (void)lightSourceRay;
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
