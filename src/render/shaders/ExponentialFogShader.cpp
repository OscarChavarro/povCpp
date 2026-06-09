#include "render/shaders/ExponentialFogShader.h"
#include "common/color/Color.h"
#include <cmath>

void
ExponentialFogShader::shade(double distance, ColorRgba *fogColor, double fogDistance,
    ColorRgba *color)
{
    double fogFactor;
    double fogFactorInverse;

    fogFactor = exp(-1.0 * distance / fogDistance);
    fogFactorInverse = 1.0 - fogFactor;
    color->setR(color->getR() * fogFactor + fogColor->getR() * fogFactorInverse);
    color->setG(
        color->getG() * fogFactor + fogColor->getG() * fogFactorInverse);
    color->setB(
        color->getB() * fogFactor + fogColor->getB() * fogFactorInverse);
}
