#include "render/shaders/ExponentialFogShader.h"
#include "common/color/Color.h"
#include <cmath>

void
ExponentialFogShader::shade(double distance, RGBAColor *fogColor, double fogDistance,
    RGBAColor *color)
{
    double fogFactor;
    double fogFactorInverse;

    fogFactor = exp(-1.0 * distance / fogDistance);
    fogFactorInverse = 1.0 - fogFactor;
    color->Red = color->Red * fogFactor + fogColor->Red * fogFactorInverse;
    color->Green =
        color->Green * fogFactor + fogColor->Green * fogFactorInverse;
    color->Blue =
        color->Blue * fogFactor + fogColor->Blue * fogFactorInverse;
}
