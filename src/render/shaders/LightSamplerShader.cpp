#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/light/Light.h"
#include "render/shaders/LightSamplerShader.h"

void
LightSamplerShader::sample(const Light *lightSource, double *lightSourceDepth,
    RayWithSegments *lightSourceRay, const Vector3Dd *intersectionPoint,
    ColorRgba *lightColor)
{
    double attenuation = 1.0;

    // Get the light source color
    if ( lightSource->shapeColor == 0 ) {
        lightColor->setR(1.0); lightColor->setG(1.0); lightColor->setB(1.0); lightColor->setA(0);
    }
    else {
        *lightColor = *lightSource->shapeColor;
    }

    lightSourceRay->position = *intersectionPoint;
    lightSourceRay->quadricConstantsCached = false;

    lightSourceRay->direction =
        lightSource->Center.subtract(*intersectionPoint);

    *lightSourceDepth = lightSourceRay->direction.length();

    lightSourceRay->direction =
        lightSourceRay->direction.multiply(1.0 / (*lightSourceDepth));

    attenuation = Light::attenuateLight(lightSource, lightSourceRay);

    // Now scale the color by the attenuation
    lightColor->setR(lightColor->getR() * attenuation);
    lightColor->setG(lightColor->getG() * attenuation);
    lightColor->setB(lightColor->getB() * attenuation);
}
#include "java/util/PriorityQueue.txx"

