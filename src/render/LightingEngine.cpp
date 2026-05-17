/****************************************************************************
 *                     LightingEngine.cpp
 *
 *  This module implements the lighting engine for ray tracing.
 *
 ****************************************************************************/

#include "render/LightingEngine.h"
#include "common/Color.h"
#include "common/FrameConfig.h"
#include "common/Transformation.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/light/Light.h"
#include "environment/geometry/Intersection.h"
#include "common/dataStructures/PriorityQueue.h"
#include "media/BumpTextureFixture.h"
#include "media/ColorTextureFixture.h"
#include "media/MapTextureFixture.h"
#include "media/TextureFixture.h"
#include "render/RenderEngine.h"
#include "render/RenderFrame.h"

extern int traceLevel;
extern RenderFrame globalFrame;
extern unsigned int Options;
extern int quality;
extern int shadowTestFlag;
extern long shadowRayTests, shadowRaysSucceeded;
extern long reflectedRaysTraced, refractedRaysTraced;
extern long transmittedRaysTraced;

static constexpr double SHADOW_TOLERANCE = 0.05;

void
LightingEngine::doLight(Light *lightSource, double *lightSourceDepth, RayWithSegments *lightSourceRay,
    Vector3Dd *intersectionPoint, RGBAColor *lightColour)
{
    double attenuation = 1.0;

    /* Get the light source colour. */
    if (lightSource->Shape_Colour == nullptr) {
        Color::makeColor(lightColour, 1.0, 1.0, 1.0);
    } else {
        *lightColour = *lightSource->Shape_Colour;
    }

    lightSourceRay->position = *intersectionPoint;
    lightSourceRay->quadricConstantsCached = FALSE;

    VectorOps::vSub(lightSourceRay->direction, lightSource->Center, *intersectionPoint);

    *lightSourceDepth = lightSourceRay->direction.length();

    VectorOps::vScale(lightSourceRay->direction, lightSourceRay->direction,
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
LightingEngine::doPhong(Texture *texture, RayWithSegments *lightSourceRay, Vector3Dd eye,
    Vector3Dd *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
    RGBAColor *surfaceColour)
{
    double cosAngleOfIncidence, normalLength, intensity;
    Vector3Dd localNormal;
    Vector3Dd normalProjection;
    Vector3Dd reflectDirection;

    cosAngleOfIncidence = eye.dotProduct(*surfaceNormal);

    if (cosAngleOfIncidence < 0.0) {
        localNormal = *surfaceNormal;
        cosAngleOfIncidence = -cosAngleOfIncidence;
    } else {
        VectorOps::vScale(localNormal, *surfaceNormal, -1.0);
    }

    VectorOps::vScale(normalProjection, localNormal, cosAngleOfIncidence);
    normalProjection.scale(2.0);
    VectorOps::vAdd(reflectDirection, eye, normalProjection);

    cosAngleOfIncidence = reflectDirection.dotProduct(lightSourceRay->direction);
    normalLength = lightSourceRay->direction.length();

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
LightingEngine::doSpecular(Texture *texture, RayWithSegments *lightSourceRay, Vector3Dd rEye,
    Vector3Dd *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
    RGBAColor *surfaceColour)
{
    double cosAngleOfIncidence, normalLength, intensity, halfwayLength, roughness;
    Vector3Dd halfway;

    halfway = rEye.half(lightSourceRay->direction);
    normalLength = (*surfaceNormal).length();
    halfwayLength = halfway.length();
    cosAngleOfIncidence = halfway.dotProduct(*surfaceNormal);

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
LightingEngine::doDiffuse(Texture *texture, RayWithSegments *lightSourceRay, Vector3Dd *surfaceNormal,
    RGBAColor *colour, RGBAColor *lightColour, RGBAColor *surfaceColour,
    double attenuation)
{
    double cosAngleOfIncidence, intensity, randomNumber;

    cosAngleOfIncidence = (*surfaceNormal).dotProduct(lightSourceRay->direction);
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

void
LightingEngine::perturbNormal(Vector3Dd *newNormal, Texture *texture,
    Vector3Dd *intersectionPoint, Vector3Dd *surfaceNormal)
{
    Vector3Dd transformedPoint;
    register double x;
    register double y;
    register double z;

    if (texture->Bump_Number == NO_BUMPS) {
        *newNormal = *surfaceNormal;
        return;
    }

    if (texture->Texture_Transformation) {
        Transformation::MInverseTransformVector(&transformedPoint, intersectionPoint,
            texture->Texture_Transformation);
    } else {
        transformedPoint = *intersectionPoint;
    }

    x = transformedPoint.x;
    y = transformedPoint.y;
    z = transformedPoint.z;

    switch (texture->Bump_Number) {

    case WAVES:
        BumpTextureFixture::waves(x, y, z, texture, newNormal);
        break;

    case RIPPLES:
        BumpTextureFixture::ripples(x, y, z, texture, newNormal);
        break;

    case WRINKLES:
        BumpTextureFixture::wrinkles(x, y, z, texture, newNormal);
        break;

    case BUMPS:
        BumpTextureFixture::bumps(x, y, z, texture, newNormal);
        break;

    case DENTS:
        BumpTextureFixture::dents(x, y, z, texture, newNormal);
        break;

    case BUMPY1:
        TextureFixture::bumpy1(x, y, z, texture, newNormal);
        break;

    case BUMPY2:
        TextureFixture::bumpy2(x, y, z, texture, newNormal);
        break;

    case BUMPY3:
        TextureFixture::bumpy3(x, y, z, texture, newNormal);
        break;

    case BUMPMAP:
        MapTextureFixture::bumpMap(x, y, z, texture, newNormal);
        break;
    }
}

void
LightingEngine::ambient(Texture *texture, RGBAColor *surfaceColour, RGBAColor *colour,
    double attenuation)
{
    if (texture->Object_Ambient == 0.0) {
        return;
    }

    colour->Red += surfaceColour->Red * texture->Object_Ambient * attenuation;
    colour->Green +=
        surfaceColour->Green * texture->Object_Ambient * attenuation;
    colour->Blue += surfaceColour->Blue * texture->Object_Ambient * attenuation;
}

void
LightingEngine::diffuse(Texture *texture, Vector3Dd *intersectionPoint, RayWithSegments *eye,
    Vector3Dd *surfaceNormal, RGBAColor *surfaceColour, RGBAColor *colour,
    double attenuation)
{
    double lightSourceDepth;
    RayWithSegments lightSourceRay;
    Light *lightSource;
    SimpleBody *blockingObject;
    int intersectionFound;
    Intersection *localIntersection;
    Vector3Dd rEye;
    RGBAColor lightColour;
    PriorityQueueNode *localQueue;

    rEye.x = 0;
    rEye.y = 0;
    rEye.z = 0;

    if ((texture->Object_Diffuse == 0.0) && (texture->Object_Specular == 0.0) &&
        (texture->Object_Phong == 0.0)) {
        return;
    }

    if (texture->Object_Specular != 0.0) {
        rEye.x = -eye->direction.x;
        rEye.y = -eye->direction.y;
        rEye.z = -eye->direction.z;
    }

    localQueue = PriorityQueuePool::pqPop(128);

    for (lightSource = globalFrame.Light_Sources; lightSource != nullptr;
         lightSource = lightSource->Next_Light_Source) {
        intersectionFound = FALSE;

        doLight(lightSource, &lightSourceDepth, &lightSourceRay,
            intersectionPoint, &lightColour);

        /* What objects does this ray intersect? */
        if (quality > 3) {
            shadowTestFlag = TRUE;
            for (blockingObject = globalFrame.Objects;
                 blockingObject != nullptr;
                 blockingObject = blockingObject->Next_Object) {

                shadowRayTests++;
                for (GeometryOperations::allIntersections(
                         blockingObject, &lightSourceRay, localQueue);
                     (localIntersection = localQueue->getHighest()) != nullptr;
                     localQueue->deleteHighest()) {

                    if ((localIntersection->Depth <
                            lightSourceDepth - Small_Tolerance) &&
                        (localIntersection->Depth > SHADOW_TOLERANCE)) {

                        /* Does the object not cast a shadow? */
                        if (!localIntersection->Object->No_Shadow_Flag) {
                            if (doBlocking(localIntersection, &lightColour,
                                    localQueue)) {
                                intersectionFound = TRUE;
                                break;
                            }
                        }
                    }
                }
                if (intersectionFound) {
                    break;
                }
            }
            shadowTestFlag = FALSE;
        }

        /* If light source was not blocked by any intervening object, then
              calculate it's contribution to the object's overall illumination
         */

        if (!intersectionFound) {
            if (texture->Object_Phong > 0.0) { /* Phong Hilite */
                doPhong(texture, &lightSourceRay, eye->direction, surfaceNormal,
                    colour, &lightColour, surfaceColour);
            }

            if (texture->Object_Specular > 0.0) { /* Specular Hilite */
                doSpecular(texture, &lightSourceRay, rEye, surfaceNormal,
                    colour, &lightColour, surfaceColour);
            }

            if (texture->Object_Diffuse > 0.0) { /* Normal Diffuse Illum. */
                doDiffuse(texture, &lightSourceRay, surfaceNormal, colour,
                    &lightColour, surfaceColour, attenuation);
            }
        }
    }
    localQueue->pushBackToPool();
}

void
LightingEngine::reflect(Texture *texture, Vector3Dd *intersectionPoint, RayWithSegments *ray,
    Vector3Dd *surfaceNormal, RGBAColor *colour)
{
    RayWithSegments newRay;
    RGBAColor tempColour;
    Vector3Dd localNormal;
    Vector3Dd normalProjection;
    Vector3Dd surfaceOffset;
    register double normalComponent;

    if (texture->Object_Reflection != 0.0) {
        reflectedRaysTraced++;
        normalComponent = ray->direction.dotProduct(*surfaceNormal);
        if (normalComponent < 0.0) {
            localNormal = *surfaceNormal;
            normalComponent *= -1.0;
        } else
            VectorOps::vScale(localNormal, *surfaceNormal, -1.0);

        VectorOps::vScale(normalProjection, localNormal, normalComponent);
        normalProjection.scale(2.0);
        VectorOps::vAdd(newRay.direction, ray->direction, normalProjection);
        newRay.position = *intersectionPoint;

        /* ARE 08/25/91 */

        VectorOps::vScale(surfaceOffset, newRay.direction, 2.0 * Small_Tolerance);
        newRay.position.add(surfaceOffset);

        newRay.copyContainersFrom(ray);
        traceLevel++;
        Color::makeColor(&tempColour, 0.0, 0.0, 0.0);
        newRay.quadricConstantsCached = FALSE;
        RenderEngine::trace(&newRay, &tempColour);
        traceLevel--;

        colour->Red += tempColour.Red * texture->Object_Reflection;
        colour->Green += tempColour.Green * texture->Object_Reflection;
        colour->Blue += tempColour.Blue * texture->Object_Reflection;
    }
}

void
LightingEngine::refract(Texture *texture, Vector3Dd *intersectionPoint, RayWithSegments *ray,
    Vector3Dd *surfaceNormal, RGBAColor *colour)
{
    RayWithSegments newRay;
    RGBAColor tempColour;
    Vector3Dd localNormal;
    Vector3Dd rayDirection;
    register double normalComponent;
    register double tempIor;
    double temp, ior;

    if (surfaceNormal == nullptr) {
        newRay.position = *intersectionPoint;
        newRay.direction = ray->direction;

        newRay.copyContainersFrom(ray);
        traceLevel++;
        transmittedRaysTraced++;
        Color::makeColor(&tempColour, 0.0, 0.0, 0.0);
        newRay.quadricConstantsCached = FALSE;
        RenderEngine::trace(&newRay, &tempColour);
        traceLevel--;
        (colour->Red) += tempColour.Red;
        (colour->Green) += tempColour.Green;
        (colour->Blue) += tempColour.Blue;
    } else {
        refractedRaysTraced++;
        normalComponent = ray->direction.dotProduct(*surfaceNormal);
        if (normalComponent <= 0.0) {
            localNormal.x = surfaceNormal->x;
            localNormal.y = surfaceNormal->y;
            localNormal.z = surfaceNormal->z;
            normalComponent *= -1.0;
        } else {
            VectorOps::vScale(localNormal, *surfaceNormal, -1.0);
        }

        newRay.copyContainersFrom(ray);

        if (ray->containingIndex == -1) {
            /* The ray is entering from the atmosphere */
            newRay.enterContainingMedium(texture);
            ior = (globalFrame.Atmosphere_IOR) /
                  (texture->Object_Index_Of_Refraction);
        } else {
            /* The ray is currently inside an object */
            if (newRay.containingTextures[newRay.containingIndex] == texture)
            /*            if (inside) */
            {
                /* The ray is leaving the current object */
                newRay.exitContainingMedium();
                if (newRay.containingIndex == -1) {
                    /* The ray is leaving into the atmosphere */
                    tempIor = globalFrame.Atmosphere_IOR;
                } else {
                    /* The ray is leaving into another object */
                    tempIor = newRay.containingIORs[newRay.containingIndex];
                }

                ior = (texture->Object_Index_Of_Refraction) / tempIor;
            } else {
                /* The ray is entering a new object */
                tempIor = newRay.containingIORs[newRay.containingIndex];
                newRay.enterContainingMedium(texture);

                ior = tempIor / (texture->Object_Index_Of_Refraction);
            }
        }

        temp = 1.0 + ior * ior * (normalComponent * normalComponent - 1.0);
        if (temp < 0.0) {
            /* Total internal reflection - not yet implemented.
    reflect (texture, intersectionPoint, ray, surfaceNormal, colour);
    */
            return;
        }

        temp = ior * normalComponent - sqrt(temp);
        localNormal.scale(temp);
        VectorOps::vScale(rayDirection, ray->direction, ior);
        VectorOps::vAdd(newRay.direction, localNormal, rayDirection);
        newRay.direction.normalize();

        newRay.position = *intersectionPoint;
        traceLevel++;
        Color::makeColor(&tempColour, 0.0, 0.0, 0.0);
        newRay.quadricConstantsCached = FALSE;

        RenderEngine::trace(&newRay, &tempColour);
        traceLevel--;

        (colour->Red) += (tempColour.Red) * (texture->Object_Refraction);
        (colour->Green) += (tempColour.Green) * (texture->Object_Refraction);
        (colour->Blue) += (tempColour.Blue) * (texture->Object_Refraction);
    }
}

void
LightingEngine::computeReflectedColour(RayWithSegments *ray, Texture *texture,
    Intersection *rayIntersection, RGBAColor *surfaceColour,
    RGBAColor *filterColour, RGBAColor *colour)
{
    Vector3Dd surfaceNormal;
    double normalDirection, attenuation;
    RGBAColor emittedColour;

    /* This variable keeps track of how much colour comes from the surface
of the object and how much is transmited through. */

    Color::makeColor(&emittedColour, 0.0, 0.0, 0.0);

    if (texture == nullptr) {
        texture = rayIntersection->Object->Object_Texture;
    }

    if (quality <= 1) {
        surfaceColour->Alpha = 0.0;

        colour->Red += surfaceColour->Red * filterColour->Alpha;
        colour->Green += surfaceColour->Green * filterColour->Alpha;
        colour->Blue += surfaceColour->Blue * filterColour->Alpha;
        return;
    }

    GeometryOperations::normal(&surfaceNormal, (SimpleBody *)rayIntersection->Shape,
        &rayIntersection->Point);

    if (quality >= 8) {
        perturbNormal(
            &surfaceNormal, texture, &rayIntersection->Point, &surfaceNormal);
    }

    /* If the surface normal points away, flip its direction. */
    normalDirection = surfaceNormal.dotProduct(ray->direction);
    if (normalDirection > 0.0) {
        surfaceNormal.scale(-1.0);
    }

    attenuation = filterColour->Alpha * (1.0 - surfaceColour->Alpha);

    ambient(texture, surfaceColour, &emittedColour, attenuation);
    diffuse(texture, &rayIntersection->Point, ray, &surfaceNormal,
        surfaceColour, &emittedColour, attenuation);
    colour->Red += emittedColour.Red;
    colour->Green += emittedColour.Green;
    colour->Blue += emittedColour.Blue;
    if (quality >= 8) {
        reflect(texture, &rayIntersection->Point, ray, &surfaceNormal, colour);
    }
}

void
LightingEngine::determineSurfaceColour(
    Intersection *rayIntersection, RGBAColor *colour, RayWithSegments *ray, int shadowRay)
{
    RGBAColor surfaceColour;
    RGBAColor refractedColour;
    RGBAColor filterColour;
    Texture *tempTexture;
    Texture *texture;
    Vector3Dd surfaceNormal;
    double normalDirection;
    int surface;

    if (!shadowRay)
        Color::makeColor(colour, 0.0, 0.0, 0.0);

    if (Options & DEBUGGING) {
        if (rayIntersection->Shape->Shape_Colour) {
            printf("Depth: %f Object %d Colour %f %f %f ",
                rayIntersection->Depth, rayIntersection->Shape->Type,
                rayIntersection->Shape->Shape_Colour->Red,
                rayIntersection->Shape->Shape_Colour->Green,
                rayIntersection->Shape->Shape_Colour->Blue);
        } else {
            printf("Depth: %f Object %d Colour NIL ", rayIntersection->Depth,
                rayIntersection->Shape->Type);
        }
    }

    Color::makeColor(&surfaceColour, 0.0, 0.0, 0.0);

    /* Is there a texture in the shape?  If not, use the one in the object. */
    if ((texture = rayIntersection->Shape->Shape_Texture) == nullptr) {
        texture = rayIntersection->Object->Object_Texture;
    }
    /* Check to see if this object/shape has a material_map texture, if so */
    /* then change the texture pointer to point to the mapped texture - CdW 7/91
     */
    if (texture->Texture_Number == MATERIAL_MAP_TEXTURE) {
        texture = MapTextureFixture::materialMap(&rayIntersection->Point, texture);
    }

    /* If this is just a shadow ray and we're rendering low quality, then return
     */

    if (shadowRay && (quality <= 5)) {
        return;
    }

    Color::makeColor(&filterColour, 1.0, 1.0, 1.0);
    filterColour.Alpha = 1.0;

    /* Now, we perform the lighting calculations. */
    for (surface = 1, tempTexture = texture;
         (tempTexture != nullptr) && (filterColour.Alpha > 0.01);
         surface++, tempTexture = tempTexture->Next_Texture) {

        Color::makeColor(&surfaceColour, 0.0, 0.0, 0.0);
        if (quality <= 5) {
            if (rayIntersection->Shape->Shape_Colour != nullptr) {
                surfaceColour = *rayIntersection->Shape->Shape_Colour;
            } else if (rayIntersection->Object->Object_Colour != nullptr) {
                surfaceColour = *rayIntersection->Object->Object_Colour;
            } else {
                Color::makeColor(&surfaceColour, 0.5, 0.5, 0.5);
            }
        } else {
            ColorTextureFixture::colourAt(&surfaceColour, tempTexture, &rayIntersection->Point);
        }
        /* We don't need to compute the lighting characteristics for shadow
         * rays. */
        if (!shadowRay) {
            computeReflectedColour(ray, tempTexture, rayIntersection,
                &surfaceColour, &filterColour, colour);
        }

        if (Options & DEBUGGING) {
            printf("Surface %d\n", surface);
            printf("    Surf: %6.4f %6.4f %6.4f %6.4f\n", surfaceColour.Red,
                surfaceColour.Green, surfaceColour.Blue, surfaceColour.Alpha);
            printf("    Filter_Colour:    %6.4f %6.4f %6.4f %6.4f  Final "
                   "Colour: %6.4f %6.4f %6.4f %6.4f  \n",
                filterColour.Red, filterColour.Green, filterColour.Blue,
                filterColour.Alpha, colour->Red, colour->Green, colour->Blue,
                colour->Alpha);
        }

        filterColour.Red *= surfaceColour.Red;
        filterColour.Green *= surfaceColour.Green;
        filterColour.Blue *= surfaceColour.Blue;

        filterColour.Alpha *= surfaceColour.Alpha;
    }

    /* For shadow rays, we have the filter colour now - time to return */
    if (shadowRay) {

        if (filterColour.Alpha < 0.01) {
            Color::makeColor(colour, 0.0, 0.0, 0.0);
            return;
        }

        if (texture->Object_Refraction > 0.0) {
            colour->Red *= filterColour.Red * texture->Object_Refraction *
                           filterColour.Alpha;
            colour->Green *= filterColour.Green * texture->Object_Refraction *
                             filterColour.Alpha;
            colour->Blue *= filterColour.Blue * texture->Object_Refraction *
                            filterColour.Alpha;
        } else {
            colour->Red *= filterColour.Red * filterColour.Alpha;
            colour->Green *= filterColour.Green * filterColour.Alpha;
            colour->Blue *= filterColour.Blue * filterColour.Alpha;
        }
        return;
    }

    if ((filterColour.Alpha > 0.01) && (quality > 5)) {
        Color::makeColor(&refractedColour, 0.0, 0.0, 0.0);

        if (texture->Object_Refraction > 0.0) {
            GeometryOperations::normal(&surfaceNormal, (SimpleBody *)rayIntersection->Shape,
                &rayIntersection->Point);

            if (quality > 7) {
                perturbNormal(&surfaceNormal, texture, &rayIntersection->Point,
                    &surfaceNormal);
            }

            /* If the surface normal points away, flip its direction. */
            normalDirection = surfaceNormal.dotProduct(ray->direction);
            if (normalDirection > 0.0) {
                surfaceNormal.scale(-1.0);
            }

            refract(texture, &rayIntersection->Point, ray, &surfaceNormal,
                &refractedColour);
        } else {
            refract(texture, &rayIntersection->Point, ray, nullptr,
                &refractedColour);
        }

        colour->Red +=
            filterColour.Red * refractedColour.Red * filterColour.Alpha;
        colour->Green +=
            filterColour.Green * refractedColour.Green * filterColour.Alpha;
        colour->Blue +=
            filterColour.Blue * refractedColour.Blue * filterColour.Alpha;

        if (texture->Object_Refraction > 0.0 &&
            texture->Object_Transmit > 0.0) {
            Color::makeColor(&refractedColour, 0.0, 0.0, 0.0);
            refract(texture, &rayIntersection->Point, ray, nullptr,
                &refractedColour);
            colour->Red +=
                filterColour.Red * refractedColour.Red * filterColour.Alpha;
            colour->Green +=
                filterColour.Green * refractedColour.Green * filterColour.Alpha;
            colour->Blue +=
                filterColour.Blue * refractedColour.Blue * filterColour.Alpha;
        }
    }

    if (globalFrame.Fog_Distance != 0.0) {
        fog(rayIntersection->Depth, &globalFrame.Fog_Colour,
            globalFrame.Fog_Distance, colour);
    }
}
