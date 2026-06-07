#include "render/shaders/LightSamplerShader.h"
#include "common/color/Color.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/linealAlgebra/Vector3DdOps.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/light/Light.h"

void
LightSamplerShader::sample(Light *lightSource, double *lightSourceDepth,
    RayWithSegments *lightSourceRay, Vector3Dd *intersectionPoint,
    RGBAColor *lightColor)
{
    double attenuation = 1.0;

    /* Get the light source color. */
    if ( lightSource->Shape_Colour == 0 ) {
        Color::makeColor(lightColor, 1.0, 1.0, 1.0);
    }
    else {
        *lightColor = *lightSource->Shape_Colour;
    }

    lightSourceRay->position = *intersectionPoint;
    lightSourceRay->quadricConstantsCached = false;

    lightSourceRay->direction =
        lightSource->Center.subtract(*intersectionPoint);

    *lightSourceDepth = lightSourceRay->direction.length();

    lightSourceRay->direction =
        Vec3::scaled(lightSourceRay->direction, 1.0 / (*lightSourceDepth));

    attenuation = Light::attenuateLight(lightSource, lightSourceRay);

    /* Now scale the color by the attenuation */
    lightColor->Red *= attenuation;
    lightColor->Green *= attenuation;
    lightColor->Blue *= attenuation;
}

