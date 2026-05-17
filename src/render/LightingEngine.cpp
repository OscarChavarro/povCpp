/****************************************************************************
 *                     LightingEngine.cpp
 *
 *  This module implements the lighting engine for ray tracing.
 *
 ****************************************************************************/

#include "render/LightingEngine.h"
#include "common/Color.h"
#include "common/Vector.h"
#include "common/VectorOps.h"
#include "geom/Light.h"
#include "geom/Intersection.h"
#include "geom/PrioQ.h"
#include "render/Render.h"

void
LightingEngine::doLight(Light *lightSource, double *lightSourceDepth, Ray *lightSourceRay,
    Vector3D *intersectionPoint, RGBAColor *lightColour)
{
    double attenuation = 1.0;

    /* Get the light source colour. */
    if (lightSource->Shape_Colour == nullptr) {
        Color::makeColor(lightColour, 1.0, 1.0, 1.0);
    } else {
        *lightColour = *lightSource->Shape_Colour;
    }

    lightSourceRay->Initial = *intersectionPoint;
    lightSourceRay->Quadric_Constants_Cached = FALSE;

    VectorOps::vSub(lightSourceRay->Direction, lightSource->Center, *intersectionPoint);

    VectorOps::vLength(*lightSourceDepth, lightSourceRay->Direction);

    VectorOps::vScale(lightSourceRay->Direction, lightSourceRay->Direction,
        1.0 / (*lightSourceDepth));

    attenuation = Light::attenuateLight(lightSource, lightSourceRay);

    /* Now scale the color by the attenuation */
    lightColour->Red *= attenuation;
    lightColour->Green *= attenuation;
    lightColour->Blue *= attenuation;
}

int
LightingEngine::doBlocking(Intersection *localIntersection, RGBAColor *lightColour,
    PriorityQueueNode *localQueue)
{
    extern void determineSurfaceColour(Intersection *Ray_Intersection,
        RGBAColor *Colour, Ray *Ray, int Shadow_Ray);

    determineSurfaceColour(localIntersection, lightColour, nullptr, TRUE);

    if ((lightColour->Red < 0.01) && (lightColour->Green < 0.01) &&
        (lightColour->Blue < 0.01)) {

        while (localQueue->getHighest()) {
            localQueue->deleteHighest();
        }
        return TRUE;
    }
    return FALSE;
}

void
LightingEngine::doPhong(Texture *texture, Ray *lightSourceRay, Vector3D eye,
    Vector3D *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
    RGBAColor *surfaceColour)
{
    double cosAngleOfIncidence, normalLength, intensity;
    Vector3D localNormal;
    Vector3D normalProjection;
    Vector3D reflectDirection;

    VectorOps::vDot(cosAngleOfIncidence, eye, *surfaceNormal);

    if (cosAngleOfIncidence < 0.0) {
        localNormal = *surfaceNormal;
        cosAngleOfIncidence = -cosAngleOfIncidence;
    } else {
        VectorOps::vScale(localNormal, *surfaceNormal, -1.0);
    }

    VectorOps::vScale(normalProjection, localNormal, cosAngleOfIncidence);
    VectorOps::vScale(normalProjection, normalProjection, 2.0);
    VectorOps::vAdd(reflectDirection, eye, normalProjection);

    VectorOps::vDot(cosAngleOfIncidence, reflectDirection, lightSourceRay->Direction);
    VectorOps::vLength(normalLength, lightSourceRay->Direction);

    if (normalLength == 0.0) {
        cosAngleOfIncidence = 0.0;
    } else {
        cosAngleOfIncidence /= normalLength;
    }

    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = 0;
    }

    if (texture->Object_PhongSize != 1.0) {
        intensity = pow(cosAngleOfIncidence, texture->Object_PhongSize);
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->Object_Phong;

    if (texture->Metallic_Flag) {
        colour->Red += intensity * (surfaceColour->Red);
        colour->Green += intensity * (surfaceColour->Green);
        colour->Blue += intensity * (surfaceColour->Blue);
    } else {
        colour->Red += intensity * (lightColour->Red);
        colour->Green += intensity * (lightColour->Green);
        colour->Blue += intensity * (lightColour->Blue);
    }
}

void
LightingEngine::doSpecular(Texture *texture, Ray *lightSourceRay, Vector3D rEye,
    Vector3D *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
    RGBAColor *surfaceColour)
{
    double cosAngleOfIncidence, normalLength, intensity, halfwayLength, roughness;
    Vector3D halfway;

    VectorOps::vHalf(halfway, rEye, lightSourceRay->Direction);
    VectorOps::vLength(normalLength, *surfaceNormal);
    VectorOps::vLength(halfwayLength, halfway);
    VectorOps::vDot(cosAngleOfIncidence, halfway, *surfaceNormal);

    if (normalLength == 0.0 || halfwayLength == 0.0) {
        cosAngleOfIncidence = 0.0;
    } else {
        cosAngleOfIncidence /= (normalLength * halfwayLength);
    }

    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = 0.0;
    }

    roughness = 1.0 / texture->Object_Roughness;

    if (roughness != 1.0) {
        intensity = pow(cosAngleOfIncidence, roughness);
    } else {
        intensity = cosAngleOfIncidence;
    }
    intensity *= texture->Object_Specular;
    if (texture->Metallic_Flag) {
        colour->Red += intensity * (surfaceColour->Red);
        colour->Green += intensity * (surfaceColour->Green);
        colour->Blue += intensity * (surfaceColour->Blue);
    } else {
        colour->Red += intensity * (lightColour->Red);
        colour->Green += intensity * (lightColour->Green);
        colour->Blue += intensity * (lightColour->Blue);
    }
}

void
LightingEngine::doDiffuse(Texture *texture, Ray *lightSourceRay, Vector3D *surfaceNormal,
    RGBAColor *colour, RGBAColor *lightColour, RGBAColor *surfaceColour,
    double attenuation)
{
    double cosAngleOfIncidence, intensity, randomNumber;

    VectorOps::vDot(cosAngleOfIncidence, *surfaceNormal, lightSourceRay->Direction);
    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = -cosAngleOfIncidence;
    }

    if (texture->Object_Brilliance != 1.0) {
        intensity = pow(cosAngleOfIncidence, texture->Object_Brilliance);
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->Object_Diffuse * attenuation;

    randomNumber = (rand() & 0x7FFF) / (double)0x7FFF;

    intensity -= randomNumber * texture->Texture_Randomness;

    colour->Red += intensity * (surfaceColour->Red) * (lightColour->Red);
    colour->Green += intensity * (surfaceColour->Green) * (lightColour->Green);
    colour->Blue += intensity * (surfaceColour->Blue) * (lightColour->Blue);
}

void
LightingEngine::fog(double distance, RGBAColor *fogColour, double fogDistance, RGBAColor *colour)
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
