#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/light/Light.h"
#include "render/shaders/LightSamplerShader.h"

void
LightSamplerShader::sample(const Light *lightSource, double *lightSourceDepth,
    RayWithSegments *lightSourceRay, const Vector3Dd *intersectionPoint,
    ColorRgba *lightColor)
{
    double attenuation = 1.0;

    // Get the light source color
    if (lightSource->getShapeColor() == 0) {
        lightColor->setR(1.0); lightColor->setG(1.0); lightColor->setB(1.0); lightColor->setA(0);
    }
    else {
        *lightColor = *lightSource->getShapeColor();
    }

    lightSourceRay->setOrigin(*intersectionPoint);
    lightSourceRay->quadricConstantsCached = false;

    lightSourceRay->setDirection(
        lightSource->getCenter().subtract(*intersectionPoint));

    *lightSourceDepth = lightSourceRay->getDirection().length();

    lightSourceRay->setDirection(
        lightSourceRay->getDirection().multiply(1.0 / (*lightSourceDepth)));

    attenuation = lightSource->evaluateLightResponseFactor(lightSourceRay);

    // Now scale the color by the attenuation
    lightColor->setR(lightColor->getR() * attenuation);
    lightColor->setG(lightColor->getG() * attenuation);
    lightColor->setB(lightColor->getB() * attenuation);
}
