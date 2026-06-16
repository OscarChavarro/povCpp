/**
light.c

This module implements the point light source primitive.
*/

#include "environment/light/PointLight.h"

#include "environment/material/MaterialUtils.h"

PointLight::PointLight()
{
    this->center = Vector3Dd(0.0, 0.0, 0.0);
    this->pointsAt = Vector3Dd(0.0, 0.0, 1.0);
    this->geometryType = GeometryTypes::POINT_LIGHT_TYPE;
    this->inverted = false;
    this->coeff = 10.0;
    this->radius = 0.35;
    this->falloff = 0.35;
    this->nextLightSource = nullptr;
}

double
PointLight::attenuate(const RayWithSegments *lightSourceRay) const
{
    return 1.0;
}

void *
PointLight::copy()
{
    PointLight * const newLight = new PointLight();
    if (newLight == nullptr) {
        return nullptr;
    }
    copyStateInto(newLight);
    return newLight;
}
#include "java/util/PriorityQueue.txx"
