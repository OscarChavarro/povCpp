#include "render/shaders/ExponentialFogShader.h"
#include "common/color/Color.h"
#include <cmath>

void
ExponentialFogShader::shade(double distance, RGBAColor *fogColour, double fogDistance,
    RGBAColor *colour)
{
    double fogFactor, fogFactorInverse;

    fogFactor = exp(-1.0 * distance / fogDistance);
    fogFactorInverse = 1.0 - fogFactor;
    colour->Red = colour->Red * fogFactor + fogColour->Red * fogFactorInverse;
    colour->Green =
        colour->Green * fogFactor + fogColour->Green * fogFactorInverse;
    colour->Blue =
        colour->Blue * fogFactor + fogColour->Blue * fogFactorInverse;
}
