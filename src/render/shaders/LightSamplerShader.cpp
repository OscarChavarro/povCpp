#include "render/shaders/LightSamplerShader.h"
#include "common/color/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/light/Light.h"

void
LightSamplerShader::sample(Light *lightSource, double *lightSourceDepth,
    RayWithSegments *lightSourceRay, Vector3Dd *intersectionPoint,
    RGBAColor *lightColour)
{
    double attenuation = 1.0;

    /* Get the light source colour. */
    if ( lightSource->Shape_Colour == 0 ) {
        Color::makeColor(lightColour, 1.0, 1.0, 1.0);
    }
    else {
        *lightColour = *lightSource->Shape_Colour;
    }

    lightSourceRay->position = *intersectionPoint;
    lightSourceRay->quadricConstantsCached = FALSE;

    VectorOps::vSub(lightSourceRay->direction, lightSource->Center, *intersectionPoint);

    *lightSourceDepth = lightSourceRay->direction.length();

    VectorOps::vScale(
        lightSourceRay->direction, lightSourceRay->direction, 1.0 / (*lightSourceDepth));

    attenuation = Light::attenuateLight(lightSource, lightSourceRay);

    /* Now scale the color by the attenuation */
    lightColour->Red *= attenuation;
    lightColour->Green *= attenuation;
    lightColour->Blue *= attenuation;
}

